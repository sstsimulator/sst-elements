// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cache.h
 * Author: Caesar De la Paz III
 */

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include <queue>
#include <map>
#include <string>
#include <sstream>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/cacheArray.h"
#include "sst/elements/memHierarchy/mshr.h"
#include "sst/elements/memHierarchy/replacementManager.h"
#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/*
 * Component: memHirearchy.Cache
 *
 * Cache controller
 */

class Cache : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(Cache, "memHierarchy", "Cache", SST_ELI_ELEMENT_VERSION(1,0,0), "Cache controller", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            /* Required */
            {"cache_frequency",         "(string) Clock frequency or period with units (Hz or s; SI units OK). For L1s, this is usually the same as the CPU's frequency.", NULL},
            {"cache_size",              "(string) Cache size with units. Eg. 4KiB or 1MiB"},
            {"associativity",           "(int) Associativity of the cache. In set associative mode, this is the number of ways."},
            {"access_latency_cycles",   "(int) Latency (in cycles) to access the cache data array. This latency is paid by cache hits and coherence requests that need to return data."},
            {"L1",                      "(bool) Required for L1s, specifies whether cache is an L1. Options: 0[not L1], 1[L1]", "false"},
            {"node",			"(uint) Node number in multinode evnironment"},
            /* Not required */
            {"cache_line_size",         "(uint) Size of a cache line (aka cache block) in bytes.", "64"},
            {"coherence_protocol",      "(string) Coherence protocol. Options: MESI, MSI, NONE", "MESI"},
            {"cache_type",              "(string) - Cache type. Options: inclusive cache ('inclusive', required for L1s), non-inclusive cache ('noninclusive') or non-inclusive cache with a directory ('noninclusive_with_directory', required for non-inclusive caches with multiple upper level caches directly above them),", "inclusive"},
            {"max_requests_per_cycle",  "(int) Maximum number of requests to accept per cycle. 0 or negative is unlimited.", "-1"},
            {"request_link_width",      "(string) Limits number of request bytes sent per cycle. Use 'B' units. '0B' is unlimited.", "0B"},
            {"response_link_width",     "(string) Limits number of response bytes sent per cycle. Use 'B' units. '0B' is unlimited.", "0B"},
            {"noninclusive_directory_entries", "(uint) Number of entries in the directory. Must be at least 1 if the non-inclusive directory exists.", "0"},
            {"noninclusive_directory_associativity", "(uint) For a set-associative directory, number of ways.", "1"},
            {"mshr_num_entries",        "(int) Number of MSHR entries. Not valid for L1s because L1 MSHRs assumed to be sized for the CPU's load/store queue. Setting this to -1 will create a very large MSHR.", "-1"},
            {"tag_access_latency_cycles",
                "(uint) Latency (in cycles) to access tag portion only of cache. Paid by misses and coherence requests that don't need data. If not specified, defaults to access_latency_cycles","access_latency_cycles"},
            {"mshr_latency_cycles",
                "(uint) Latency (in cycles) to process responses in the cache and replay requests. Paid on the return/response path for misses instead of access_latency_cycles. If not specified, simple intrapolation is used based on the cache access latency", "1"},
            {"prefetch_delay_cycles",   "(uint) Delay prefetches from prefetcher by this number of cycles.", "1"},
            {"max_outstanding_prefetch","(uint) Maximum number of prefetch misses that can be outstanding, additional prefetches will be dropped/NACKed. Default is 1/2 of MSHR entries.", "0.5*mshr_num_entries"},
            {"drop_prefetch_mshr_level","(uint) Drop/NACK prefetches if the number of in-use mshrs is greater than or equal to this number. Default is mshr_num_entries - 2.", "mshr_num_entries-2"},
            {"num_cache_slices",        "(uint) For a distributed, shared cache, total number of cache slices", "1"},
            {"slice_id",                "(uint) For distributed, shared caches, unique ID for this cache slice", "0"},
            {"slice_allocation_policy", "(string) Policy for allocating addresses among distributed shared cache. Options: rr[round-robin]", "rr"},
            {"maxRequestDelay",         "(uint) Set an error timeout if memory requests take longer than this in ns (0: disable)", "0"},
            {"snoop_l1_invalidations",  "(bool) Forward invalidations from L1s to processors. Options: 0[off], 1[on]", "false"},
            {"debug",                   "(uint) Where to send output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level",             "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},
            {"debug_addr",              "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
            {"verbose",                 "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
            {"cache_line_size",         "(uint) Size of a cache line [aka cache block] in bytes.", "64"},
            {"force_noncacheable_reqs", "(bool) Used for verification purposes. All requests are considered to be 'noncacheable'. Options: 0[off], 1[on]", "false"},
            {"min_packet_size",         "(string) Number of bytes in a request/response not including payload (e.g., addr + cmd). Specify in B.", "8B"},
            {"banks",                   "(uint) Number of cache banks: One access per bank per cycle. Use '0' to simulate no bank limits (only limits on bandwidth then are max_requests_per_cycle and *_link_width", "0"},
            /* Old parameters - deprecated or moved */
            {"network_address",             "DEPRECATED - Now auto-detected by link control."}, // Remove 9.0
            {"network_bw",                  "MOVED - Now a member of the MemNIC subcomponent.", "80GiB/s"}, // Remove 9.0
            {"network_input_buffer_size",   "MOVED - Now a member of the MemNIC subcomponent.", "1KiB"}, // Remove 9.0
            {"network_output_buffer_size",  "MOVED - Now a member of the MemNIC subcomponent.", "1KiB"}, // Remove 9.0
            {"prefetcher",                  "MOVED - Prefetcher subcomponent, instead specify by putting it in the 'prefetcher' subcomponent slot", ""},
            {"replacement_policy",          "MOVED - Cache replacement policy, now a subcomponent so specify by putting in the index 0 of the 'replacement' subcomponent slot in the input config", ""},
            {"noninclusive_directory_repl", "MOVED - Replacement policy for noninclusive directory, now a subcomponent, specify by putting in the index 1 of the 'replacement' subcomponent slot in the input config", ""},
            {"hash_function",               "MOVED - Hash function for mapping addresses to cache lines, now a subcomponent, specify by filling the 'hash' slot (default/unfilled is none)", ""})

    SST_ELI_DOCUMENT_PORTS(
            {"low_network_0",   "Port connected to lower level caches (closer to main memory)",                     {"memHierarchy.MemEventBase"} },
            {"high_network_0",  "Port connected to higher level caches (closer to CPU)",                            {"memHierarchy.MemEventBase"} },
            {"directory",       "Network link port to directory; doubles as request network port for split networks", {"memHierarchy.MemRtrEvent"} },
            {"directory_ack",   "For split networks, response/ack network port to directory",                       {"memHierarchy.MemRtrEvent"} },
            {"directory_fwd",   "For split networks, forward request network port to directory",                    {"memHierarchy.MemRtrEvent"} },
            {"directory_data",  "For split networks, data network port to directory",                               {"memHierarchy.MemRtrEvent"} },
            {"cache",           "Network link port to cache; doubles as request network port for split networks",   {"memHierarchy.MemRtrEvent"} },
            {"cache_ack",       "For split networks, response/ack network port to cache",                           {"memHierarchy.MemRtrEvent"} },
            {"cache_fwd",       "For split networks, forward request network port to cache",                        {"memHierarchy.MemRtrEvent"} },
            {"cache_data",      "For split networks, data network port to cache",                                   {"memHierarchy.MemRtrEvent"} })

    SST_ELI_DOCUMENT_STATISTICS(
            /* Cache hits and misses */
            {"CacheHits",               "Total number of cache hits", "count", 1},
            {"CacheMisses",             "Total number of cache misses", "count", 1},
            {"GetSHit_Arrival",         "GetS was handled at arrival and was a cache hit", "count", 1},
            {"GetSHit_Blocked",         "GetS was blocked in MSHR at arrival and later was a cache hit", "count", 1},
            {"GetXHit_Arrival",         "GetX was handled at arrival and was a cache hit", "count", 1},
            {"GetXHit_Blocked",         "GetX was blocked in MSHR at arrival and  later was a cache hit", "count", 1},
            {"GetSXHit_Arrival",        "GetSX was handled at arrival and was a cache hit", "count", 1},
            {"GetSXHit_Blocked",        "GetSX was blocked in MSHR at arrival and  later was a cache hit", "count", 1},
            {"GetSMiss_Arrival",        "GetS was handled at arrival and was a cache miss", "count", 1},
            {"GetSMiss_Blocked",        "GetS was blocked in MSHR at arrival and later was a cache miss", "count", 1},
            {"GetXMiss_Arrival",        "GetX was handled at arrival and was a cache miss", "count", 1},
            {"GetXMiss_Blocked",        "GetX was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
            {"GetSXMiss_Arrival",       "GetSX was handled at arrival and was a cache miss", "count", 1},
            {"GetSXMiss_Blocked",       "GetSX was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
            {"TotalEventsReceived",     "Total number of events received by this cache", "events", 1},
            {"TotalEventsReplayed",     "Total number of events that were initially blocked and then were replayed", "events", 1},
            {"TotalNoncacheableEventsReceived", "Total number of non-cache or noncacheable cache events that were received by this cache and forward", "events", 1},
            {"MSHR_occupancy",          "Number of events in MSHR each cycle", "events", 1},
            {"Bank_conflicts",          "Total number of bank conflicts detected", "count", 1},
            {"Prefetch_requests",       "Number of prefetches received from prefetcher at this cache", "events", 1},
            {"Prefetch_drops",          "Number of prefetches that were cancelled. Reasons: too many prefetches outstanding, cache can't handle prefetch this cycle, currently handling another event for the address.", "events", 1},
            /* Coherence events - break down GetS between S/E */
            {"SharedReadResponse",      "Coherence: Received shared response to a GetS request", "count", 2},
            {"ExclusiveReadResponse",   "Coherence: Received exclusive response to a GetS request", "count", 2},
            /*Event receives */
            {"GetS_recv",               "Event received: GetS", "count", 2},
            {"GetX_recv",               "Event received: GetX", "count", 2},
            {"GetSX_recv",              "Event received: GetSX", "count", 2},
            {"GetSResp_recv",           "Event received: GetSResp", "count", 2},
            {"GetXResp_recv",           "Event received: GetXResp", "count", 2},
            {"PutM_recv",               "Event received: PutM", "count", 2},
            {"PutS_recv",               "Event received: PutS", "count", 2},
            {"PutE_recv",               "Event received: PutE", "count", 2},
            {"FetchInv_recv",           "Event received: FetchInv", "count", 2},
            {"Fetch_recv",              "Event received: Fetch", "count", 2},
            {"FetchInvX_recv",          "Event received: FetchInvX", "count", 2},
            {"ForceInv_recv",           "Event received: ForceInv", "count", 2},
            {"Inv_recv",                "Event received: Inv", "count", 2},
            {"FetchResp_recv",          "Event received: FetchResp", "count", 2},
            {"FetchXResp_recv",         "Event received: FetchXResp", "count", 2},
            {"AckInv_recv",             "Event received: AckInv", "count", 2},
            {"AckPut_recv",             "Event received: AckPut", "count", 2},
            {"FlushLine_recv",          "Event received: FlushLine", "count", 2},
            {"FlushLineInv_recv",       "Event received: FlushLineInv", "count", 2},
            {"FlushLineResp_recv",      "Event received: FlushLineResp", "count", 2},
            {"NACK_recv",               "Event: NACK received", "count", 2},
            {"Get_recv",                "Event: Get received", "count", 6},
            {"Put_recv",                "Event: Put received", "count", 6},
            {"AckMove_recv",            "Event: AckMove received", "count", 6},
            {"CustomReq_recv",          "Event: CustomReq received", "count", 4},
            {"CustomResp_recv",         "Event: CustomResp received", "count", 4},
            {"CustomAck_recv",          "Event: CustomAck received", "count", 4},
            {"default_stat",            "Default statistic used for unexpected events/cases/etc. Should be 0, if not, check for missing statistic registerations.", "none", 7})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"cpulink", "CPU-side link manager, for single-link caches, use this one only", "SST::MemHierarchy::MemLinkBase"},
            {"memlink", "Memory-side link manager", "SST::MemHierarchy::MemLinkBase"},
            {"replacement", "Replacement policies, slot 0 is for cache, slot 1 is for directory (if it exists)", "SST::MemHierarchy::ReplacementPolicy"},
            {"coherence", "Coherence protocol", "SST::MemHierarchy::CoherenceController"},
            {"prefetcher", "Prefetcher(s)", "SST::MemHierarchy::CacheListener"},
            {"listener", "Cache listener(s) for statistics, tracing, etc. In contrast to prefetcher, cannot send events to cache", "SST::MemHierarchy::CacheListener"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"} )

/* Class definition */
    typedef CacheArray::CacheLine           CacheLine;
    typedef CacheArray::DataLine            DataLine;
    typedef map<Addr, MSHRRegister>            mshrTable;
    typedef unsigned int                    uint;
    typedef uint64_t                        uint64;

    friend class InstructionStream;

    /** Constructor for Cache Component */
    Cache(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup(void);
    virtual void finish(void);

    /** Computes the 'Base Address' of the requests.  The base address point the first address of the cache line */
    Addr toBaseAddr(Addr addr){
        return (addr) & ~(cacheArray_->getLineSize() - 1);
    }

    /* Debug/fatal */
    void printStatus(Output & out); // Called on SIGUSR2
    void emergencyShutdown();       // Called on output.fatal(), (SIGINT/SIGTERM)

private:
    /** Constructor helper methods */
    void checkDeprecatedParams(Params &params);
    ReplacementPolicy* constructReplacementManager(std::string policy, uint64_t lines, uint64_t associativity, int slot);
    CacheArray* createCacheArray(Params &params);
    int createMSHR(Params &params);
    void createListeners(Params &params, int mshrSize);
    void createClock(Params &params);
    void registerStatistics();
    void createCoherenceManager(Params &params);

    /** Handle incoming events -> prepare to process */
    void handleEvent(SST::Event *event);

    /** Handle incoming prefetching events -> prepare to process */
    void handlePrefetchEvent(SST::Event *event);

    /** Process an incoming event that is not meant for the cache */
    void processNoncacheable(MemEventBase* event);

    /** Process the oldest incoming event */
    bool processEvent(MemEventBase* event, bool mshrHit);

    /** Configure this component's links */
    void configureLinks(Params &params);

    /** Self-Event prefetch handler for this component */
    void processPrefetchEvent(SST::Event *event);

    /** Function processes incomming access requests from HiLv$ or the CPU
        It appropriately redirects requests to Top and/or Bottom controllers.  */
    void processCacheRequest(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);
    void processCacheReplacement(MemEvent *event, Command cmd, Addr baseAddr, bool mshrHit);
    void processCacheFlush(MemEvent * event, Addr baseAddr, bool mshrHit);

    /** Function processes incomming GetS/GetX responses.
        Redirects message to Top Controller */
    void processCacheResponse(MemEvent* ackEvent, Addr baseAddr);

    /** Function attempts to send all responses for previous events that 'blocked' due to an outstanding request.
        If response blocks cache line the remaining responses go to MSHR till new outstanding request finishes  */
    void activatePrevEvents(Addr baseAddr);

    /** This function re-processes a signle previous request.  In hardware, the MSHR would be looked up,
        the MSHR entry would be modified and the response would be sent directly without reading the cache
        array and tag.  In SST, we just rerun the request to avoid complexity */
    inline bool activatePrevEvent(MemEvent* event, list<MSHREntry>& entries, Addr addr, list<MSHREntry>::iterator& it, int i);

    inline void postRequestProcessing(MemEvent* event);

    inline void postReplacementProcessing(MemEvent* event, CacheAction action, bool mshrHit);

    /** If cache line was user-locked, then events might be waiting for lock to be released
        and need to be reactivated */
    inline void reActivateEventWaitingForUserLock(CacheLine* cacheLine);

    /** Insert to MSHR wrapper */
    inline bool insertToMSHR(Addr baseAddr, MemEvent* event);

    /** Try to insert request to MSHR.  If not sucessful, function send a NACK to requestor */
    bool processRequestInMSHR(Addr baseAddr, MemEvent* event);

    /** Determines what CC will send the NACK. */
    void sendNACK(MemEvent* event);

    /** Verify that input parameters are valid */
    void errorChecking();

    /** Print input members/parameters */
    void pMembers();

    /** Get the front element of a MSHR entry */
    MemEvent* getOrigReq(const list<MSHREntry> entries);

    /** Timestamp getter */
    uint64 getTimestamp(){ return timestamp_; }

    /** Find the appropriate MSHR lookup latency cycles in case the user did not provide
        any parameter values for this type of latency.  This function intrapolates from
        numbers gather by other papers/results */
    void intrapolateMSHRLatency();

    void profileEvent(MemEvent* event, Command cmd, bool replay, bool canStall);

    /**  Clock Handler.  Every cycle events are executed (if any).  If clock is idle long enough,
         the clock gets deregistered from TimeVortx and reregistered only when an event is received */
    bool clockTick(Cycle_t time);
    void turnClockOn();
    void turnClockOff();

    void maxWaitWakeup(SST::Event * ev) {
        checkMaxWait();
        maxWaitSelfLink_->send(1, nullptr);
    }

    void checkMaxWait(void) const {
        SimTime_t curTime = Simulation::getSimulation()->getCurrentSimCycle();
        const MSHREntry *oldEntry = mshr_->getOldestRequest();

        if ( oldEntry ) {
            SimTime_t waitTime = curTime - oldEntry->getInsertionTime();
            if ( waitTime > maxWaitTime_ ) {
                out_->fatal(CALL_INFO, 1, "%s, Error: Maximum Cache Request time reached - potential deadlock or other error. "
                        "Event: %s. Current time: (%" PRIu64 " cycles, %" PRIu64 " ns). Event start time: %" PRIu64 "cycles.\n",
                    getName().c_str(), (oldEntry->elem).getEvent()->getVerboseString().c_str(), curTime, getCurrentSimTimeNano(), oldEntry->getInsertionTime());
            }
        }
    }

    /* Cache parameters */
    std::string             type_;
    CoherenceProtocol       protocol_;
    bool                    L1_;
    bool                    allNoncacheableRequests_;
    SimTime_t               maxWaitTime_;
    SimTime_t               maxWaitTimeNano_;
    unsigned int            maxBytesUpPerCycle_;
    unsigned int            maxBytesDownPerCycle_;
    uint64_t                accessLatency_;
    uint64_t                tagLatency_;
    uint64_t                mshrLatency_;
    uint64_t                dropPrefetchLevel_;
    uint64_t                maxOutstandingPrefetch_;
    SimTime_t               prefetchDelay_;
    int                     maxRequestsPerCycle_;

    /* Cache structures */
    CacheArray*             cacheArray_;
    std::vector<CacheListener*> listeners_;
    MemLinkBase*            linkUp_;
    MemLinkBase*            linkDown_;
    Link*                   prefetchLink_;
    Link*                   maxWaitSelfLink_;
    MSHR*                   mshr_;
    CoherenceController*    coherenceMgr_;
    Clock::Handler<Cache>*  clockHandler_;
    TimeConverter*          defaultTimeBase_;

    /* Debug and output */
    Output*                 out_;
    Output*                 d_;
    Output*                 d2_;
    std::set<Addr>          DEBUG_ADDR;

    /* Variables */
    vector<string>          lowerLevelCacheNames_;
    vector<string>          upperLevelCacheNames_;
    uint64_t                timestamp_;
    int                     requestsThisCycle_;
    std::map<SST::Event::id_type, std::string> responseDst_;

    std::list<MemEventBase*> eventBuffer_;      // Buffer incoming events until clock tick
    std::list<MemEventBase*> retryBuffer_;      // Events that need to be retried
    std::queue<MemEventBase*> prefetchBuffer_;  // Prefetch events; purged every cycle
    
    std::queue<MemEventBase*>       requestBuffer_;                 // Buffer requests that can't be processed due to port limits
    std::vector< std::queue<MemEventBase*> > bankConflictBuffer_;   // Buffer requests that have bank conflicts
    std::vector<bool>               bankStatus_;    // TODO change if we want multiported banks
    MemRegion                       region_; // Memory region handled by this cache

    // These parameters are for the coherence controller and are detected during init
    bool                    isLL;
    bool                    lowerIsNoninclusive;
    bool                    expectWritebackAcks;
    bool                    silentEvict;

    /* Performance enhancement: turn clocks off when idle */
    bool                    clockIsOn_;                 // Tell us whether clock is on or off
    SimTime_t               lastActiveClockCycle_;      // Cycle we turned the clock off at - for re-syncing stats
    SimTime_t               checkMaxWaitInterval_;      // Check for timeouts on this interval - when clock is on
    UnitAlgebra             maxWaitWakeupDelay_;        // Set wakeup event to check timeout on this interval - when clock is off
    bool                    maxWaitWakeupExists_;       // Whether a timeout wakeup exists
    bool                    clockUpLink_; // Whether link actually needs clock() called or not
    bool                    clockDownLink_; // Whether link actually needs clock() called or not

    // Temporary
    bool doInCoherenceMgr_;

    /*
     * Statistics API stats
     */
    // Cache hits
    Statistic<uint64_t>* statCacheHits;
    Statistic<uint64_t>* statGetSHitOnArrival;
    Statistic<uint64_t>* statGetXHitOnArrival;
    Statistic<uint64_t>* statGetSXHitOnArrival;
    Statistic<uint64_t>* statGetSHitAfterBlocked;
    Statistic<uint64_t>* statGetXHitAfterBlocked;
    Statistic<uint64_t>* statGetSXHitAfterBlocked;
    // Cache misses
    Statistic<uint64_t>* statCacheMisses;
    Statistic<uint64_t>* statGetSMissOnArrival;
    Statistic<uint64_t>* statGetXMissOnArrival;
    Statistic<uint64_t>* statGetSXMissOnArrival;
    Statistic<uint64_t>* statGetSMissAfterBlocked;
    Statistic<uint64_t>* statGetXMissAfterBlocked;
    Statistic<uint64_t>* statGetSXMissAfterBlocked;
    // Events received
    Statistic<uint64_t>* stat_eventRecv[(int)Command::LAST_CMD];
    Statistic<uint64_t>* statTotalEventsReceived;
    Statistic<uint64_t>* statTotalEventsReplayed;   // Used to be "MSHR Hits" but this makes more sense because incoming events may be an MSHR hit but will be counted as "event received"
    Statistic<uint64_t>* statNoncacheableEventsReceived; // Counts any non-cache events that are received and forwarded
    Statistic<uint64_t>* statInvStalledByLockedLine;

    Statistic<uint64_t>* statMSHROccupancy;
    Statistic<uint64_t>* statBankConflicts;

    // Prefetch statistics
    Statistic<uint64_t>* statPrefetchRequest;
    Statistic<uint64_t>* statPrefetchDrop;
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
        subclasses of the main based abstrac class (ReplacementPolicy), therefore they need to implement
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
        - The cache supports hardware "locking" (GetSX command), and atomics-based requests (LLSC, GetS with
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


    Latencies
        - access_latency_cycles - Time to access the cache data array. Assumed to be longer than or equal to tag_access_latency_cycles so that a
                                miss pays the lesser of the two and a hit the greater. This latency is paid by cache hits and coherence requests that need to return data.
        - tag_access_latency_cycles - Time to access the cache tag array. This latency is paid by caches misses and by coherence requests like invalidations which don't need to touch the data array.
        - mshr_latency_cycles - Time to access the mshrs - used instead of the tag_access_latency and/or access_latency_cycles for replayed events and MSHR hits.

        Examples:
        L1 miss + L2 hit: L1 tag_access_latency_cycles + L2 access_latency_cycles + L1 mshr_latency_cycles + (any transit time over buses/network/etc.)
        L1 hit: access_latency_cycles
        Invalidation request: tag_access_latency_cycles
        Invalidation + data (Fetch) request: access_latency_cycles


        Other notes:
            Accesses to a single address are serialized in time by their access latency. So for a cache with a 4 cycle access,
            if requests A and B for the same block are received at cycles 1 and 2 respectively, A will return at 5 and B will return at 9.
*/



}}

#endif
