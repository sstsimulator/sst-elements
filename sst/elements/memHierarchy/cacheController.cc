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
    bool done;
    
    try{
        bool updateLine = !_mshrHit && MemEvent::isDataRequest(_cmd);
        int lineIndex = cf_.cacheArray_->find(_baseAddr, updateLine);   /* Update cacheline only if it's NOT mshrHit */
        
        if(isCacheMiss(lineIndex)){                                     /* Miss.  If needed, evict candidate */
            checkRequestValidity(_event);
            allocateCacheLine(_event, _baseAddr, lineIndex);
        }
        
        CacheLine* cacheLine = getCacheLine(lineIndex);
        checkCacheLineIsStable(_event, cacheLine, _cmd);                        /* If cache line is locked or in transition, wait until it is stable */
        
        bottomCC_->handleRequest(_event, cacheLine, _cmd);              /* upgrade or fetch line from higher level caches */
        if(cacheLine->inTransition()){
            upgradeCount_++;
            throw stallException();                                     /* stall request if upgrade is in progress */
        }
        
        if(!_event->isPrefetch()){                                      /* Don't do anything with TopCC if it is a prefetch request */
            done = topCC_->handleRequest(_event, cacheLine, _mshrHit);  /* Invalidate sharers, send respond to requestor if needed */
            postRequestProcessing(_event, cacheLine, done, _mshrHit);
        }
    
    }
    catch(stallException const& e){
        processRequestInMSHR(_baseAddr, _event);                        /* This request needs to stall until another pending request finishes.  This event is now in the  MSHR waiting to 'reactive' upon completion of the outstanding request in progress  */
    }
    catch(ignoreEventException const& e){}
}


void Cache::processCacheInvalidate(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    
    if(!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;

    int lineIndex = cacheLine->index();

    if(!L1_){
        if(!processRequestInMSHR(_baseAddr, _event)){                   /* L1s wont stall because they don't have any sharers */
            d_->debug(_WARNING_,"WARNING!! c3\n");
            return;
        }
    }
    topCC_->handleInvalidate(lineIndex, _cmd);                          /* Invalidate lower levels */
    if(invalidatesInProgress(lineIndex)) return;
    
    bottomCC_->handleInvalidate(_event, cacheLine, _cmd);               /* Invalidate this cache line */
    if(!L1_) mshr_->removeElement(_baseAddr, _event);
    delete _event;
}


void Cache::processCacheResponse(MemEvent* _responseEvent, Addr _baseAddr){
    CacheLine* cacheLine = getCacheLine(_baseAddr); assert(cacheLine);
    
    MemEvent* origRequest = getOriginalRequest(mshr_->lookup(_baseAddr));
    updateUpgradeLatencyAverage(origRequest);
    
    bottomCC_->handleResponse(_responseEvent, cacheLine, origRequest);
    activatePrevEvents(_baseAddr);

    delete _responseEvent;
}


void Cache::processFetch(MemEvent* _event, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    Command cmd = _event->getCmd();

    if(!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;

    int lineIndex = cacheLine->index();

    if(!L1_){
        if(!processRequestInMSHR(_baseAddr, _event)){                   /* L1s wont stall because they don't have any sharers */
            d_->debug(_WARNING_,"WARNING!! c4\n");
            return;
        }
    }
    topCC_->handleInvalidate(lineIndex, cmd);
    if(invalidatesInProgress(lineIndex)) return;
    
    bottomCC_->handleFetchInvalidate(_event, cacheLine, cmd, _mshrHit);
    if(!L1_) mshr_->removeElement(_baseAddr, _event);

    delete _event;
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
    
    assert(!mshr_->exists(_baseAddr));
}


CacheArray::CacheLine* Cache::findReplacementCacheLine(Addr _baseAddr){
    int wbLineIndex = cf_.cacheArray_->preReplace(_baseAddr); assert(wbLineIndex >= 0);
    CacheLine* wbCacheLine = cf_.cacheArray_->lines_[wbLineIndex];    assert(wbCacheLine);
    wbCacheLine->setIndex(wbLineIndex);
    return wbCacheLine;
}


void Cache::candidacyCheck(MemEvent* _event, CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(stallException){
    d_->debug(_L1_,"Replacement cache needs to be evicted. WbAddr: %"PRIx64", St: %s\n", _wbCacheLine->getBaseAddr(), BccLineString[_wbCacheLine->getState()]);
    
    if(_wbCacheLine->isLockedByUser()){
        d_->debug(_WARNING_, "Warning: Replacement cache line is user-locked. WbCLine Addr: %"PRIx64"\n", _wbCacheLine->getBaseAddr());
        _wbCacheLine->eventsWaitingForLock_ = true;
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw stallException();
    }
    else if(isCandidateInTransition(_wbCacheLine)){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw stallException();
    }
    return;
}


bool Cache::isCandidateInTransition(CacheLine* _wbCacheLine){
    CCLine* wbCCLine = topCC_->getCCLine(_wbCacheLine->index());
    if(wbCCLine->inTransition() || CacheLine::inTransition(_wbCacheLine->getState())){
        d_->debug(_L1_,"Stalling request: Replacement cache line in transition.\n");
        return true;
    }
    return false;
}


void Cache::evictInHigherLevelCaches(CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(stallException){
    topCC_->handleEviction(_wbCacheLine->index(), _wbCacheLine->getState());
    CCLine* ccLine = topCC_->getCCLine(_wbCacheLine->index());
    
    if(ccLine->inTransition()){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw stallException();
    }
    
    ccLine->clear();
}


void Cache::writebackToLowerLevelCaches(MemEvent* _event, CacheLine* _wbCacheLine){
    bottomCC_->handleEviction(_wbCacheLine, _event->getGroupId());
    d_->debug(_L1_,"Replacement cache line evicted.\n");
}


void Cache::replaceCacheLine(int _replacementCacheLineIndex, int& _newCacheLineIndex, Addr _newBaseAddr){
    CCLine* wbCCLine = topCC_->getCCLine(_replacementCacheLineIndex);
    wbCCLine->setBaseAddr(_newBaseAddr);

    _newCacheLineIndex = _replacementCacheLineIndex;
    cf_.cacheArray_->replace(_newBaseAddr, _newCacheLineIndex);
    d_->debug(_L1_,"Allocated current request in a cache line.\n");
}


/* ---------------------------------------
   Invalidate Request helper functions
   --------------------------------------- */
bool Cache::invalidatesInProgress(int _lineIndex){
    if(L1_) return false;
    
    CCLine* ccLine = topCC_->getCCLine(_lineIndex);
    if(ccLine->inTransition()){
        d_->debug(_L1_,"Invalidate request forwared to HiLv caches.\n");
        return true;
    }
    return false;
}

bool Cache::shouldInvRequestProceed(MemEvent* _event, CacheLine* _cacheLine, Addr _baseAddr, bool _mshrHit){
    /* Scenario where this 'if' occurs:  HiLv$ evicts a shared line (S->I), sends PutS to LowLv$.
       Simultaneously, LowLv$ sends an Inv to HiLv$. Thus, HiLv$ sends an Inv an already invalidated line */
    if(!_cacheLine || (_cacheLine->getState() == I && !_mshrHit)){
        d_->debug(_WARNING_,"Warning: Ignoring request. CLine doesn't exists or invalid.\n");
        return false;
    }

    if(_cacheLine->isLockedByUser()){        /* If user-locked then wait this lock is released to activate this event. */
        if(!processRequestInMSHR(_baseAddr, _event)) {
            d_->debug(_WARNING_,"WARNING!! c0\n");
            return false;
        }
        incInvalidateWaitingForUserLock(groupId);
        _cacheLine->eventsWaitingForLock_ = true;
        d_->debug(_WARNING_,"Warning:  CLine is user-locked.\n");
        
        return false;
    }
    
    CCLine* ccLine = topCC_->getCCLine(_cacheLine->index());
    if(ccLine->getState() != V){
        d_->debug(_WARNING_,"WARNING!! c1\n");
        processRequestInMSHR(_baseAddr, _event);
        return false;
    }
    
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
            if(!mshr_->isHit(pointerAddr)){                                 /* Entry has been already been processed, delete mshr entry */
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
            if(mshr_->isHit(_baseAddr)){
                mshr_->insertAll(_baseAddr, mshrEntry);
                break;
            }
        }
        else{                                                               /* MemEvent Type */
            SimTime_t start = boost::get<MemEvent*>((*it).elem)->getStartTime();
            cont = activatePrevEvent(boost::get<MemEvent*>((*it).elem), mshrEntry, _baseAddr, it, i);
            if(!cont) break;
            else{
                totalLatency_ += (timestamp_ - start);
                mshrHits_++;
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
    
    /* If the event we just ran 'blocked', then there is not reason to activate other events. */
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
        d_->debug(_L1_,"Request Completed\n");
        if(cmd == PutM || cmd == PutE || cmd == PutX || cmd == PutXE ||
           (cmd == PutS && mshr_->exists(baseAddr) && getOriginalRequest(mshr_->lookup(baseAddr))->getCmd() == GetSEx)){
            /* Check if topCC line is locked */
            CCLine* ccLine = topCC_->getCCLine(_cacheLine->index());
            if(_cacheLine->unlocked() && ccLine->isValid() && !_mshrHit)
                activatePrevEvents(baseAddr);
            
        }
        reActivateEventWaitingForUserLock(_cacheLine, _mshrHit);
        d_->debug(_L1_,"Deleting Event\n");
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

void Cache::checkCacheLineIsStable(MemEvent* _event, CacheLine* _cacheLine, Command _cmd) throw(ignoreEventException){
    /* If cache line is in transition, that means this requests is a writeback from a lower level cache.
       In this case, it has to be a PutS requests because the only possible transition going on is SM.  We can just ignore
       the request after removing the sharer. */
    if(_cacheLine->inTransition()){
        assert(_cmd == PutS);
        topCC_->handleRequest(_event, _cacheLine, false);
        d_->debug(_L0_,"Sharer removed while cache line was in transition. Cmd = %s, St = %s\n", CommandString[_cmd], BccLineString[_cacheLine->getState()]);
        throw ignoreEventException();
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
MemEvent* Cache::getOriginalRequest(const vector<mshrType> _mshrEntry){
    if(_mshrEntry.front().elem.type() == typeid(MemEvent*))
        return boost::get<MemEvent*>(_mshrEntry.front().elem);
    else{
        Addr addr = boost::get<Addr>(_mshrEntry.front().elem);
        d_->debug(_ERROR_, "Not MemEvent type.  Pointer Addr = %"PRIx64"\n", addr);
        assert(0);
    }
}

void Cache::updateUpgradeLatencyAverage(MemEvent* _origMemEvent){
    SimTime_t start = _origMemEvent->getStartTime();
    totalUpgradeLatency_ += (timestamp_ - start);
    upgradeCount_++;
}


void Cache::errorChecking(){
    if(cf_.MSHRSize_ <= 0)             _abort(Cache, "MSHR size not specified correctly. \n");
    if(cf_.numLines_ <= 0)             _abort(Cache, "Number of lines not set correctly. \n");
    if(!isPowerOfTwo(cf_.lineSize_))   _abort(Cache, "Cache line size is not a power of two. \n");
}

void Cache::pMembers(){
    string protocolStr;
    if(cf_.protocol_) protocolStr = "MESI";
    else protocolStr = "MSI";
    
    d_->debug(_INFO_,"Coherence Protocol: %s \n", protocolStr.c_str());
    d_->debug(_INFO_,"Cache lines: %d \n", cf_.numLines_);
    d_->debug(_INFO_,"Cache line size: %d \n", cf_.lineSize_);
    d_->debug(_INFO_,"MSHR entries:  %d \n\n", cf_.MSHRSize_);
}

void Cache::retryRequestLater(MemEvent* _event){
    retryQueueNext_.push_back(_event);
}


CacheArray::CacheLine* Cache::getCacheLine(Addr _baseAddr){
    int lineIndex =  cf_.cacheArray_->find(_baseAddr, false);
    if(lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cf_.cacheArray_->lines_[lineIndex];
        cacheLine->setIndex(lineIndex);
        return cacheLine;
    }
}

CacheArray::CacheLine* Cache::getCacheLine(int _lineIndex){
    if(_lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cf_.cacheArray_->lines_[_lineIndex];
        cacheLine->setIndex(_lineIndex);
        return cacheLine;
    }
}


bool Cache::isCacheLineAllocated(int _lineIndex){
    return (_lineIndex == -1) ? false : true;
}


bool Cache::insertToMSHR(Addr _baseAddr, MemEvent* _event){
    return mshr_->insert(_baseAddr, _event);
}


bool Cache::processRequestInMSHR(Addr _baseAddr, MemEvent* _event){
    bool inserted = insertToMSHR(_baseAddr, _event);
    if(!inserted) {
        sendNACK(_event);
        return false;
    }
    return true;
}


void Cache::sendNACK(MemEvent* _event){
    if(_event->isHighNetEvent())        topCC_->sendNACK(_event);
    else if(_event->isLowNetEvent())    bottomCC_->sendNACK(_event);
    else
        _abort(Cache, "Command type not recognized, Cmd = %s\n", CommandString[_event->getCmd()]);
}

void Cache::processIncomingNACK(MemEvent* _origReqEvent){
    
    /* Determine what CC will retry sending the event */
    if(_origReqEvent->fromHighNetNACK())       topCC_->sendEvent(_origReqEvent);
    else if(_origReqEvent->fromLowNetNACK())   bottomCC_->sendEvent(_origReqEvent);
    else
        _abort(Cache, "Command type not recognized, Cmd = %s\n", CommandString[_origReqEvent->getCmd()]);
    
    d_->debug(_L0_,"Orig Cmd NACKed = %s \n", CommandString[_origReqEvent->getCmd()]);
    //delete _origReqEvent; TODO:  why does adding this line make some test fail?
}

void Cache::checkRequestValidity(MemEvent* _event) throw(ignoreEventException){
    Command cmd = _event->getCmd();
    assert(cmd != PutM && cmd != PutE && cmd != PutX && cmd != PutXE);
    if(cmd == PutS) {
        d_->debug(_WARNING_,"Ignoring PutS request. \n");
        throw ignoreEventException();
    }
}


bool operator== ( const mshrType& _n1, const mshrType& _n2) {
    if(_n1.elem.type() == typeid(Addr)) return false;
    return(boost::get<MemEvent*>(_n1.elem) == boost::get<MemEvent*>(_n2.elem));
}


void Cache::incTotalRequestsReceived(int _groupId){
    stats_[0].TotalRequestsReceived_++;
    if(groupStats_){
        assert(_groupId != 0 || _groupId != -1);
        stats_[_groupId].TotalRequestsReceived_++;
    }
}

void Cache::incTotalMSHRHits(int _groupId){
    stats_[0].TotalMSHRHits_++;
    if(groupStats_){
        assert(_groupId != 0 || _groupId != -1);
        stats_[_groupId].TotalMSHRHits_++;
    }
}

void Cache::incInvalidateWaitingForUserLock(int _groupId){
    stats_[0].InvWaitingForUserLock_++;
    if(groupStats_){
        assert(_groupId != 0 || _groupId != -1);
        stats_[_groupId].InvWaitingForUserLock_++;
    }
}



