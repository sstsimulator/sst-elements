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


#ifndef _H_SST_NEXT_BLOCK_PREFETCH
#define _H_SST_NEXT_BLOCK_PREFETCH

#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>
#include <sst/core/elementinfo.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

class NextBlockPrefetcher : public SST::MemHierarchy::CacheListener {
public:
    NextBlockPrefetcher(Component* owner, Params& params);
    ~NextBlockPrefetcher();

    void notifyAccess(const CacheListenerNotification& notify);
    void registerResponseCallback(Event::HandlerBase *handler);
    void printStats(Output& out);

    SST_ELI_REGISTER_SUBCOMPONENT(
        NextBlockPrefetcher,
        "cassini",
        "NextBlockPrefetcher",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Next Block Prefetcher",
        "SST::Cassini::CacheListener"
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "cache_line_size", "Size of the cache line the prefetcher is attached to", "64" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "prefetches_issued", "Number of prefetch requests issued", "prefetches", 1 },
        { "miss_events_processed", "Number of cache misses received", "misses", 2 },
        { "hit_events_processed", "Number of cache hits received", "hits", 2 }
    )

private:
    std::vector<Event::HandlerBase*> registeredCallbacks;
    uint64_t blockSize;

    Statistic<uint64_t>* statPrefetchEventsIssued;
    Statistic<uint64_t>* statMissEventsProcessed;
    Statistic<uint64_t>* statHitEventsProcessed;

};

}
}

#endif
