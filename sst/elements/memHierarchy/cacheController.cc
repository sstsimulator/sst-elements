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
#include <sst/core/interfaces/memEvent.h>

#include "mshr.h"
#include "coherenceControllers.h"
#include "hash.h"


using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


/* Function processes incomming access requests from HiLv$ or the CPU
 * It appropriately redirects requests to Top and/or Bottom controllers.  */
void Cache::processAccess(MemEvent *event, Command cmd, Addr baseAddr, bool reActivation) throw(stallException){
    int lineIndex = cArray_->find(baseAddr, (!reActivation && MemEvent::isDataRequest(cmd)));  //Update only if it's NOT reActivation
    
    if(isCacheMiss(lineIndex)){                                /* Miss.  If needed, evict candidate */
        checkRequestValidity(event, baseAddr);
        allocateCacheLine(event, baseAddr, lineIndex);
    }
    
    CacheLine* cacheLine = getCacheLine(lineIndex);
    if(!isCacheLineStable(cacheLine, cmd)) return;             /* If cache line is locked or in transition, wait until it is stable */
    
    /* Time to process request */
    bottomCC_->handleAccess(event, cacheLine, cmd);            /* upgrade or fetch line from higher level caches */
    if(cacheLine->inTransition()) return;                      /* stall request if upgrade is in progress */
    
    bool done = topCC_->handleAccess(event, cacheLine); /* Invalidate sharers, send respond to requestor if needed */
    postRequestProcessing(event, cacheLine, done, reActivation);
}


/* Function processes incomming invalidate messages.  Redirects message to Top and Bottom controllers appropriately */
void Cache::processInvalidate(MemEvent *event, Command cmd, Addr baseAddr, bool reActivation){
    CacheLine* cacheLine = getCacheLine(baseAddr);
    if(!shouldThisInvalidateRequestProceed(event, cacheLine, baseAddr, reActivation)) return;
    
    topCC_->handleInvalidate(cacheLine->index(), cmd);    /* Invalidate lower levels */
    if(invalidatesInProgress(cacheLine->index())) return;
    
    bottomCC_->handleInvalidate(event, cacheLine, cmd);   /* Invalidate this cache line */
    mshr_->removeElement(baseAddr, event);                //TODO: delete event;???
    return;
}


/* Function processes incomming Invalidate-Acknowledge messages.  Redirects message to Top Controller */
void Cache::processInvalidateAcknowledge(MemEvent* event, Addr baseAddr, bool reActivation){
    d_->debug(_L1_,"Processing invalidate ack\n");

    CacheLine* cacheLine = getCacheLine(baseAddr);
    if(!cacheLine) return;
    CCLine* ccLine = getCCLine(cacheLine->index());
    if(ccLine->getState() == V) return;
    
    topCC_->handleInvAck(event, ccLine);
    
    if(cacheLine->unlocked() && ccLine->isValid() && !reActivation) activatePrevEvents(baseAddr);
    else d_->debug(_L1_,"Received InvAck but states are still not valid.  BottomState: %s, Sharers: %i \n",
                   BccLineString[cacheLine->getState()], ccLine->numSharers());
}


/* Function processes incomming GetS/GetX responses.  Redirects message to Top Controller */
void Cache::processAccessAcknowledge(MemEvent* ackEvent, Addr baseAddr){
    CacheLine* cacheLine = getCacheLine(baseAddr); assert(cacheLine);
    
    bottomCC_->handleAccessAck(ackEvent, cacheLine, mshr_->lookup(baseAddr));
    if(topCC_->getState(cArray_->find(baseAddr, false)) == V) activatePrevEvents(baseAddr);
    else d_->debug(_L1_,"Received AccessAck but states are still not valid.  BottomState: %s\n",
                   BccLineString[cacheLine->getState()]);
    
    delete ackEvent;
}

/* Function processes incomming Fetch or FetchInvalidate requests from the Directory Controller
   Fetches send data, while FetchInvalidates evict data to the directory controller */
void Cache::processFetch(MemEvent* event, Addr baseAddr, bool reActivation){
    CacheLine* cacheLine = getCacheLine(baseAddr);
    Command cmd = event->getCmd();
    
    if(!shouldThisInvalidateRequestProceed(event, cacheLine, baseAddr, reActivation)) return;

    topCC_->handleFetchInvalidate(cacheLine, cmd);
    if(invalidatesInProgress(cacheLine->index())) return;
    
    bottomCC_->handleFetchInvalidate(event, cacheLine, cmd);
    mshr_->removeElement(baseAddr, event);
}


/* ---------------------------------
   Writeback Related Functions
   --------------------------------- */

void Cache::allocateCacheLine(MemEvent *event, Addr baseAddr, int& newCacheLineIndex) throw(stallException){
    CacheLine* wbCacheLine = findReplacementCacheLine(baseAddr);
    
    /* Is writeback candidate invalid and not in transition? */
    if(wbCacheLine->valid()){
        candidacyCheck(event, wbCacheLine, baseAddr);
        evictInHigherLevelCaches(wbCacheLine, baseAddr);
        writebackToLowerLevelCaches(event, wbCacheLine, baseAddr);
    }
    replaceCacheLine(wbCacheLine->index(), newCacheLineIndex, baseAddr); /* OK to change addr of topCC cache line, OK to replace cache line  */
    if(!isCacheLineAllocated(newCacheLineIndex)) throw stallException();
}

/* Depending on the replacement policy and cache array type, we need appropriately search for the replacement candidate */
CacheArray::CacheLine* Cache::findReplacementCacheLine(Addr baseAddr){
    int wbLineIndex = cArray_->preReplace(baseAddr);        assert(wbLineIndex >= 0);
    CacheLine* wbCacheLine = cArray_->lines_[wbLineIndex];  assert(wbCacheLine);
    wbCacheLine->setIndex(wbLineIndex);
    return wbCacheLine;
}


/* Evict cache line if necessary.  TopCC sends invalidates to lower level caches */
void Cache::evictInHigherLevelCaches(CacheLine* wbCacheLine, Addr requestBaseAddr) throw(stallException){
    bool invalidatesSent = topCC_->handleEviction(wbCacheLine->index(), wbCacheLine->getState());
    if(invalidatesSent){
        mshr_->insertPointer(wbCacheLine->getBaseAddr(), requestBaseAddr);
        throw stallException();
    }
    if(!L1_){
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[wbCacheLine->index()];
        ccLine->clear();
    }
}


/* Writeback cache line to lower level caches */
bool Cache::writebackToLowerLevelCaches(MemEvent *event, CacheLine* wbCacheLine, Addr requestBaseAddr){
    bottomCC_->handleEviction(event, wbCacheLine);

    assert(wbCacheLine->getAckCount() == 0 && !wbCacheLine->isLockedByUser() && wbCacheLine->getState() == I);
    d_->debug(_L1_,"Replacement cache line evicted.\n");
    return true;
}

/* Eviction succeeded.  Now 'cache line' is available and replaced with the Addr of the current request */
void Cache::replaceCacheLine(int replacementCacheLineIndex, int& newCacheLineIndex, Addr newBaseAddr){
    if(!L1_){
        CCLine* wbCCLine = ((MESITopCC*)topCC_)->ccLines_[replacementCacheLineIndex];
        wbCCLine->setBaseAddr(newBaseAddr);
    }
    newCacheLineIndex = replacementCacheLineIndex;
    cArray_->replace(newBaseAddr, newCacheLineIndex);
    d_->debug(_L1_,"Allocated current request in a cache line.\n");
}



void Cache::candidacyCheck(MemEvent* event, CacheLine* wbCacheLine, Addr requestBaseAddr) throw(stallException){
    d_->debug(_L1_,"Replacement cache needs to be evicted. WbAddr: %"PRIx64", St: %s\n", wbCacheLine->getBaseAddr(), BccLineString[wbCacheLine->getState()]);
    
    if(wbCacheLine->isLockedByUser()){
        d_->debug(_WARNING_, "Warning: Replacement cache line is user-locked. WbCLine Addr: %"PRIx64"\n", wbCacheLine->getBaseAddr());
        retryRequestLater(event, requestBaseAddr);
        throw stallException();
    }
    else if(isCandidateInTransition(wbCacheLine)){
        mshr_->insertPointer(wbCacheLine->getBaseAddr(), requestBaseAddr);
        throw stallException();
    }
    return;
}

/* If Candidate is in transition then we cannot writeback cacheline, postpone current request */
bool Cache::isCandidateInTransition(CacheLine* wbCacheLine){
    CCLine* wbCCLine = getCCLine(wbCacheLine->index());
    if(CacheLine::inTransition(wbCacheLine->getState()) || (!L1_ && wbCCLine->getState() != V)){
        d_->debug(_L1_,"Stalling request: Replacement cache line in transition.\n");
        return true;
    }
    return false;
}


/* ---------------------------------------
   Invalidate Request helper functions
   --------------------------------------- */
bool Cache::invalidatesInProgress(int lineIndex){
    if(L1_) return false;
    
    CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[lineIndex];
    if(ccLine->inTransition()){
        d_->debug(_L1_,"Invalidate request needs to be forwared to HiLv caches.\n");
        return true;
    }
    return false;
}

bool Cache::shouldThisInvalidateRequestProceed(MemEvent *event, CacheLine* cacheLine, Addr baseAddr, bool reActivation){
    /* Scenario where this 'if' occurs:  HiLv$ evicts a shared line (S->I), sends PutS to LowLv$.
       Simultaneously, LowLv$ sends an Inv to HiLv$. Thus, HiLv$ sends an Inv an already invalidated line */
    if(!cacheLine || (cacheLine->getState() == I && !reActivation)){
        mshr_->removeElement(baseAddr, event);
        d_->debug(_WARNING_,"Warning: Ignoring request. CLine doesn't exists or invalid.\n");
        return false;
    }
   
    if(cacheLine->isLockedByUser()){        /* If user-locked then wait this lock is released to activate this event. */
        STAT_InvalidateWaitingForUserLock_++;
        cacheLine->eventsWaitingForLock_ = true;
        d_->debug(_WARNING_,"Warning:  CLine is user-locked.\n");
        return false;
    }
    
    if(!L1_){
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[cacheLine->index()];
        if(ccLine->getState() == V) assert(ccLine->getAckCount() == 0);
        if(ccLine->getState() != V) return false;
    }
    d_->debug(_L1_,"Invalidate request is valid. State: %s\n", BccLineString[cacheLine->getState()]);
    return true;
}

/* -------------------------------------------------------------------------------------
            Helper Functions
 ------------------------------------------------------------------------------------- */

/* Function attempts to send all responses for previous events that 'blocked' due to an outstanding request.
   If response blocks cache line the remaining responses go to MSHR till new outstanding request finishes,
 */
void Cache::activatePrevEvents(Addr baseAddr){
    if(!mshr_->isHit(baseAddr)) return;
    vector<mshrType*> mshrEntry = mshr_->removeAll(baseAddr); assert(!mshr_->isHit(baseAddr));
    bool cont;    int i = 0;
    d_->debug(_L1_,"---------start--------- Size: %lu\n", mshrEntry.size());
    
    for(vector<mshrType*>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
        if((*it)->elem.type() == typeid(Addr)){                             /* Pointer Type */
            Addr pointerAddr = boost::get<Addr>((*it)->elem);  
            d_->debug(_L3_,"Pointer Addr: %"PRIx64"\n", pointerAddr);
            if(!mshr_->isHit(pointerAddr)){ mshrEntry.erase(it); continue;} /* Entry has been already been processed */
            vector<mshrType*> pointerMSHR = mshr_->removeAll(pointerAddr);

            for(vector<mshrType*>::iterator it2 = pointerMSHR.begin(); it2 != pointerMSHR.end(); i++){
                assert((*it2)->elem.type() == typeid(MemEvent*));
                cont = activatePrevEvent(boost::get<MemEvent*>((*it2)->elem), pointerMSHR, pointerAddr, it2, i);
                if(!cont) break;
            }
            mshrEntry.erase(it);
        }
        else{                                                                /* MemEvent Type */
            cont = activatePrevEvent(boost::get<MemEvent*>((*it)->elem), mshrEntry, baseAddr, it, i);
            if(!cont) break;
        }
    }
    d_->debug(_L1_,"---------end---------\n");
}


bool Cache::activatePrevEvent(MemEvent* event, vector<mshrType*>& mshrEntry, Addr addr, vector<mshrType*>::iterator it, int i){
    d_->debug(_L1_,"Activating event #%i, cmd = %s, bsAddr: %"PRIx64", addr: %"PRIx64", dst: %s\n", i, CommandString[event->getCmd()], toBaseAddr(event->getAddr()), event->getAddr(), event->getDst().c_str());
    d_->debug(_L1_,"--------------------------------------\n");
    this->processEvent(event, true);
    d_->debug(_L1_,"--------------------------------------\n");
    mshrEntry.erase(it);

    if(mshr_->isHit(addr)){
        try{ mshr_->insertAll(addr, mshrEntry); }
        catch(mshrException const& e){
            _abort(Cache, "MSHR Should not overflow when processing previous events\n");
        }
        return false;
    }
    return true;
}

void Cache::postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool requestCompleted, bool reActivation){
    Command cmd = event->getCmd();
    Addr baseAddr = cacheLine->getBaseAddr();
    if(requestCompleted){
        mshr_->removeElement(baseAddr, event);
        if(cmd == PutM || cmd == PutE){
            if(!L1_){                  /* Check if topCC line is locked */
                CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[cacheLine->index()];
                if(cacheLine->unlocked() && ccLine->isValid() && !reActivation) activatePrevEvents(baseAddr);
            }
        }
            //processInvalidateAcknowledge(event, baseAddr, reActivation); /*PutM also functions as an Inv Ack */
        reActivateEventWaitingForUserLock(cacheLine, reActivation);
        delete event;
    }
}

/* if cache line was user-locked, then events might be waiting for lock to be released and need to be reactivated */
void Cache::reActivateEventWaitingForUserLock(CacheLine* cacheLine, bool reActivation){
    Addr baseAddr = cacheLine->getBaseAddr();
    if(cacheLine->eventsWaitingForLock_ && !cacheLine->isLockedByUser() && !reActivation){
        d_->debug(_L1_,"Reactivating events:  User-locked cache line is now available\n");
        if(mshr_->isHit(baseAddr)) activatePrevEvents(baseAddr);
        cacheLine->eventsWaitingForLock_ = false;
    }
}

bool Cache::isCacheLineStable(CacheLine* _cacheLine, Command _cmd){
    bool stable = true;
    
    if(_cacheLine->inTransition()){ /* Check if cache line is locked */
        d_->debug(_L1_,"Stalling request: Cache line in transition. BccSt: %s\n", BccLineString[_cacheLine->getState()]);
        stable = false;
    }
    else if(!L1_){                  /* Check if topCC line is locked */
        CCLine* ccLine = ((MESITopCC*)topCC_)->ccLines_[_cacheLine->index()];
        assert(ccLine);
        if(ccLine->inTransition() && _cmd != PutM && _cmd != PutE && _cmd != PutS){
            d_->debug(_L1_,"Stalling request: Cache line in transition. TccSt: %s\n", TccLineString[ccLine->getState()]);
            stable = false;
        }
    }
    return stable;
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

void Cache::retryRequestLater(MemEvent* event, Addr baseAddr){
    mshr_->removeElement(baseAddr, event);
    retryQueueNext_.push_back(event);
}

CacheArray::CacheLine* Cache::getCacheLine(Addr baseAddr){
    int lineIndex =  cArray_->find(baseAddr, false);
    if(lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cArray_->lines_[lineIndex];
        cacheLine->setIndex(lineIndex);
        return cacheLine;
    }
}

CacheArray::CacheLine* Cache::getCacheLine(int lineIndex){
    if(lineIndex == -1) return NULL;
    else{
        CacheLine* cacheLine = cArray_->lines_[lineIndex];
        cacheLine->setIndex(lineIndex);
        return cacheLine;
    }
}

int Cache::getParentId(MemEvent* event){
    return 0;
}

int Cache::getParentId(Addr baseAddr){
    return 0;
}

bool Cache::isCacheLineAllocated(int lineIndex){
    return (lineIndex == -1) ? false : true;
}

bool Cache::isCacheMiss(int lineIndex){
    if(lineIndex == -1){
        d_->debug(_L1_,"-- Cache Miss --\n");
        return true;
    }
    else return false;
}


TopCacheController::CCLine* Cache::getCCLine(int index){
    if(!L1_) return ((MESITopCC*)topCC_)->ccLines_[index];
    else return NULL;

}

bool Cache::checkRequestValidity(MemEvent* event, Addr baseAddr) throw(stallException){
    Command cmd = event->getCmd();
    assert(cmd != PutM && cmd != PutE);
    if(cmd == PutS) {
        d_->debug(_WARNING_,"Ignoring PutS request. \n");
        mshr_->removeElement(baseAddr, event);
        throw stallException();
    }
    return true;
}


bool operator== ( const mshrType& n1, const mshrType& n2) {
    if(n1.elem.type() == typeid(Addr)) return false;
    return(boost::get<MemEvent*>(n1.elem) == boost::get<MemEvent*>(n2.elem));
}
