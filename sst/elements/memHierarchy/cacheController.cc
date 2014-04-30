// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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
#include "cacheController.h"

#include <csignal>
#include <boost/variant.hpp>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include "memEvent.h"

#include "mshr.h"
#include "coherenceControllers.h"
#include "hash.h"


using namespace SST;
using namespace SST::MemHierarchy;



void Cache::processCacheRequest(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    try{
        bool updateCacheline = !_mshrHit && MemEvent::isDataRequest(_cmd);
        int lineIndex = cArray_->find(_baseAddr, updateCacheline);      /* Update cacheline only if it's NOT mshrHit */
        
        if(isCacheMiss(lineIndex)){                                     /* Miss.  If needed, evict candidate */
            checkRequestValidity(_event);
            allocateCacheLine(_event, _baseAddr, lineIndex);
        }
        
        CacheLine* cacheLine = getCacheLine(lineIndex);
        checkCacheLineIsStable(cacheLine, _cmd);                        /* If cache line is locked or in transition, wait until it is stable */
        
        bottomCC_->handleRequest(_event, cacheLine, _cmd);              /* upgrade or fetch line from higher level caches */
        if(cacheLine->inTransition()) throw stallException();           /* stall request if upgrade is in progress */
        
        bool done = topCC_->handleRequest(_event, cacheLine, _mshrHit); /* Invalidate sharers, send respond to requestor if needed */
        postRequestProcessing(_event, cacheLine, done, _mshrHit);
    
    }
    catch(stallException const& e){
        mshr_->insert(_baseAddr, _event);                               /* This request needs to wait for another request to finish.  This event is now in the  MSHR waiting to 'reactive' upon completion of the outstanding request in progress  */
    }
    catch(ignoreEventException const& e){}
}


void Cache::processCacheInvalidate(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    if(!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;
    
    topCC_->handleInvalidate(cacheLine->index(), _cmd);     /* Invalidate lower levels */
    if(invalidatesInProgress(cacheLine->index())) return;
    
    bottomCC_->handleInvalidate(_event, cacheLine, _cmd);   /* Invalidate this cache line */
    mshr_->removeElement(_baseAddr, _event);
    delete _event;
    return;
}


void Cache::processCacheResponse(MemEvent* _responseEvent, Addr _baseAddr){
    CacheLine* cacheLine = getCacheLine(_baseAddr); assert(cacheLine);
    
    bottomCC_->handleResponse(_responseEvent, cacheLine, mshr_->lookup(_baseAddr));
    if(topCC_->getState(cArray_->find(_baseAddr, false)) == V) activatePrevEvents(_baseAddr);
    else d_->debug(_L1_,"Received AccessAck but states are still not valid.  BottomState: %s\n",
                   BccLineString[cacheLine->getState()]);
    
    delete _responseEvent;
}


void Cache::processFetch(MemEvent* _event, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    Command cmd = _event->getCmd();
    
    if(!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;

    topCC_->handleFetchInvalidate(cacheLine, cmd);
    if(invalidatesInProgress(cacheLine->index())) return;
    
    bottomCC_->handleFetchInvalidate(_event, cacheLine, cmd, _mshrHit);
    mshr_->removeElement(_baseAddr, _event);
}


/* ---------------------------------
   Writeback Related Functions
   --------------------------------- */

void Cache::allocateCacheLine(MemEvent* _event, Addr _baseAddr, int& _newCacheLineIndex) throw(stallException){
    CacheLine* wbCacheLine = findReplacementCacheLine(_baseAddr);
    
    /* Is writeback candidate invalid and not in transition? */
    if(wbCacheLine->valid()){
        candidacyCheck(_event, wbCacheLine, _baseAddr);
        evictInHigherLevelCaches(wbCacheLine, _baseAddr);
        writebackToLowerLevelCaches(_event, wbCacheLine);
    }
    replaceCacheLine(wbCacheLine->index(), _newCacheLineIndex, _baseAddr); /* OK to change addr of topCC cache line, OK to replace cache line  */
    if(!isCacheLineAllocated(_newCacheLineIndex)) throw stallException();
}


CacheArray::CacheLine* Cache::findReplacementCacheLine(Addr _baseAddr){
    int wbLineIndex = cArray_->preReplace(_baseAddr);        assert(wbLineIndex >= 0);
    CacheLine* wbCacheLine = cArray_->lines_[wbLineIndex];   assert(wbCacheLine);
    wbCacheLine->setIndex(wbLineIndex);
    return wbCacheLine;
}


void Cache::candidacyCheck(MemEvent* _event, CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(stallException){
    d_->debug(_L1_,"Replacement cache needs to be evicted. WbAddr: %"PRIx64", St: %s\n", _wbCacheLine->getBaseAddr(), BccLineString[_wbCacheLine->getState()]);
    
    if(_wbCacheLine->isLockedByUser()){
        d_->debug(_WARNING_, "Warning: Replacement cache line is user-locked. WbCLine Addr: %"PRIx64"\n", _wbCacheLine->getBaseAddr());
        retryRequestLater(_event);
        throw stallException();
    }
    else if(isCandidateInTransition(_wbCacheLine)){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw stallException();
    }
    return;
}


bool Cache::isCandidateInTransition(CacheLine* _wbCacheLine){
    CCLine* wbCCLine = getCCLine(_wbCacheLine->index());
    if(CacheLine::inTransition(_wbCacheLine->getState()) || (!L1_ && wbCCLine->getState() != V)){
        d_->debug(_L1_,"Stalling request: Replacement cache line in transition.\n");
        return true;
    }
    return false;
}


void Cache::evictInHigherLevelCaches(CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(stallException){
    bool invalidatesSent = topCC_->handleEviction(_wbCacheLine->index(), _wbCacheLine->getState());
    if(invalidatesSent){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw stallException();
    }
    if(!L1_){
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_wbCacheLine->index()];
        ccLine->clear();
    }
}


void Cache::writebackToLowerLevelCaches(MemEvent* _event, CacheLine* _wbCacheLine){
    bottomCC_->handleEviction(_wbCacheLine);
    d_->debug(_L1_,"Replacement cache line evicted.\n");
}


void Cache::replaceCacheLine(int _replacementCacheLineIndex, int& _newCacheLineIndex, Addr _newBaseAddr){
    if(!L1_){
        CCLine* wbCCLine = ((MESITopCC*)topCC_)->ccLines_[_replacementCacheLineIndex];
        wbCCLine->setBaseAddr(_newBaseAddr);
    }
    _newCacheLineIndex = _replacementCacheLineIndex;
    cArray_->replace(_newBaseAddr, _newCacheLineIndex);
    d_->debug(_L1_,"Allocated current request in a cache line.\n");
}


/* ---------------------------------------
   Invalidate Request helper functions
   --------------------------------------- */
bool Cache::invalidatesInProgress(int _lineIndex){
    if(L1_) return false;
    
    CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_lineIndex];
    if(ccLine->inTransition()){
        d_->debug(_L1_,"Invalidate request needs to be forwared to HiLv caches.\n");
        return true;
    }
    return false;
}

bool Cache::shouldInvRequestProceed(MemEvent* _event, CacheLine* _cacheLine, Addr _baseAddr, bool _mshrHit){
    /* Scenario where this 'if' occurs:  HiLv$ evicts a shared line (S->I), sends PutS to LowLv$.
       Simultaneously, LowLv$ sends an Inv to HiLv$. Thus, HiLv$ sends an Inv an already invalidated line */
    if(!_cacheLine || (_cacheLine->getState() == I && !_mshrHit)){
        mshr_->removeElement(_baseAddr, _event);
        d_->debug(_WARNING_,"Warning: Ignoring request. CLine doesn't exists or invalid.\n");
        return false;
    }
   
    if(_cacheLine->isLockedByUser()){        /* If user-locked then wait this lock is released to activate this event. */
        STAT_InvalidateWaitingForUserLock_++;
        _cacheLine->eventsWaitingForLock_ = true;
        d_->debug(_WARNING_,"Warning:  CLine is user-locked.\n");
        return false;
    }
    
    if(!L1_){
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_cacheLine->index()];
        if(ccLine->getState() != V) return false;
    }
    d_->debug(_L1_,"Invalidate request is valid. State: %s\n", BccLineString[_cacheLine->getState()]);
    return true;
}

/* -------------------------------------------------------------------------------------
            Helper Functions
 ------------------------------------------------------------------------------------- */

void Cache::activatePrevEvents(Addr _baseAddr){
    if(!mshr_->isHit(_baseAddr)) return;
    
    vector<mshrType> mshrEntry = mshr_->removeAll(_baseAddr);
    bool cont;    int i = 0;
    d_->debug(_L1_,"---------start--------- Size: %lu\n", mshrEntry.size());
    
    for(vector<mshrType>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
        if((*it).elem.type() == typeid(Addr)){                             /* Pointer Type */
            Addr pointerAddr = boost::get<Addr>((*it).elem);
            d_->debug(_L3_,"Pointer Addr: %"PRIx64"\n", pointerAddr);
            if(!mshr_->isHit(pointerAddr)){ /* Entry has been already been processed, delete mshr entry */
                mshrEntry.erase(it);
                continue;
            }
            
            vector<mshrType> pointerMSHR = mshr_->removeAll(pointerAddr);

            for(vector<mshrType>::iterator it2 = pointerMSHR.begin(); it2 != pointerMSHR.end(); i++){
                assert((*it2).elem.type() == typeid(MemEvent*));
                cont = activatePrevEvent(boost::get<MemEvent*>((*it2).elem), pointerMSHR, pointerAddr, it2, i);
                if(!cont) break;
            }
            
            mshrEntry.erase(it);
        }
        else{                                                               /* MemEvent Type */
            SimTime_t start = boost::get<MemEvent*>((*it).elem)->getStartTime();
            cont = activatePrevEvent(boost::get<MemEvent*>((*it).elem), mshrEntry, _baseAddr, it, i);
            if(!cont) break;
            else{
                totalUpgradeLatency_ += (timestamp_ - start);
                upgradeCount_++;
            }
        }
    }
    d_->debug(_L1_,"---------end---------\n");
}


bool Cache::activatePrevEvent(MemEvent* _event, vector<mshrType>& _mshrEntry, Addr _addr, vector<mshrType>::iterator _it, int _i){
    d_->debug(_L1_,"Activating event #%i, cmd = %s, bsAddr: %"PRIx64", addr: %"PRIx64", dst: %s\n",
                  _i, CommandString[_event->getCmd()], toBaseAddr(_event->getAddr()), _event->getAddr(), _event->getDst().c_str());
    d_->debug(_L1_,"--------------------------------------\n");
    this->processEvent(_event, true);
    d_->debug(_L1_,"--------------------------------------\n");

    _mshrEntry.erase(_it);
    
    // If the event we just ran 'blocked', then there is not reason to activate other events.
    if(mshr_->isHit(_addr)){
        mshr_->insertAll(_addr, _mshrEntry);
        return false;
    }
    return true;
}

void Cache::postRequestProcessing(MemEvent* _event, CacheLine* _cacheLine, bool _requestCompleted, bool _mshrHit) throw(stallException){
    Command cmd = _event->getCmd();
    Addr baseAddr = _cacheLine->getBaseAddr();
    if(_requestCompleted){
        if(cmd == PutM || cmd == PutE || cmd == PutX){
            if(!L1_){                  /* Check if topCC line is locked */
                CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_cacheLine->index()];
                if(_cacheLine->unlocked() && ccLine->isValid() && !_mshrHit) activatePrevEvents(baseAddr);
            }
        }
        reActivateEventWaitingForUserLock(_cacheLine, _mshrHit);
        delete _event;
    }
    else throw stallException();
}


void Cache::reActivateEventWaitingForUserLock(CacheLine* _cacheLine, bool _mshrHit){
    Addr baseAddr = _cacheLine->getBaseAddr();
    if(_cacheLine->eventsWaitingForLock_ && !_cacheLine->isLockedByUser() && !_mshrHit){
        d_->debug(_L1_,"Reactivating events:  User-locked cache line is now available\n");
        if(mshr_->isHit(baseAddr)) activatePrevEvents(baseAddr);
        _cacheLine->eventsWaitingForLock_ = false;
    }
}

void Cache::checkCacheLineIsStable(CacheLine* _cacheLine, Command _cmd) throw(ignoreEventException){
    if(_cacheLine->inTransition()){ /* Check if cache line is locked */
        d_->debug(_L1_,"Stalling request: Cache line in transition. BccSt: %s\n", BccLineString[_cacheLine->getState()]);
        throw ignoreEventException();
    }
    else if(!L1_){                  /* Check if topCC line is locked */
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_cacheLine->index()];
        if(ccLine->inTransition() && _cmd < 5 && _cmd > 8){  //InTransition && !PutM, !PutX, !PutS, !PutE
            d_->debug(_L1_,"Stalling request: Cache line in transition. TccSt: %s\n", TccLineString[ccLine->getState()]);
            throw ignoreEventException();
        }
    }
}

bool Cache::isCacheMiss(int _lineIndex){
    if(_lineIndex == -1){
        d_->debug(_L1_,"-- Cache Miss --\n");
        return true;
    }
    else return false;
}

/* ---------------------------------------
   Extras
   --------------------------------------- */

void Cache::errorChecking(){
    if(MSHRSize_ <= 0)             _abort(Cache, "MSHR size not specified correctly. \n");
    if(!isPowerOfTwo(MSHRSize_))   _abort(Cache, "MSHR size is not a power of two. \n");
    if(numLines_ <= 0)             _abort(Cache, "Number of lines not set correctly. \n");
    if(!isPowerOfTwo(lineSize_))   _abort(Cache, "Cache line size is not a power of two. \n");
}

void Cache::pMembers(){
    string protocolStr;
    if(protocol_) protocolStr = "MESI";
    else protocolStr = "MSI";
    
    d_->debug(_INFO_,"Coherence Protocol: %s \n", protocolStr.c_str());
    d_->debug(_INFO_,"Cache lines: %d \n", numLines_);
    d_->debug(_INFO_,"Cache line size: %d \n", lineSize_);
    d_->debug(_INFO_,"MSHR entries:  %d \n\n", MSHRSize_);
}

void Cache::retryRequestLater(MemEvent* _event){
    retryQueueNext_.push_back(_event);
}


CacheArray::CacheLine* Cache::getCacheLine(Addr _baseAddr){
    int lineIndex =  cArray_->find(_baseAddr, false);
    if(lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cArray_->lines_[lineIndex];
        cacheLine->setIndex(lineIndex);
        return cacheLine;
    }
}

CacheArray::CacheLine* Cache::getCacheLine(int _lineIndex){
    if(_lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cArray_->lines_[_lineIndex];
        cacheLine->setIndex(_lineIndex);
        return cacheLine;
    }
}


bool Cache::isCacheLineAllocated(int _lineIndex){
    return (_lineIndex == -1) ? false : true;
}


TopCacheController::CCLine* Cache::getCCLine(int _index){
    if(!L1_) return ((MESITopCC*)topCC_)->ccLines_[_index];
    else return NULL;

}

void Cache::checkRequestValidity(MemEvent* _event) throw(ignoreEventException){
    Command cmd = _event->getCmd();
    assert(cmd != PutM && cmd != PutE && cmd != PutX);
    if(cmd == PutS) {
        d_->debug(_WARNING_,"Ignoring PutS request. \n");
        throw ignoreEventException();
    }
}


bool operator== ( const mshrType& _n1, const mshrType& _n2) {
    if(_n1.elem.type() == typeid(Addr)) return false;
    return(boost::get<MemEvent*>(_n1.elem) == boost::get<MemEvent*>(_n2.elem));
}
