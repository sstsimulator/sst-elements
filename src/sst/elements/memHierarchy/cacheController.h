// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_CACHECONTROLLER_H_
#define MEMHIERARCHY_CACHECONTROLLER_H_

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

#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/mshr.h"
#include "sst/elements/memHierarchy/replacementManager.h"
#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/*
 * Component: memHierarchy.Cache
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
            {"llsc_block_cycles",       "(uint64_t) Number of cycles to prevent competing access to an LL/LR line. Encourages forward progress", "0"},
            {"debug",                   "(uint) Where to send output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level",             "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},
            {"debug_addr",              "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
            {"verbose",                 "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
            {"cache_line_size",         "(uint) Size of a cache line [aka cache block] in bytes.", "64"},
            {"force_noncacheable_reqs", "(bool) Used for verification purposes. All requests are considered to be 'noncacheable'. Options: 0[off], 1[on]", "false"},
            {"min_packet_size",         "(string) Number of bytes in a request/response not including payload (e.g., addr + cmd). Specify in B.", "8B"},
            {"banks",                   "(uint) Number of cache banks: One access per bank per cycle. Use '0' to simulate no bank limits (only limits on bandwidth then are max_requests_per_cycle and *_link_width", "0"})

    SST_ELI_DOCUMENT_PORTS(
            {"highlink",        "Non-network upper/processor-side link (i.e., link towards the core/accelerator/etc.). This port loads the 'memHierarchy.MemLink' manager. "
                                "To connect to a network component or to use non-default parameters on the MemLink subcomponent, fill the 'highlink' subcomponent slot instead of connecting this port.", {"memHierarchy.MemEventBase"} },
            {"lowlink",         "Non-network lower/memory-side link (i.e., link towards memory). This port loads the 'memHierarchy.MemLink' manager. "
                                "To connect to a network component or use non-default parameters on the MemLink subcomponent, fill the 'lowlink' subcomponent slot instead of connecting this port.", {"memHierarchy.MemEventBase"} },
            {"low_network_0",   "DEPRECATED: Use the 'lowlink' port or fill the 'lowlink' subcomponent slot with 'memHierarchy.MemLink' instead. "
                                "Non-network connection to lower level caches (closer to main memory)",             {"memHierarchy.MemEventBase"} },
            {"high_network_0",  "DEPRECATED: Use the 'highlink' port or fill the 'highlink' subcomponent slot with 'memHierarchy.MemLink' instead. "
                                "Non-network connection to higher level caches (closer to CPU)",                    {"memHierarchy.MemEventBase"} },
            {"directory",       "DEPRECATED: Use MemNIC or MemNICFour subcomponent in the 'lowlink' subcomponent slot instead. "
                                "Network link port to directory; doubles as request network port for split networks", {"memHierarchy.MemRtrEvent"} },
            {"directory_ack",   "DEPRECATED: Use MemNICFour subcomponent in the 'lowlink' subcomponent slot instead. "
                                "For split networks, response/ack network port to directory",                       {"memHierarchy.MemRtrEvent"} },
            {"directory_fwd",   "DEPRECATED: Use MemNICFour subcomponent in the 'lowlink' subcomponent slot instead. "
                                "For split networks, forward request network port to directory",                    {"memHierarchy.MemRtrEvent"} },
            {"directory_data",  "DEPRECATED: Use MemNICFour subcomponent in the 'lowlink' subcomponent slot instead. "
                                "For split networks, data network port to directory",                               {"memHierarchy.MemRtrEvent"} },
            {"cache",           "DEPRECATED: Use MemNIC or MemNICFour subcomponent in the 'highlink' subcomponent slot instead. "
                                "Network link port to cache; doubles as request network port for split networks",   {"memHierarchy.MemRtrEvent"} },
            {"cache_ack",       "DEPRECATED: Use MemNICFour subcomponent in the 'highlink' subcomponent slot instead. "
                                "For split networks, response/ack network port to cache.",                           {"memHierarchy.MemRtrEvent"} },
            {"cache_fwd",       "DEPRECATED: Use MemNICFour subcomponent in the 'highlink' subcomponent slot instead. "
                                "For split networks, forward request network port to cache",                        {"memHierarchy.MemRtrEvent"} },
            {"cache_data",      "DEPRECATED: Use MemNICFour subcomponent in the 'highlink' subcomponent slot instead. "
                                "For split networks, data network port to cache",                                   {"memHierarchy.MemRtrEvent"} })

    SST_ELI_DOCUMENT_STATISTICS(
            /* Cache hits and misses */
            {"TotalEventsReceived",     "Total number of events received by this cache", "events", 1},
            {"TotalEventsReplayed",     "Total number of events that were initially blocked and then were replayed", "events", 1},
            {"MSHR_occupancy",          "Number of events in MSHR each cycle", "events", 1},
            {"Bank_conflicts",          "Total number of bank conflicts detected", "count", 1},
            {"Prefetch_requests",       "Number of prefetches received from prefetcher at this cache", "events", 1},
            {"Prefetch_drops",          "Number of prefetches that were cancelled. Reasons: too many prefetches outstanding, cache can't handle prefetch this cycle, currently handling another event for the address.", "events", 1},
            /*Event receives */
            {"GetS_recv",               "Event received: GetS", "count", 2},
            {"GetX_recv",               "Event received: GetX", "count", 2},
            {"Write_recv",              "Event received: Write", "count", 2},
            {"GetSX_recv",              "Event received: GetSX", "count", 2},
            {"GetSResp_recv",           "Event received: GetSResp", "count", 2},
            {"GetXResp_recv",           "Event received: GetXResp", "count", 2},
            {"WriteResp_recv",          "Event received: WriteResp", "count", 2},
            {"PutM_recv",               "Event received: PutM", "count", 2},
            {"PutX_recv",               "Event received: PutX", "count", 2},
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
            {"FlushAll_recv",           "Event received: FlushAll", "count", 2},
            {"ForwardFlush_recv",       "Event received: ForwardFlush", "count", 2},
            {"UnblockFlush_recv",       "Event received: UnblockFlush", "count", 2},
            {"FlushLineResp_recv",      "Event received: FlushLineResp", "count", 2},
            {"FlushAllResp_recv",       "Event received: FlushAllResp", "count", 2},
            {"AckFlush_recv",           "Event received: AckFlush", "count", 2},
            {"NACK_recv",               "Event: NACK received", "count", 2},
            {"NULLCMD_recv",            "Event: NULLCMD received", "count", 2},
            {"Get_uncache_recv",        "Noncacheable Event: Get received", "count", 6},
            {"Put_uncache_recv",        "Noncacheable Event: Put received", "count", 6},
            {"AckMove_uncache_recv",    "Noncacheable Event: AckMove received", "count", 6},
            {"CustomReq_uncache_recv",  "Noncacheable Event: CustomReq received", "count", 4},
            {"CustomResp_uncache_recv", "Noncacheable Event: CustomResp received", "count", 4},
            {"CustomAck_uncache_recv",  "Noncacheable Event: CustomAck received", "count", 4},
            {"GetS_uncache_recv",       "Noncacheable Event: GetS received", "count", 4},
            {"Write_uncache_recv",      "Noncacheable Event: Write received", "count", 4},
            {"GetSX_uncache_recv",      "Noncacheable Event: GetSX received", "count", 4},
            {"GetSResp_uncache_recv",   "Noncacheable Event: GetSResp received", "count", 4},
            {"WriteResp_uncache_recv",  "Noncacheable Event: WriteResp received", "count", 4},
            {"default_stat",            "Default statistic used for unexpected events/cases/etc. Should be 0, if not, check for missing statistic registrations.", "none", 7})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"highlink", "Port manager on the upper/processor-side (i.e., where requests typically come from). If you use this subcomponent slot, you do not need to connect the cache's highlink port. Do connect this subcomponent's ports instead. For caches with a single link, use this subcomponent slot only.", "SST::MemHierarchy::MemLinkBase"},
            {"lowlink", "Port manager on the lower/memory side (i.e., where cache misses should be sent to). If you use this subcomponent slot, you do not need to connect the cache's lowlink port. Do connect this subcomponent's ports instead. For caches with a single link, use the 'highlink' subcomponent slot only.", "SST::MemHierarchy::MemLinkBase"},
            {"cpulink", "DEPRECATED. To standardize naming, use the 'highlink' slot instead. Port manager on the CPU-side (i.e., where requests typically come from). If you use this subcomponent slot, you do not need to connect the cache's ports. Do connect this subcomponent's ports instead. For caches with a single link, use this subcomponent slot only.", "SST::MemHierarchy::MemLinkBase"},
            {"memlink", "DEPRECATED. To standardize naming, use the 'lowlink' slot instead. Port manager on the memory side (i.e., where cache misses should be sent to). If you use this subcomponent slot, you do not need to connect the cache's ports. Do connect this subcomponent's ports instead. For caches with a single link, use the 'highlink' subcomponent slot only.", "SST::MemHierarchy::MemLinkBase"},
            {"prefetcher", "Prefetcher(s)", "SST::MemHierarchy::CacheListener"},
            {"listener", "Cache listener(s) for statistics, tracing, etc. In contrast to prefetcher, cannot send events to cache", "SST::MemHierarchy::CacheListener"},
            {"replacement", "Replacement policies. Slot 0 is for cache. In caches that include a directory, use slot 1 to specify the directory's replacement policy. ", "SST::MemHierarchy::ReplacementPolicy"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"},
            {"coherence", "Coherence protocol. The cache will fill this slot automatically based on cache parameters. Do not use this slot directly.", "SST::MemHierarchy::CoherenceController"})

/* Class definition */
    friend class InstructionStream; // TODO what is this?

    /** Constructor for Cache Component */
    Cache(ComponentId_t id, Params &params);
    ~Cache() { }

    /** Component API - pre- and post-simulation */
    virtual void init(unsigned int phase) override;
    virtual void setup(void) override;
    virtual void complete(unsigned int phase) override;
    virtual void finish(void) override;

    /** Component API - debug and fatal */
    void printStatus(Output & out) override; // Called on SIGUSR2
    void emergencyShutdown() override;       // Called on output.fatal(), SIGINT/SIGTERM

    /* Serialization */
    Cache() : Component() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::Cache)

private:
    /** Cache factory methods **************************************************/

    // Check for deprecated params and warn
    void checkDeprecatedParams(Params &params);

    // Create the cache array
    void createCacheArray(Params &params);

    // Construct MSHR
    uint64_t createMSHR(Params &params, uint64_t accessLatency, bool L1);

    // Construct cache listeners
    void createListeners(Params &params);

    // Create clock
    void createClock(Params &params);

    // Statistic initialization
    void registerStatistics();

    // Coherence manager creation
    void createCoherenceManager(Params &params);

    // Configure links
    void configureLinks(Params &params, TimeConverter* tc);

    /** Cache operation ********************************************************/

    // Computes base address based on line size
    Addr toBaseAddr(Addr addr) {
        return (addr) & ~(lineSize_ - 1);
    }

    // Handle incoming events -> prepare to process
    void handleEvent(SST::Event *event);

    // Handle incoming prefetching events -> prepare to process
    void handlePrefetchEvent(SST::Event *event);

    // Process events
    bool processEvent(MemEventBase * ev, bool inMSHR);

    // Process an incoming event that is not meant for the cache
    void processNoncacheable(MemEventBase* event);

    // Self-Event prefetch handler for this component
    void processPrefetchEvent(SST::Event *event);

    // Clock handler
    bool clockTick(Cycle_t time);

    // Clock helpers - turn clock on & off
    void turnClockOn();
    void turnClockOff();

    // Trigger timeouts if events sit in MSHR for too long
    void timeoutWakeup(SST::Event * ev);
    void checkTimeout();

    // Arbitrate for bank and/or line access
    bool arbitrateAccess(Addr addr);
    void updateAccessStatus(Addr addr);

    // Process coherence initialization events
    void processInitCoherenceEvent(MemEventInitCoherence* event, bool src);


    /** Cache structures *******************************************************/
    std::vector<CacheListener*> listeners_; // Cache listeners, including prefetchers
    MemLinkBase* linkUp_ = nullptr;         // link manager up (towards CPU)
    MemLinkBase* linkDown_ = nullptr;       // link manager down (towards memory)
    Link* prefetchSelfLink_ = nullptr;      // link to delay prefetch request receive
    Link* timeoutSelfLink_ = nullptr;       // link to check for timeouts (possible deadlock)
    MSHR* mshr_;                            // MSHR
    CoherenceController* coherenceMgr_;     // Coherence protocol - where most of the event handling happens
    std::map<MemEventBase::id_type, std::string> init_requests_;    // Event response routing for untimed/init events

    /** Latencies **************************************************************/
    SimTime_t   prefetchDelay_;

    /** Cache configuration ****************************************************/
    uint64_t            lineSize_;
    bool                allNoncacheableRequests_;
    int                 maxRequestsPerCycle_;
    MemRegion           region_; // Memory region handled by this cache
    SimTime_t           timeout_;
    uint64_t            maxOutstandingPrefetch_;
    bool                banked_;

    /** Clocks *****************************************************************/
    Clock::HandlerBase*     clockHandler_;
    TimeConverter           defaultTimeBase_;
    bool                    clockIsOn_;     // Whether clock is on or off
    bool                    clockUpLink_;   // Whether link actually needs clock() called or not
    bool                    clockDownLink_; // Whether link actually needs clock() called or not
    SimTime_t               lastActiveClockCycle_;  // Cycle we turned the clock off at - for re-syncing stats

    /** Cache state ************************************************************/
    uint64_t                    timestamp_;
    int                         requestsThisCycle_;
    std::vector<bool>           bankStatus_;
    std::set<Addr>              addrsThisCycle_;
    std::list<MemEventBase*>    retryBuffer_;
    std::list<MemEventBase*>    eventBuffer_;
    std::queue<MemEventBase*>   prefetchBuffer_;
    std::map<SST::Event::id_type, std::string> noncacheableResponseDst_;


    /** Output and debug *******************************************************/
    Output*                 out_;
    Output*                 dbg_;
    std::set<Addr>          debug_addr_filter_;

    /** Statistics *************************************************************/
    Statistic<uint64_t>* statMSHROccupancy;
    Statistic<uint64_t>* statBankConflicts;

    // Prefetch statistics
    Statistic<uint64_t>* statPrefetchRequest;
    Statistic<uint64_t>* statPrefetchDrop;

    // Event counts
    Statistic<uint64_t>* statRecvEvents;
    Statistic<uint64_t>* statRetryEvents;
    Statistic<uint64_t>* statUncacheRecv[(int)Command::LAST_CMD];
    Statistic<uint64_t>* statCacheRecv[(int)Command::LAST_CMD];
};

}}
#endif // MEMHIERARCHY_CACHECONTROLLER_H_
