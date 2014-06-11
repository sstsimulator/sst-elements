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

    typedef CacheArray::CacheLine           CacheLine;
    typedef TopCacheController::CCLine      CCLine;
    typedef map<Addr, vector<mshrType> >    mshrTable;
    typedef unsigned int                    uint;
    typedef uint64_t                        uint64;

    friend class InstructionStream;
    
    /** MSHR full exception.  Catch block responsible for sending NACK */
    class mshrException : public exception{ const char* what () const throw (){ return "Memory requests needs to be NACKed. MSHR is full\n"; }};

    class MSHR{
    public:
        mshrTable   map_ ;
        Cache*      cache_;
        int         size_;
        int         maxSize_;
        
        MSHR(Cache*, int);
        bool exists(Addr baseAddr);
        void insertFront(Addr baseAddr, MemEvent* event);
        bool insertAll(Addr, vector<mshrType>);
        bool insert(Addr baseAddr, MemEvent* event);
        bool insertPointer(Addr keyAddr, Addr pointerAddr);
        bool insert(Addr baseAddr, Addr pointer);
        bool insert(Addr baseAddr, mshrType mshrEntry);
        MemEvent* removeFront(Addr _baseAddr);
        void removeElement(Addr baseAddr, MemEvent* event);
        void removeElement(Addr baseAddr, Addr pointer);
        void removeElement(Addr baseAddr, mshrType mshrEntry);
        vector<mshrType> removeAll(Addr);
        const vector<mshrType> lookup(Addr baseAddr);
        MemEvent* lookupFront(Addr _baseAddr);
        bool isHit(Addr baseAddr);
        bool isHitAndStallNeeded(Addr baseAddr, Command cmd);
        uint getSize(){ return size_; }
        void printEntry(Addr baseAddr);
        void printEntry2(vector<MemEvent*> events);
        bool isFull();
    };
    
    virtual void init(unsigned int);
    virtual void setup(void);
    virtual void finish(void);
    
    /** Creates cache componennt */
    static Cache* cacheFactory(SST::ComponentId_t id, SST::Params& params);
    
    /** Computes the 'Base Address' of the requests.  The base address point the first address of the cache line */
    Addr toBaseAddr(Addr addr){
        Addr baseAddr = (addr) & ~(cf_.cacheArray_->getLineSize() - 1);  //Remove the block offset bits
        return baseAddr;
    }
    
    /** Stall Exception.  Exception thrown when an events needs to stall due 
        to an upgrade needed, cache line being locked, etc */
    class stallException : public exception{ const char* what () const throw (){ return "Memory requests needs to 'stall'. Request will be processed at a later time\n"; } };
    
    /** Ignore Event Exception.  Exception thrown when the event received is considered to be irrelevant (ie Invalidate received when cacheline was alread invalid (due to eviction) */
    class ignoreEventException : public exception{ const char* what () const throw (){ return "Memory requests needs to be ignored. Request is irrelevant or out-of-date\n"; }};

private:
    struct CacheConfig;
    
    /** Constructor for Cache Component */
    Cache(ComponentId_t _id, Params &_params, CacheConfig _config);
    
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

    /** Function processes incomming Fetch invalidate requests from the Directory Controller
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

    /** Most requests are stalled in the MSHR if the cache line is 'blocking'.  However, this does not happen for
        writebacks.  This funciton handles the case where the cache line is 'blocking' and an incomming PutS request is received. */
    void handleIgnorableRequests(MemEvent* _event, CacheLine* cacheLine, Command cmd) throw (ignoreEventException);

    /** After BCC is executed, this function checks if an upgrade request was sent to LwLv caches.  If so, this request
        needs to stall */
    void stallIfUpgradeInProgress(CacheLine* _cacheLine) throw(stallException);

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

    /** Insert to MSHR wrapper */
    inline bool insertToMSHR(Addr baseAddr, MemEvent* event);
    
    /** Try to insert request to MSHR.  If not sucessful, function send a NACK to requestor */
    bool processRequestInMSHR(Addr baseAddr, MemEvent* event);
    
    /** Determines what CC will send the NACK. */
    void sendNACK(MemEvent* _event);

    /** In charge of processng incoming NACK.  
        Currently, it simply retries event */
    void processIncomingNACK(MemEvent* _origReqEvent);
    
    
    /** Verify that input parameters are valid */
    void errorChecking();
    
    /** Print input members/parameters */
    void pMembers();
    
    /** Udpate the upgrade latency stats */
    void updateUpgradeLatencyAverage(MemEvent* origMemEvent);
    
    /** Get the front elevemnt of a MSHR entry */
    MemEvent* getOriginalRequest(const vector<mshrType> _mshrEntry);
    
    /** Find out if number is a power of 2 */
    bool isPowerOfTwo(uint x){ return (x & (x - 1)) == 0; }
    
    /** Timestamp getter */
    uint64 getTimestamp(){ return timestamp_; }

    /** Find the appropriate MSHR lookup latency cycles in case the user did not provide
        any parameter values for this type of latency.  This function intrapolates from 
        numbers gather by other papers/results */
    void intrapolateMSHRLatency();

    /** Add requests to the 'retry queue.'  This event will be reissued at a later time */
    void retryRequestLater(MemEvent* event);

    void incTotalRequestsReceived(int _groupId);
    void incTotalMSHRHits(int _groupId);
    void incInvalidateWaitingForUserLock(int _groupId);
    int groupId;

    /**  Clock Handler.  Every cycle events are executed (if any).  If clock is idle long enough, 
         the clock gets deregested from TimeVortx and reregisted only when an event is received */
    bool clockTick(Cycle_t time) {
        timestamp_++; topCC_->timestamp_++; bottomCC_->timestamp_++;
        
        if(cf_.dirControllerExists_) memNICIdle_ = directoryLink_->clock();
        
        bool topCCBusy      = topCC_->queueBusy();
        bool bottomCCBusy   = bottomCC_->queueBusy();
        
        if(topCCBusy)     topCC_->sendOutgoingCommands();
        if(bottomCCBusy)  bottomCC_->sendOutgoingCommands();

        //if(cacheBusy) processQueueRequest();
        //else if(!topCCBusy && !bottomCCBusy && !dirControllerExists_)
        //    ret = incIdleCount();
        
        //return ret;
        return false;
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
            d_->debug(_L0_,"Retrying event\n");
            if(mshr_->isFull()){
                retryQueueNext_ = retryQueue_;
                return;
            }
            processEvent(*it, true);
            retryQueue_.erase(it);
        }
        idleCount_ = 0;
    }
    
    /** Increment idle clock count */
    bool incIdleCount(){
        idleCount_++;
        if(!cf_.dirControllerExists_ && idleCount_ > idleMax_){
            clockOn_ = false;
            idleCount_ = 0;
            return true;
        }
        return false;
    }
    
    struct CacheConfig{
        string cacheFrequency_;
        CacheArray* cacheArray_;
        uint protocol_;
        Output* dbg_;
        ReplacementMgr* rm_;
        uint numLines_;
        uint lineSize_;
        uint MSHRSize_;
        bool L1_;
        bool dirControllerExists_;
        vector<int> statGroupIds_;
        bool allUncachedRequests_;
    };
    
    CacheConfig             cf_;
    uint                    ID_;
    CacheListener*          listener_;
    vector<Link*>*          lowNetPorts_;
    vector<Link*>*          highNetPorts_;
    Link*                   selfLink_;
    MemNIC*                 directoryLink_;
    Output*                 d_;
    Output*                 d2_;
    string                  nextLevelCacheName_;
    bool                    L1_;
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
    uint64                  totalUpgradeLatency_;     //Latency for upgrade outstanding requests
    uint64                  totalLatency_;            //Latency for ALL outstanding requrests (Upgrades, Inv, etc)
    uint64                  mshrHits_;
    uint64                  upgradeCount_;
    uint64                  timestamp_;
    int                     statsFile_;
    bool                    groupStats_;
    map<int,CtrlStats>      stats_;
    int                     idleMax_;
    int                     idleCount_;
    bool                    memNICIdle_;
    int                     memNICIdleCount_;
    bool                    clockOn_;
    Clock::Handler<Cache>*  clockHandler_;
    TimeConverter*          defaultTimeBase_;
 	std::map<string, LinkId_t>     nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;


};


}}

#endif
