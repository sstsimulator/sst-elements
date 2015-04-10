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
#include "mshr.h"
#include "replacementManager.h"
#include "coherenceControllers.h"
#include "util.h"
#include "cacheListener.h"
#include "memNIC.h"
#include <boost/assert.hpp>
#include <string>
#include <sstream>

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
    class blockedEventException : public exception{ const char* what () const throw (){ return "Memory requests needs to 'stall'. Request will be processed at a later time\n"; } };
    
    /** Ignore Event Exception.  Exception thrown when the event received is considered to be irrelevant (ie Invalidate received when cacheline was alread invalid (due to eviction) */
    class ignoreEventException : public exception{ const char* what () const throw (){ return "Memory requests needs to be ignored. Request is irrelevant or out-of-date\n"; }};

private:
    struct CacheConfig;
    
    /** Constructor for Cache Component */
    Cache(ComponentId_t _id, Params &_params, CacheConfig _config);
    
    /** Handler for incoming link events.  Add incoming event to 'incoming event queue'. */
    void processIncomingEvent(SST::Event *event);
    
    /** Process the oldest incoming 'noncacheable' event */
    void processNoncacheable(MemEvent* event, Command cmd, Addr baseAddr);
    
    /** Process the oldest incoming event */
    void processEvent(MemEvent* event, bool mshrHit);
    
    /** Configure this component's links */
    void configureLinks();
    
    /** Handler for incoming prefetching events. */
    void handlePrefetchEvent(SST::Event *event);
    
    /** Self-Event handler for this component */
    void handleSelfEvent(SST::Event *event);
    
    /** Function processes incomming access requests from HiLv$ or the CPU
        It appropriately redirects requests to Top and/or Bottom controllers.  */
    void processCacheRequest(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);
    void processCacheReplacement(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);
    
    /** Function processes incomming invalidate messages.  Redirects message 
        to Top and Bottom controllers appropriately  */
    void processCacheInvalidate(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);

    /** Function processes incomming GetS/GetX responses.  
        Redirects message to Top Controller */
    void processCacheResponse(MemEvent* ackEvent, Addr baseAddr);
    
    void processFetchResp(MemEvent* event, Addr baseAddr);

    /** Function processes incomming Fetch invalidate requests from the Directory Controller
        Fetches send data, while FetchInvalidates evict data to the directory controller */
    void processFetch(MemEvent* event, Addr baseAddr, bool mshrHit);

    /** Find replacement for the current request.  If the replacement candidate is
        valid then a writeback is needed.  If replacemenent candidate is transitioning, we 
        need to wait (stall) until the replacement is in a 'stable' state */
    inline void allocateCacheLine(MemEvent *event, Addr baseAddr, int& lineIndex) throw(blockedEventException);

    /** Depending on the replacement policy and cache array type, this function appropriately
        searches for the replacement candidate */
    inline CacheLine* findReplacementCacheLine(Addr baseAddr);

    /** Check that the selected replacement candidate can actually be replaced.
        The cacheline could be 'user-locked' for atomicity or the cacheline could be in transition */
    inline void candidacyCheck(MemEvent* event, CacheLine* wbCacheLine, Addr requestBaseAddr) throw(blockedEventException);

    /** Evict replacement cache line in higher level caches (if necessary).
        TopCC sends invalidates to lower level caches; stall if invalidates were sent */
    inline void evictInHigherLevelCaches(CacheLine* wbCacheLine, Addr requestBaseAddr) throw (blockedEventException);

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

    inline void postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool requestCompleted, bool mshrHit) throw(blockedEventException);

    /** If cache line was user-locked, then events might be waiting for lock to be released
        and need to be reactivated */
    inline void reActivateEventWaitingForUserLock(CacheLine* cacheLine);

    /** Most requests are stalled in the MSHR if the cache line is 'blocking'.  However, this does not happen for
        writebacks.  This funciton handles the case where the cache line is 'blocking' and an incomming PutS request is received. */
    void handleIgnorableRequests(MemEvent* _event, CacheLine* cacheLine, Command cmd) throw (ignoreEventException);

    /** After BCC is executed, this function checks if an upgrade request was sent to LwLv caches.  If so, this request
        needs to stall */
    void stallIfUpgradeInProgress(CacheLine* _cacheLine) throw(blockedEventException);

    /** Check if there a cache miss */
    inline bool isCacheMiss(int lineIndex);

    /** Find cache line by base addr */
    inline CacheLine* getCacheLine(Addr baseAddr);
    
    /** Find cache line by line index */
    inline CacheLine* getCacheLine(int lineIndex);

    /** Get Cache Coherency line */
    inline TopCacheController::CCLine* getCCLine(int index);

    /** Make sure that this request is not a dirty writeback.  Cache miss cannot occur on a dirty writeback  */
    inline void checkCacheMissValidity(MemEvent* event) throw(ignoreEventException);
    
    /** Check whether this request will hit or miss in the cache - including correct coherence permission */
    int isCacheHit(MemEvent* _event, Command _cmd, Addr _baseAddr);

    /** Insert to MSHR wrapper */
    inline bool insertToMSHR(Addr baseAddr, MemEvent* event);
    
    /** Try to insert request to MSHR.  If not sucessful, function send a NACK to requestor */
    bool processRequestInMSHR(Addr baseAddr, MemEvent* event);
    bool processInvRequestInMSHR(Addr baseAddr, MemEvent* event);
    
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
    void updateUpgradeLatencyAverage(SimTime_t start);
    void recordLatency(MemEvent * event);

    /** Get the front elevemnt of a MSHR entry */
    MemEvent* getOrigReq(const vector<mshrType> _mshrEntry);
    
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
    void profileEvent(MemEvent* event, Command cmd, bool mshrHit);
    void incTotalRequestsReceived(int _groupId);
    void incTotalMSHRHits(int _groupId);
    void incInvalidateWaitingForUserLock(int _groupId);
    int groupId;

    /**  Clock Handler.  Every cycle events are executed (if any).  If clock is idle long enough, 
         the clock gets deregistered from TimeVortx and reregistered only when an event is received */
    bool clockTick(Cycle_t time) {
        timestamp_++;
        if(cf_.bottomNetwork_ != "") memNICIdle_ = bottomNetworkLink_->clock();
        topCC_->sendOutgoingCommands(getCurrentSimTimeNano());
        bottomCC_->sendOutgoingCommands(getCurrentSimTimeNano());
        if ( cf_.maxWaitTime > 0 ) {
            checkMaxWait();
        }
        return false;
    }
    
    /** Increment idle clock count */
    bool incIdleCount(){
        idleCount_++;
        if(cf_.bottomNetwork_ == "" && idleCount_ > idleMax_){
            clockOn_ = false;
            idleCount_ = 0;
            return true;
        }
        return false;
    }

    void checkMaxWait(void) const {
        SimTime_t curTime = getCurrentSimTimeNano();
        MemEvent *oldReq = NULL;
        MemEvent *oldCacheReq = mshr_->getOldestRequest();
        MemEvent *oldUnCacheReq = mshrNoncacheable_->getOldestRequest();

        if ( oldCacheReq && oldUnCacheReq ) {
            oldReq = (oldCacheReq->getInitializationTime() < oldUnCacheReq->getInitializationTime()) ? oldCacheReq : oldUnCacheReq;
        } else if ( oldCacheReq ) {
            oldReq = oldCacheReq;
        } else {
            oldReq = oldUnCacheReq;
        }

        if ( oldReq ) {
            SimTime_t waitTime = curTime - oldReq->getInitializationTime();
            if ( waitTime > cf_.maxWaitTime ) {
                d_->fatal(CALL_INFO, 1, "%s, Error: Maximum Cache Request time reached!\n"
                        "Event: %s 0x%" PRIx64 " from %s. Time = %" PRIu64 " ns\n",
                        getName().c_str(), CommandString[oldReq->getCmd()], oldReq->getAddr(), oldReq->getSrc().c_str(), curTime);
            }
        }
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
        string bottomNetwork_;
        string topNetwork_;
        vector<int> statGroupIds_;
        bool allNoncacheableRequests_;
        SimTime_t maxWaitTime;
        bool upperSliced;
    };
    
    CacheConfig             cf_;
    uint                    ID_;
    CacheListener*          listener_;
    vector<Link*>*          lowNetPorts_;
    vector<Link*>*          highNetPorts_;
    Link*                   selfLink_;
    MemNIC*                 bottomNetworkLink_;
    MemNIC*                 topNetworkLink_;
    Output*                 d_;
    Output*                 d2_;
    vector<string>*         nextLevelCacheNames_;
    bool                    L1_;
    MSHR*                   mshr_;
    MSHR*                   mshrNoncacheable_;
    TopCacheController*     topCC_;
    MESIBottomCC*           bottomCC_;
    bool                    sharersAware_;
    vector<MemEvent*>       retryQueue_;
    vector<MemEvent*>       retryQueueNext_;
    queue<pair<SST::Event*, uint64> >   incomingEventQueue_;
    uint64                  accessLatency_;
    uint64                  tagLatency_;
    uint64                  mshrLatency_;
    uint64                  timestamp_;
    int                     statsFile_;
    int                     idleMax_;
    int                     idleCount_;
    bool                    memNICIdle_;
    int                     memNICIdleCount_;
    bool                    clockOn_;
    Clock::Handler<Cache>*  clockHandler_;
    TimeConverter*          defaultTimeBase_;
    std::map<string, LinkId_t>     nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;
    std::map<MemEvent*,uint64> startTimeList;
    std::map<MemEvent*,int> missTypeList;
    
    /* Profiling */
    bool                    groupStats_;
    map<int,CtrlStats>      stats_;
    uint64                  totalUpgradeLatency_;     //Latency for upgrade outstanding requests
    uint64                  totalLatency_;            //Latency for ALL outstanding requrests (Upgrades, Inv, etc)
    uint64                  upgradeCount_;
    uint64                  missLatency_GetS_IS;
    uint64                  missLatency_GetS_M;
    uint64                  missLatency_GetX_IM;
    uint64                  missLatency_GetX_SM;
    uint64                  missLatency_GetX_M;
    uint64                  missLatency_GetSEx_IM;
    uint64                  missLatency_GetSEx_SM;
    uint64                  missLatency_GetSEx_M;
    
    /* 
     * Statistics API stats  - 
     * Note: these duplicate some of the existing stats that are output 
     * at the end of the run with the cache option "statistics". 
     * Eventually the two statistics will merge. 
     */
    // Cache hits
    Statistic<uint64_t>* statCacheHits;
    Statistic<uint64_t>* statGetSHitOnArrival;
    Statistic<uint64_t>* statGetXHitOnArrival;
    Statistic<uint64_t>* statGetSExHitOnArrival;
    Statistic<uint64_t>* statGetSHitAfterBlocked;
    Statistic<uint64_t>* statGetXHitAfterBlocked;
    Statistic<uint64_t>* statGetSExHitAfterBlocked;
    // Cache misses
    Statistic<uint64_t>* statCacheMisses;
    Statistic<uint64_t>* statGetSMissOnArrival;
    Statistic<uint64_t>* statGetXMissOnArrival;
    Statistic<uint64_t>* statGetSExMissOnArrival;
    Statistic<uint64_t>* statGetSMissAfterBlocked;
    Statistic<uint64_t>* statGetXMissAfterBlocked;
    Statistic<uint64_t>* statGetSExMissAfterBlocked;
    // Events received
    Statistic<uint64_t>* statGetS_recv;
    Statistic<uint64_t>* statGetX_recv;
    Statistic<uint64_t>* statGetSEx_recv;
    Statistic<uint64_t>* statGetSResp_recv;
    Statistic<uint64_t>* statGetXResp_recv;
    Statistic<uint64_t>* statPutS_recv;
    Statistic<uint64_t>* statPutM_recv;
    Statistic<uint64_t>* statPutX_recv;
    Statistic<uint64_t>* statPutE_recv;
    Statistic<uint64_t>* statFetchInv_recv;
    Statistic<uint64_t>* statFetchInvX_recv;
    Statistic<uint64_t>* statInv_recv;
    Statistic<uint64_t>* statNACK_recv;
};

/*  Implementation Details
    The coherence protocol implemented by MemHierarchy's 'Cache' component is a directory-based intra-node MESI/MSI coherency
    similar to modern processors like Intel sandy bridge and ARM CCI.  Intra-node Directory-based protocols.
    The class "Directory Controller" implements directory-based inter-node coherence and needs to 
    be used along "Merlin", our interconnet network simulator (Contact Scott Hemmert about Merlin).
 
    The Cache class serves as the main cache controller.  It is in charge or handling incoming
    SST-based events (cacheEventProcessing.cc) and forwarding the requests to the other system's
    subcomponents:
        - Cache Array:  Class in charge keeping track of all the cache lines in the cache.  The 
        Cache Line inner class stores the data and state related to a particular cache line.
 
        - Replacement Manager:  Class handles work related to the replacement policy of the cache.  
        Similar to Cache Array, this class is OO-based so different replacement policies are simply
        subclasses of the main based abstrac class (ReplacementMgr), therefore they need to implement 
        certain functions in order for them to work properly. Implemented policies are: least-recently-used (lru), 
        least-frequently-used (lfu), most-recently-used (mru), random, and not-most-recently-used (nmru).
 
        - Hash:  Class implements common hashing functions.  These functions are used by the Cache Array
        class.  For instance, a typical set associative array uses a simple hash function, whereas
        skewed associate and ZCaches use more advanced hashing functions.
 
        - MSHR:  Class that represents a hardware Miss Status Handling Register (or Miss Status Holding
        Register).  Noncacheable requests use a separate MSHR for simplicity.

        The MSHR is a map of <addr, vector<UNION(event, addr pointer)> >. 
        Why a UNION(event, addr pointer)?  Upon receiving a miss request, all cache line replacement candidates 
        might be in transition, regardless of the replacement policy and associativity used.  Instead
        of sending NACKS (which is inefficient), the MSHR stores a pointer in the MSHR entry.  When a 
        replacement candidate is NO longer, the MSHR looks at the pointer and 'replays' the
        previously blocked request, which actually is store in another MSHR entry (different address).
        
        Example:
            MSHR    Addr   Requests
                     A     #1, #2, pointer to B, #4
                     B     #3
                     
            In the above example, Request #3 (addr B) wanted to replace a cache line from Addr A.  Since
            the cache line containing A was in transition, a pointer is stored in the "A"-key MSHR entry.
            When A finishes (Request #1), then request #2, request #3 (from B), and request #4 are replayed 
            (as long as none of them further get blocked again for some reason).
 
 
    
    The cache itself is a "blocking" cache, which is the  type most often found in hardware.  A blocking
    cache does not respond to requests if the cache line is in transition.  Instead, it stores
    pending requests in the MSHR.  The MSHR is in charge of "replaying" pending requests once
    the cache line is in a stable state. 
 
    Coherence-related algorithms are broken down into a Top Cache Coherence (topCC) controller and a
    Bottom Cache Coherence (bottomCC) controller.  As the name suggests,  the topCC handles
    requests sent to the higher level caches (closer to CPU), which could be invalidates, and
    responses.  Whereas bottomCC deals with "Cache Lines" to store state and data information, topCC
    deals with "CCLine" and stores coherence state, mainly sharers, owners, and atomic-related
    information.  BottomCC handles upgrades, and makes sure a request only proceeds if the cache holds
    the right state.  It also sends responses if the request came from the directory controller.
    
    Prefetchers are part of SST's Cassini element, not part of MemHierarchy.  Prefetchers can be used along
    any level cache. Prefetch does not currently work alongside a shared, sliced caches.
    
 
    Key notes:
        - MemHierarchy's CClines always have deterministic information about the sharers.
        
        - In case of deadlock situations, the lower level cache has priority.  For instance,
        if an L2 sends an invalidate to an L1, while at the same time an L1 sends an upgrade request to an L2, the
        L2 has priority.  The L1 request is stored in the L2 MSHR to be handled at a later time.
        
        - The cache supports hardware "locking" (GetSEx command), and atomics-based requests (LLSC, GetS with 
        LLSC flag on).  For LLSC atomics, the flag "LLSC_Resp" is set when a store (GetX) was successful.
        
        - In case an L1's cache line is "LOCKED" (L2+ are never locked), the L1 is in charge of "replying" any
        events that might have been blocked due to that LOCK. These 'replayed' events should be invalidates, for 
        obvious reasons.
        
        - Access_latency_cycles determines the number of cycles it typically takes to complete a request, while
        MSHR_hit_latency determines the number of cycles is takes to respond to events that were pending in the MSHR.
        For instance, upon receiving a miss request, the cache takes access_latency to send that request to memory.
        Once a responce comes from Memory (or LwLvl caches), the MSHR is looked up and it takes "MSHR_hit_latency" to
        send that request to the higher level caches, instead of taking another "access_latency" cycles which would
        normally take quite longer.
        
        - To avoid deadlock, at least one MSHR is always reserved for requests from lower levels (e.g., Invs). 
        Therefore, each cache requires a minimum of two MSHRs (one for sending, one for receiving).

        - A "Bus" component is only needed between two caches when there is more than one higher level cache.
          Eg.  L1  L1 <-- bus --> L2.  When a single L1 connects directly to an L2, no bus is needed.
    
        - An L1 cache handles as many requests sent by the CPU per cycle.  However, only one request is sent
        back to the CPU per cycle (after access_latency_cycles in case of a hit).
 
        - Exceptions (eg. blockedEventException) are used mainly to avoid if-else statements for every value 
        returned in the cache controller helper functions. It creates cleaner code and performance is not 
        diminished because the exceptions used are not that common.
        IE.  Cache hits are more common than Cache misses (when blocked Event exceptions get thrown).
        
        - Class member variables have a suffix "_", while function parameters have it as a preffix.
        
        - Use a 'no wrapping' editor to view MH files, as many comments are on the 'side' and fall off the window 
 
        - TODO:
            - L2, and L3 should go to "sleep" (no clock tick handler) when they are idle.  This will improve
              SST performance.
            - Create a 'simplistic' memHierarchy model, one that does not measures contentions and sends requests
              through "links".  This simplistic model would not be as accurate but would be greatly helpful
              when simulatating hundreds of cores.  MemHierarchy is cycle-accurate and thus is slower than other
              implementations.
            - Directory-based intra-node MOESI protocol
 
    Questions: caesar.tx@gmail.com

*/



}}

#endif
