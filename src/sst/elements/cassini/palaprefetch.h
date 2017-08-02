// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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

struct StrideFilter {
    uint64_t lastAddress;
    int32_t lastStride;
    int32_t stride;
    PrefetcherState state;
};


class PalaPrefetcher : public SST::MemHierarchy::CacheListener {
    public:
        PalaPrefetcher(Component* owner, Params& params);
        ~PalaPrefetcher();

        void notifyAccess(const CacheListenerNotification& notify);
        void registerResponseCallback(Event::HandlerBase *handler);
        void printStats(Output &out);

    private:
        void     DispatchRequest(Addr targetAddress);

        Output* output;
        std::vector<Event::HandlerBase*> registeredCallbacks;
        std::deque<uint64_t>* prefetchHistory;
        uint32_t prefetchHistoryCount;
        uint64_t blockSize;
        uint64_t tagSize;
        bool overrunPageBoundary;
        uint64_t pageSize;
        std::unordered_map< uint64_t, StrideFilter >* recentAddrList;
        std::deque< std::unordered_map< uint64_t, StrideFilter >::iterator >* recentAddrListQueue;
        std::deque< std::unordered_map< uint64_t, StrideFilter >::iterator >::iterator it;

        uint32_t addressSize;
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
