// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cache.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <csignal>
#include <boost/variant.hpp>

#include "cacheController.h"
#include "../memEvent.h"
#include <vector>

using namespace SST;
using namespace SST::MemHierarchy;


Sieve::Sieve(Component *comp, Params &params) {
    cpu_link = configureLink("cpu_link",
               new Event::Handler<Sieve>(this, &Sieve::processEvent));
}


void Sieve::processEvent(SST::Event* _ev) {
    Command cmd     = event->getCmd();
    MemEvent* event = static_cast<MemEvent*>(_ev);
    
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
    MemEvent* origEvent;
    
    switch(cmd){
        case GetS:
        case GetX:
        case GetSEx:
            // track times in our separate queue
            if (startTimeList.find(event) == startTimeList.end()) startTimeList.insert(std::pair<MemEvent*,uint64>(event, timestamp_));
            processCacheRequest(event, cmd, baseAddr);
            break;
        case PutS:
        case PutM:
        case PutE:
        case PutX:
        case PutXE:
            processCacheReplacement(event, cmd, baseAddr);
            break;
        default:
            break;
    }
    delete _ev;
}


/**
 *  Handle a request from the CPU
 *  1. Locate line, if present, in the cache array & update its replacement manager state
 *  2. If cache miss, allocate a new cache line, possibly evicting another line
 *  5. Send response to requestor
 */
void Sieve::processCacheRequest(MemEvent* _event, Command _cmd, Addr _baseAddr){
    CacheLine* cacheLine;
    
    int lineIndex = cf_.cacheArray_->find(_baseAddr, updateLine);
        
    if(isCacheMiss(lineIndex)){                                     /* Miss.  If needed, evict candidate */
         if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L3_,"-- Cache Miss --\n");
             allocateCacheLine(_event, _baseAddr, lineIndex);            /* Function may except here to wait for eviction */
    }
        
    cacheLine = getCacheLine(lineIndex);
    handleRequest(_event, _cmd, cacheLine);
}


void Sieve::processCacheReplacement(MemEvent* _event, Command _cmd, Addr _baseAddr){
    CacheLine* cacheLine;

    bool updateLine = false;
    int lineIndex = cf_.cacheArray_->find(_baseAddr, updateLine);
        
    if (isCacheMiss(lineIndex)){                /* Miss.  If needed, evict candidate */
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L3_,"-- Cache Miss --\n");
            allocateCacheLine(_event, _baseAddr, lineIndex);
        }
    }
    
    cacheLine = getCacheLine(lineIndex);
    handleRequest(_event, _cmd, cacheLine);
}


void Sieve::handleRequest(MemEvent* _event, Command _cmd, CacheLine *_cacheLine) {
    vector<uint8_t>* data = _cacheLine->getData();
  
    // send response back
    switch (_cmd) {
        case GetS:
             sendResponse(_event, S, data);
             break;
        case GetX:
        case GetSEx:
             sendResponse(_event, M, data); // ignoring atomicity
             break;
        default:
             break;
    }
}


/**
 *  Send response to requestor
 *  Latency: Add 1 to the incoming timestamp
 */
void Sieve::sendResponse(MemEvent *_event, State _newState, std::vector<uint8_t>* _data){
    
    Command cmd = _event->getCmd();
    MemEvent * responseEvent = _event->makeResponse(_newState);
    responseEvent->setDst(_event->getSrc());
    bool noncacheable = _event->queryFlag(MemEvent::F_NONCACHEABLE);
    
    if(!noncacheable) {
        /* Only return the desire word */
        Addr base    = (_event->getAddr()) & ~(((Addr)lineSize_) - 1);
        Addr offset  = _event->getAddr() - base;
        if(cmd != GetX) responseEvent->setPayload(_event->getSize(), &_data->at(offset));
        else{
            /* If write (GetX) and LLSC set, then check if operation was Atomic */
  	        responseEvent->setAtomic(true);
            }
    else responseEvent->setPayload(*_data);
    
    uint64_t deliveryTime = timestamp_ + 1;
    Response resp = {responseEvent, deliveryTime, true};
    // TODO: need to send response back to CPU: addToOutgoingQueue(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == _event->getBaseAddr()) d_->debug(_L3_,"TCC - Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s, Size = %i, Granted State = %s\n", deliveryTime, timestamp_, _event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
}



void Sieve::init() {}

void Sieve::finish(){
    listener_->printStats(*d_);
    delete cf_.cacheArray_;
    delete cf_.rm_;
    delete d_;
}



/* ---------------------------------
   Writeback Related Functions
   --------------------------------- */

void Sieve::allocateCacheLine(MemEvent* _event, Addr _baseAddr, int& _newCacheLineIndex) {
    CacheLine* wbCacheLine = findReplacementCacheLine(_baseAddr);
    /* OK to replace cache line  */
    replaceCacheLine(wbCacheLine->getIndex(), _newCacheLineIndex, _baseAddr);
}

CacheLine* Sieve::findReplacementCacheLine(Addr _baseAddr){
    int wbLineIndex = cf_.cacheArray_->preReplace(_baseAddr);
    CacheLine* wbCacheLine = cf_.cacheArray_->lines_[wbLineIndex];
    return wbCacheLine;
}

void Sieve::replaceCacheLine(int _replacementCacheLineIndex, int& _newCacheLineIndex, Addr _newBaseAddr){
    CCLine* wbCCLine = topCC_->getCCLine(_replacementCacheLineIndex);
    wbCCLine->clear();
    wbCCLine->setBaseAddr(_newBaseAddr);

    _newCacheLineIndex = _replacementCacheLineIndex;
    cf_.cacheArray_->replace(_newBaseAddr, _newCacheLineIndex);
}

/* -------------------------------------------------------------------------------------
            Helper Functions
 ------------------------------------------------------------------------------------- */

bool Sieve::isCacheMiss(int _lineIndex){
    if(_lineIndex == -1){
        return true;
    }
    else return false;
}

/**
 *  Determine whether an access will be a cache hit or not
 *  Cache hit if:
 *      Line is present in the cache
*  @return int indicating cache hit (0) or miss (1)
 */
int Sieve::isCacheHit(MemEvent* _event, Command _cmd, Addr _baseAddr) {
    int lineIndex = cf_.cacheArray_->find(_baseAddr,false);
     
    if (isCacheMiss(lineIndex))                         return 1;
    CacheLine* cacheLine = getCacheLine(lineIndex);
    
    return 0;
}



/* ---------------------------------------
   Extras
   --------------------------------------- */
CacheArray::CacheLine* Sieve::getCacheLine(int _lineIndex){
    if(_lineIndex == -1) return NULL;
    else return cf_.cacheArray_->lines_[_lineIndex];
}


