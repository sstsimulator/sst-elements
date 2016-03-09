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
    typedef CacheArray::DataLine            DataLine;
    typedef map<Addr, mshrEntry>            mshrTable;
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
        return (addr) & ~(cf_.cacheArray_->getLineSize() - 1);
    }

private:
    struct CacheConfig;
    
    /** Constructor for Cache Component */
    Cache(ComponentId_t id, Params &params, CacheConfig config);
    
    /** Handler for incoming link events.  Add incoming event to 'incoming event queue'. */
    void processIncomingEvent(SST::Event *event);
    
    /** Process the oldest incoming 'noncacheable' event */
    void processNoncacheable(MemEvent* event, Command cmd, Addr baseAddr);
    
    /** Process the oldest incoming event */
    void processEvent(MemEvent* event, bool mshrHit);
    
    /** Configure this component's links */
    void configureLinks(Params &params);
    
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
    void processCacheInvalidate(MemEvent *event, Addr baseAddr, bool mshrHit);

    /** Function processes incomming GetS/GetX responses.  
        Redirects message to Top Controller */
    void processCacheResponse(MemEvent* ackEvent, Addr baseAddr);
    
    void processFetchResp(MemEvent* event, Addr baseAddr);

    /** Find replacement for the current request.  If the replacement candidate is
        valid then a writeback is needed.  If replacemenent candidate is transitioning, we 
        need to wait (stall) until the replacement is in a 'stable' state */
    inline bool allocateLine(MemEvent *event, Addr baseAddr);
    inline bool allocateCacheLine(MemEvent *event, Addr baseAddr);
    inline bool allocateDirLine(MemEvent *event, Addr baseAddr);
    inline bool allocateDirCacheLine(MemEvent *event, Addr baseAddr, CacheLine * dirLine, bool noStall);

    /** Function attempts to send all responses for previous events that 'blocked' due to an outstanding request.
        If response blocks cache line the remaining responses go to MSHR till new outstanding request finishes  */
    inline void activatePrevEvents(Addr baseAddr);

    /** This function re-processes a signle previous request.  In hardware, the MSHR would be looked up,
        the MSHR entry would be modified and the response would be sent directly without reading the cache
        array and tag.  In SST, we just rerun the request to avoid complexity */
    inline bool activatePrevEvent(MemEvent* event, vector<mshrType>& entries, Addr addr, vector<mshrType>::iterator it, int i);

    inline void postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool mshrHit);
    
    inline void postReplacementProcessing(MemEvent* event, CacheAction action, bool mshrHit);

    /** If cache line was user-locked, then events might be waiting for lock to be released
        and need to be reactivated */
    inline void reActivateEventWaitingForUserLock(CacheLine* cacheLine);

    /** Check if there a cache miss */
    inline bool isCacheMiss(int lineIndex);

    /** Find cache line by base addr */
    inline CacheLine* getLine(Addr baseAddr);
    inline CacheLine* getCacheLine(Addr baseAddr);
    inline CacheLine* getDirLine(Addr baseAddr);
    
    /** Find cache line by line index */
    inline CacheLine* getLine(int lineIndex);
    inline CacheLine* getCacheLine(int lineIndex);
    inline CacheLine* getDirLine(int lineIndex);

    /** Check whether this request will hit or miss in the cache - including correct coherence permission */
    int isCacheHit(MemEvent* _event, Command _cmd, Addr _baseAddr);

    /** Insert to MSHR wrapper */
    inline bool insertToMSHR(Addr baseAddr, MemEvent* event);
    
    /** Try to insert request to MSHR.  If not sucessful, function send a NACK to requestor */
    bool processRequestInMSHR(Addr baseAddr, MemEvent* event);
    bool processInvRequestInMSHR(Addr baseAddr, MemEvent* event, bool inProgress);
    
    /** Determines what CC will send the NACK. */
    void sendNACK(MemEvent* _event);

    /** In charge of processng incoming NACK.  
        Currently, it simply retries event */
    void processIncomingNACK(MemEvent* _origReqEvent);
    
    
    /** Verify that input parameters are valid */
    void errorChecking();
    
    /** Print input members/parameters */
    void pMembers();
    
    /** Update the latency stats */
    void recordLatency(MemEvent * event);

    /** Get the front element of a MSHR entry */
    MemEvent* getOrigReq(const vector<mshrType> entries);
   
    /** Print cache line for debugging */
    void printLine(Addr addr);

    /** Find out if number is a power of 2 */
    bool isPowerOfTwo(uint x){ return (x & (x - 1)) == 0; }
    
    /** Timestamp getter */
    uint64 getTimestamp(){ return timestamp_; }

    /** Find the appropriate MSHR lookup latency cycles in case the user did not provide
        any parameter values for this type of latency.  This function intrapolates from 
        numbers gather by other papers/results */
    void intrapolateMSHRLatency();

    void profileEvent(MemEvent* event, Command cmd, bool replay, bool canStall);

    /**  Clock Handler.  Every cycle events are executed (if any).  If clock is idle long enough, 
         the clock gets deregistered from TimeVortx and reregistered only when an event is received */
    bool clockTick(Cycle_t time) {
        timestamp_++;
        bool queuesEmpty = coherenceMgr->sendOutgoingCommands(getCurrentSimTimeNano());
        
        bool nicIdle = true;
        if (bottomNetworkLink_) nicIdle = bottomNetworkLink_->clock();
        if (checkMaxWaitInterval_ > 0 && timestamp_ % checkMaxWaitInterval_ == 0) checkMaxWait();
        
        // MSHR occupancy
        statMSHROccupancy->addData(mshr_->getSize());
        
        // Disable lower-level cache clocks if they're idle
        if (queuesEmpty && nicIdle && clockIsOn_) {
            clockIsOn_ = false;
            lastActiveClockCycle_ = time;
            if (!maxWaitWakeupExists_) {
                maxWaitWakeupExists_ = true;
                maxWaitSelfLink_->send(1, NULL);
            }
            return true;
        }
        return false;
    }

    void maxWaitWakeup(SST::Event * ev) {
        checkMaxWait();
        maxWaitSelfLink_->send(1, NULL);
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
            if ( waitTime > cf_.maxWaitTime_ ) {
                d_->fatal(CALL_INFO, 1, "%s, Error: Maximum Cache Request time reached!\n"
                        "Event: %s 0x%" PRIx64 " from %s. Time = %" PRIu64 " ns\n",
                        getName().c_str(), CommandString[oldReq->getCmd()], oldReq->getAddr(), oldReq->getSrc().c_str(), curTime);
            }
        }
    }

    struct CacheConfig{
        string cacheFrequency_;
        CacheArray* cacheArray_;
        CacheArray* directoryArray_;
        uint protocol_;
        Output* dbg_;
        ReplacementMgr* rm_;
        uint numLines_;
        uint lineSize_;
        uint MSHRSize_;
        bool L1_;
        bool LLC_;
        bool LL_;
        bool allNoncacheableRequests_;
        SimTime_t maxWaitTime_;
        string type_;
    };
    
    CacheConfig             cf_;
    uint                    ID_;
    CacheListener*          listener_;
    vector<Link*>*          lowNetPorts_;
    Link*                   highNetPort_;
    Link*                   selfLink_;
    Link*                   maxWaitSelfLink_;
    MemNIC*                 bottomNetworkLink_;
    MemNIC*                 topNetworkLink_;
    Output*                 d_;
    Output*                 d2_;
    vector<string>          lowerLevelCacheNames_;
    vector<string>          upperLevelCacheNames_;
    MSHR*                   mshr_;
    MSHR*                   mshrNoncacheable_;
    CoherencyController*    coherenceMgr;
    queue<pair<SST::Event*, uint64> >   incomingEventQueue_;
    uint64                  accessLatency_;
    uint64                  tagLatency_;
    uint64                  mshrLatency_;
    uint64                  timestamp_;
    Clock::Handler<Cache>*  clockHandler_;
    TimeConverter*          defaultTimeBase_;
    std::map<string, LinkId_t>     nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;
    std::map<MemEvent*,uint64> startTimeList;
    std::map<MemEvent*,int> missTypeList;
    bool                    DEBUG_ALL;
    Addr                    DEBUG_ADDR;

    /* Performance enhancement: turn clocks off when idle */
    bool                    clockIsOn_;                 // Tell us whether clock is on or off
    SimTime_t               lastActiveClockCycle_;      // Cycle we turned the clock off at - for re-syncing stats
    SimTime_t               checkMaxWaitInterval_;      // Check for timeouts on this interval - when clock is on
    UnitAlgebra             maxWaitWakeupDelay_;        // Set wakeup event to check timeout on this interval - when clock is off
    bool                    maxWaitWakeupExists_;       // Whether a timeout wakeup exists


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
    Statistic<uint64_t>* statPutE_recv;
    Statistic<uint64_t>* statFetchInv_recv;
    Statistic<uint64_t>* statFetchInvX_recv;
    Statistic<uint64_t>* statInv_recv;
    Statistic<uint64_t>* statNACK_recv;
    Statistic<uint64_t>* statTotalEventsReceived;
    Statistic<uint64_t>* statTotalEventsReplayed;   // Used to be "MSHR Hits" but this makes more sense because incoming events may be an MSHR hit but will be counted as "event received"
    Statistic<uint64_t>* statInvStalledByLockedLine;

    Statistic<uint64_t>* statMSHROccupancy;
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
        might be in transition, regardless of the replacement policy and associativity used.  
        In this case the MSHR stores a pointer in the MSHR entry.  When a replacement candidate is NO longer in
	transition, the MSHR looks at the pointer and 'replays' the previously blocked request, which is  
	stored in another MSHR entry (different address).
        
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

    Coherence-related algorithms are handled by coherenceControllers. Implemented algorithms include an L1 MESI or MSI protocol,
    and incoherent L1 protocol, a lower-level (farther from CPU) cache MESI/MSI protocol for inclusive or exclusive caches, a lower-level 
    cache+directory MESI/MSI protocol for exclusive caches with an inclusive directory, and an incoherent lower-level cache protocol.
    The incoherent caches maintain present/not-present information and write back dirty blocks but make no attempt to track or resolve
    coherence conflicts (e.g., two caches CAN have independent copies of the same block and both be modifying their copies). For this reason,
    processors that rely on "correct" data from memHierarchy are not likely to work with incoherent caches.
    
    Prefetchers are part of SST's Cassini element, not part of MemHierarchy.  Prefetchers can be used along
    any level cache except for exclusive caches without an accompanying directory. Such caches are not able to determine whether a cache above them 
    (closer to the CPU) has a block and thus may prefetch blocks that are already present in their hierarchy. The lower-level caches are not designed
    to deal with this. Prefetch also does not currently work alongside shared, sliced caches as prefetches generated at one cache are not neccessarily
    for blocks mapped to that cache.
 
    Key notes:
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
    
        - An L1 cache handles as many requests as are sent by the CPU per cycle.  
         
        - Use a 'no wrapping' editor to view MH files, as many comments are on the 'side' and fall off the window 
*/



}}

#endif
