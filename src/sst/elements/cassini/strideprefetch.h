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
