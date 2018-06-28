// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_STRIDE_PREFETCH
#define _H_SST_STRIDE_PREFETCH

#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>
#include <sst/core/elementinfo.h>

#include <sst/core/output.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

class StridePrefetcher : public SST::MemHierarchy::CacheListener {
public:
    StridePrefetcher(Component* owner, Params& params);
    ~StridePrefetcher();

    void notifyAccess(const CacheListenerNotification& notify);
    void registerResponseCallback(Event::HandlerBase *handler);
    void printStats(Output &out);

    SST_ELI_REGISTER_SUBCOMPONENT(
        StridePrefetcher,
            "cassini",
            "StridePrefetcher",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Stride Detection Prefetcher",
            "SST::Cassini::CacheListener"
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose", "Controls the verbosity of the Cassini component", "0" },
            { "cache_line_size", "Size of the cache line the prefetcher is attached to", "64" },
        { "history", "Number of entries to keep for historical comparison", "16" },
        { "reach", "Reach (how far forward the prefetcher should fetch lines)", "2" },
        { "detect_range", "Range to detect addresses over in request counts", "4" },
        { "address_count", "Number of addresses to keep in prefetch table", "64" },
        { "page_size", "Page size for this controller", "4096" },
        { "overrun_page_boundaries", "Allow prefetcher to run over page boundaries, 0 is no, 1 is yes", "0" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
            { "prefetches_issued", "Number of prefetch requests issued", "prefetches", 1 },
        { "prefetches_canceled_by_page_boundary",
                "Prefetches which would not be executed because they span over a page boundary.", "prefetches", 1 },
        { "prefetches_canceled_by_history",
                "Prefetches which did not get issued because of a prefetch history in the table", "prefetches", 1 },
        { "prefetch_opportunities", "Count of opportunities to prefetch", "prefetches", 1 }
    )

private:
    Output* output;
    std::vector<Event::HandlerBase*> registeredCallbacks;
    std::deque<uint64_t>* prefetchHistory;
    uint32_t prefetchHistoryCount;
    uint64_t blockSize;
    bool overrunPageBoundary;
    uint64_t pageSize;
    Addr* recentAddrList;
    uint32_t recentAddrListCount;
    uint32_t nextRecentAddressIndex;
    void DetectStride();
    uint32_t strideDetectionRange;
    uint32_t strideReach;
    Addr getAddressByIndex(uint32_t index);
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
