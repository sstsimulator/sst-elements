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
 * File:   cache.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include <boost/assert.hpp>
#include <queue>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "cacheArray.h"
#include "replacementManager.h"
#include "coherenceControllers.h"
#include "util.h"
#include "cacheListener.h"
#include "memNIC.h"
#include <boost/assert.hpp>

#define assert_msg BOOST_ASSERT_MSG

namespace SST { namespace MemHierarchy {

using namespace std;


class Cache : public SST::Component {
public:


    
    typedef CacheArray::CacheLine CacheLine;
    typedef TopCacheController::CCLine CCLine;
    typedef map<Addr, vector<mshrType> > mshrTable;
    typedef unsigned int uint;
    typedef long long unsigned int uint64;

    class mshrException : public exception{
        const char* what () const throw (){ return "Memory requests needs to be NACKed. MSHR is full\n"; }
    };
    
    class MSHR{
    public:
        mshrTable map_ ;
        Cache* cache_;
        int size_;
        int maxSize_;
        
        MSHR(Cache*, int);
        void insertFront(Addr baseAddr, MemEvent* event);
        bool insertAll(Addr, vector<mshrType>) throw (mshrException);
        bool insert(Addr baseAddr, MemEvent* event);
        bool insertPointer(Addr keyAddr, Addr pointerAddr);
        bool insert(Addr baseAddr, Addr pointer);
        bool insert(Addr baseAddr, mshrType mshrEntry) throw (mshrException);
        void removeElement(Addr baseAddr, MemEvent* event);
        void removeElement(Addr baseAddr, Addr pointer);
        void removeElement(Addr baseAddr, mshrType mshrEntry);
        vector<mshrType> removeAll(Addr);
        const vector<mshrType> lookup(Addr baseAddr);
        bool isHit(Addr baseAddr);
        bool isHitAndStallNeeded(Addr baseAddr, Command cmd);
        uint getSize(){ return size_; }
        void printEntry(Addr baseAddr);
        void printEntry2(vector<MemEvent*> events);
    };
    

    virtual void init(unsigned int);
    virtual void setup(void);
    virtual void finish(void){
        bottomCC_->printStats(stats_, STAT_GetSExReceived_, STAT_InvalidateWaitingForUserLock_, STAT_TotalInstructionsRecieved_, STAT_NonCoherenceReqsReceived_, STAT_TotalMSHRHits_);
        topCC_->printStats(stats_);
        listener_->printStats(*d_);
        delete cArray_;
        delete replacementMgr_;
        delete d_;
        retryQueue_.clear();
        retryQueueNext_.clear();
        linkIdMap_.clear();
        nameMap_.clear();
    }
    
    static Cache* cacheFactory(SST::ComponentId_t id, SST::Params& params);
    Addr toBaseAddr(Addr addr){
        Addr baseAddr = (addr) & ~(cArray_->getLineSize() - 1);
        return baseAddr;
    }
    
    class stallException : public exception{
        const char* what () const throw (){ return "Memory requests needs to 'stall'. Request will be processed at a later time\n"; }
    };

    class ignoreEventException : public exception{
        const char* what () const throw (){ return "Memory requests needs to be ignored. Request is irrelevant or out-of-date\n"; }
    };


private:

    /** Constructor for Cache Component */
    Cache(SST::ComponentId_t id, SST::Params& params, string _cacheFrequency, CacheArray* _cacheArray, uint _protocol, 
           Output* _d, LRUReplacementMgr* _rm, uint _numLines, uint lineSize, uint MSHRSize, bool _L1, bool _dirControllerExists);


    uint                    ID_;
    CacheArray*             cArray_;
    CacheListener*          listener_;
    uint                    protocol_;
    vector<Link*>*          lowNetPorts_;
    vector<Link*>*          highNetPorts_;
    Link*                   selfLink_;
    MemNIC*                 directoryLink_;
    Output*                 d_;
    Output*                 d2_;
    LRUReplacementMgr*      replacementMgr_;
    uint                    numLines_;
    uint                    lineSize_;
    uint                    MSHRSize_;
    string                  nextLevelCacheName_;
    bool                    L1_;
    bool                    dirControllerExists_;
    MSHR*                   mshr_;
    MSHR*                   mshrUncached_;
    TopCacheController*     topCC_;
    MESIBottomCC*           bottomCC_;
    bool                    sharersAware_;
    vector<MemEvent*>       retryQueue_;
    vector<MemEvent*>       retryQueueNext_;
    queue<pair<SST::Event*, uint64> >   incomingEventQueue_;
    uint64                  accessLatency_;
    uint64                  mshrLatency_;
    uint64                  STAT_GetSExReceived_;
    uint64                  STAT_InvalidateWaitingForUserLock_;
    uint64                  STAT_TotalInstructionsRecieved_;
    uint64                  STAT_NonCoherenceReqsReceived_;
    uint64                  STAT_TotalMSHRHits_;
    uint64                  timestamp_;
    int                     stats_;
    int                     idleMax_;
 	std::map<string, LinkId_t>     nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;
    
    int idleCount_;
    bool memNICIdle_;
    int memNICIdleCount_;
    bool clockOn_;
    Clock::Handler<Cache>* clockHandler_;
    TimeConverter* defaultTimeBase_;
    
    /** Handler for incoming link events.  Add incoming event to 'incoming event queue'. */
    void processIncomingEvent(SST::Event *event);
    
    /** Process the oldest incoming 'uncached' event */
    void processUncached(MemEvent* event, Command cmd, Addr baseAddr);
    
    /** Process the oldest incoming event */
    void processEvent(SST::Event* ev, bool mshrHit);
    
    /** Configure this component's links */
    void configureLinks();
    
    /** Handler for incoming prefetching events. */
    void handlePrefetchEvent(SST::Event *event);
    
    /** Self-Event handler for this component */
    void handleSelfEvent(SST::Event *event);
    
    /** Function processes incomming access requests from HiLv$ or the CPU
        It appropriately redirects requests to Top and/or Bottom controllers.  */
    void processCacheRequest(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);
    
    /** Function processes incomming invalidate messages.  Redirects message 
        to Top and Bottom controllers appropriately  */
    void processCacheInvalidate(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);

    /** Function processes incomming GetS/GetX responses.  
        Redirects message to Top Controller */
    void processCacheResponse(MemEvent* ackEvent, Addr baseAddr);

    /** Function processes incomming Fetch or FetchInvalidate requests from the Directory Controller
        Fetches send data, while FetchInvalidates evict data to the directory controller */
    void processFetch(MemEvent* event, Addr baseAddr, bool mshrHit);

    /** Find replacement for the current request.  If the replacement candidate is
        valid then a writeback is needed.  If replacemenent candidate is transitioning, we 
        need to wait (stall) until the replacement is in a 'stable' state */
    inline void allocateCacheLine(MemEvent *event, Addr baseAddr, int& lineIndex) throw(stallException);

    /** Depending on the replacement policy and cache array type, this function appropriately
        searches for the replacement candidate */
    inline CacheLine* findReplacementCacheLine(Addr baseAddr);

    /** Check that the selected replacement candidate can actually be replaced.
        The cacheline could be 'user-locked' for atomicity or the cacheline could be in transition */
    inline void candidacyCheck(MemEvent* event, CacheLine* wbCacheLine, Addr requestBaseAddr) throw(stallException);

    /** Evict replacement cache line in higher level caches (if necessary).
        TopCC sends invalidates to lower level caches; stall if invalidates were sent */
    inline void evictInHigherLevelCaches(CacheLine* wbCacheLine, Addr requestBaseAddr) throw (stallException);

    /** Writeback cache line to lower level caches */
    inline void writebackToLowerLevelCaches(MemEvent *event, CacheLine* wbCacheLine);

    /** At this point, cache line has been evicted or is not valid. 
        This function replaces cache line with the info/addr of the current request */
    inline void replaceCacheLine(int replacementCacheLineIndex, int& newCacheLineIndex, Addr newBaseAddr);

    /** Check whether or not replacement candidate is in transition
       If so, we cannot writeback cacheline, postpone current request */
    inline bool isCandidateInTransition(CacheLine* wbCacheLine);

    /** Check if invalidates are in progress */
    bool invalidatesInProgress(int lineIndex);

    /* Invalidate was received. This function checks wheter this invalidate request can proceed.
       This function prevents deadlocks by giving priority to requests in progress */
    bool shouldInvRequestProceed(MemEvent *event, CacheLine* cacheLine, Addr baseAddr, bool mshrHit);

    /** Function attempts to send all responses for previous events that 'blocked' due to an outstanding request.
        If response blocks cache line the remaining responses go to MSHR till new outstanding request finishes  */
    inline void activatePrevEvents(Addr baseAddr);

    /** This function re-processes a signle previous request.  In hardware, the MSHR would be looked up,
        the MSHR entry would be modified and the response would be sent directly without reading the cache
        array and tag.  In SST, we just rerun the request to avoid complexity */
    inline bool activatePrevEvent(MemEvent* event, vector<mshrType>& mshrEntry, Addr addr, vector<mshrType>::iterator it, int i);

    inline void postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool requestCompleted, bool mshrHit) throw(stallException);

    /** If cache line was user-locked, then events might be waiting for lock to be released
        and need to be reactivated */
    inline void reActivateEventWaitingForUserLock(CacheLine* cacheLine, bool mshrHit);

    /** Check if the cacheline and the 'CCLine' are in a stable state */
    void checkCacheLineIsStable(CacheLine* cacheLine, Command cmd) throw (ignoreEventException);

    /** Check if there a cache miss */
    inline bool isCacheMiss(int lineIndex);

    /** Find cache line by base addr */
    inline CacheLine* getCacheLine(Addr baseAddr);
    
    /** Find cache line by line index */
    inline CacheLine* getCacheLine(int lineIndex);

    /** Simply checks if line index equals -1 */
    inline bool isCacheLineAllocated(int lineIndex);

    /** Get Cache Coherency line */
    inline TopCacheController::CCLine* getCCLine(int index);

    /** Check the 'validity' of the request.  Assert expected state */
    inline void checkRequestValidity(MemEvent* event) throw(ignoreEventException);

    /** Verify that input parameters are valid */
    void errorChecking();
    
    /** Print input members/parameters */
    void pMembers();
    
    /** Find out if number is a power of 2 */
    bool isPowerOfTwo(uint x){ return (x & (x - 1)) == 0; }

    /** Find the appropriate MSHR lookup latency cycles in case the user did not provide
        any parameter values for this type of latency.  This function intrapolates from 
        numbers gather by other papers/results */
    void intrapolateMSHRLatency();

    /** Add requests to the 'retry queue.'  This event will be reissued at a later time */
    inline void retryRequestLater(MemEvent* event);


    /**  Clock Handler.  Every cycle events are executed (if any).  If clock is idle long enough, 
         the clock gets deregested from TimeVortx and reregisted only when an event is received */
    bool clockTick(Cycle_t time) {
        timestamp_++; topCC_->timestamp_++; bottomCC_->timestamp_++;
        bool ret = false;
        
        if(dirControllerExists_) memNICIdle_ = directoryLink_->clock();
        
        bool topCCBusy      = topCC_->queueBusy();
        bool bottomCCBusy   = bottomCC_->queueBusy();
        bool cacheBusy      = !incomingEventQueue_.empty();
        
        if(topCCBusy)     topCC_->sendOutgoingCommands();
        if(bottomCCBusy)  bottomCC_->sendOutgoingCommands();

        if(cacheBusy) processQueueRequest();
        else if(UNLIKELY(!retryQueueNext_.empty())) retryEvent();
        else if(!topCCBusy && !bottomCCBusy && !dirControllerExists_)
            ret = incIdleCount();
        
        return ret;
    }
    
    /** Process the request on the top of the incoming event queue */
    void processQueueRequest(){
        processEvent(incomingEventQueue_.front().first, false);
        incomingEventQueue_.pop();
        idleCount_ = 0;
    }
    
    /** Retry an event in the 'retry queue' */
    void retryEvent(){
        retryQueue_ = retryQueueNext_;
        retryQueueNext_.clear();
        for(vector<MemEvent*>::iterator it = retryQueue_.begin(); it != retryQueue_.end();){
            d_->debug(_L1_,"Retrying event\n");
            processEvent(*it, true);
            retryQueue_.erase(it);
        }
        idleCount_ = 0;
    }
    
    /** Increment idle clock count */
    bool incIdleCount(){
        idleCount_++;
        if(!dirControllerExists_ && idleCount_ > idleMax_){
            clockOn_ = false;
            idleCount_ = 0;
            return true;
        }
        return false;
    }

};


}}

#endif
