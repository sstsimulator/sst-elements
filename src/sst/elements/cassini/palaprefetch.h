// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_STRIDE_PREFETCH_PALA
#define _H_SST_STRIDE_PREFETCH_PALA

#include <unordered_map>
#include <vector>
#include <deque>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>

#include <sst/core/output.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

enum PrefetcherState { P_INVALID, P_PENDING, P_VALID };

struct StrideFilter
{
    uint64_t lastAddress;
    int32_t lastStride;
    int32_t stride;
    PrefetcherState state;
};


class PalaPrefetcher : public SST::MemHierarchy::CacheListener
{
public:
    PalaPrefetcher(ComponentId_t id, Params& params);
    ~PalaPrefetcher();

    void notifyAccess(const CacheListenerNotification& notify);
    void registerResponseCallback(Event::HandlerBase *handler);
    void printStats(Output &out);

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        PalaPrefetcher,
            "cassini",
            "PalaPrefetcher",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Stride Prefetcher [Palacharla 1994]",
            SST::MemHierarchy::CacheListener
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",                     "Controls the verbosity of the cassini components", ""},
            { "cache_line_size",             "Controls the cache line size of the cache the prefetcher is attached too", "64"},
            { "history",                     "Start of Address Range, for this controller.", "16"},
            { "reach",                       "Reach of the prefetcher (ie how far forward to make requests)", "2"},
            { "detect_range",                "Range to detact addresses over in request-counts, default is 4.", "4"},
            { "address_count",               "Number of addresses to keep in the prefetch table", "64"},
            { "page_size",                   "Start of Address Range, for this controller.", "4096"},
            { "overrun_page_boundaries",     "Allow prefetcher to run over page alignment boundaries, default is 0 (false)", "0"},
            { "tag_size",                    "Number of bits used for address matching in table", "48"},
            { "addr_size",                   "Number of bits used for addresses", "64"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "prefetches_issued",			  "Counts number of prefetches issued",	"prefetches", 1 },
        { "prefetches_canceled_by_page_boundary", "Counts number of prefetches which spanned over page boundaries and so did not issue", "prefetches", 1 },
        { "prefetches_canceled_by_history",       "Counts number of prefetches which did not get issued because of prefetch history", "prefetches", 1 },
        { "prefetch_opportunities",               "Counts the number of prefetch opportunities", "prefetches", 1 }
    )

private:
    void     DispatchRequest(Addr targetAddress);

    Output* output;
    std::vector<Event::HandlerBase*> registeredCallbacks;
    std::deque<uint64_t>* prefetchHistory;
    std::unordered_map< uint64_t, StrideFilter >* recentAddrList;
    std::deque< std::unordered_map< uint64_t, StrideFilter >::iterator >* recentAddrListQueue;
    std::deque< std::unordered_map< uint64_t, StrideFilter >::iterator >::iterator it;

    uint64_t pageSize;
    uint64_t blockSize;
    uint64_t tagSize;
    uint32_t addressSize;

    bool     overrunPageBoundary;
    uint32_t prefetchHistoryCount;
    uint32_t recentAddrListCount;
    uint32_t nextRecentAddressIndex;
    uint32_t strideDetectionRange;
    uint32_t strideReach;
    uint32_t recheckCountdown;
    uint64_t missEventsProcessed;
    uint64_t hitEventsProcessed;
    uint32_t verbosity;

    Statistic<uint64_t>* statPrefetchOpportunities;
    Statistic<uint64_t>* statPrefetchEventsIssued;
    Statistic<uint64_t>* statPrefetchIssueCanceledByPageBoundary;
    Statistic<uint64_t>* statPrefetchIssueCanceledByHistory;
};

} //namespace Cassini
} //namespace SST

#endif
