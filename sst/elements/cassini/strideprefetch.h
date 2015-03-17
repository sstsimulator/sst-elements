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
	StridePrefetcher(Params& params);
        ~StridePrefetcher();

        void setOwningComponent(const SST::Component* owner);
        void notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType, const Addr addr, const uint32_t size);
        void registerResponseCallback(Event::HandlerBase *handler);
	void printStats(Output &out);

    private:
	Output* output;
	const SST::Component* owner;
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
        uint64_t prefetchEventsIssued;
        uint64_t missEventsProcessed;
        uint64_t hitEventsProcessed;
	uint64_t prefetchIssueCanceledByPageBoundary;
	uint32_t verbosity;
	uint64_t prefetchIssueCanceledByHistory;
	uint64_t prefetchOpportunities;
};

} //namespace Cassini
} //namespace SST

#endif
