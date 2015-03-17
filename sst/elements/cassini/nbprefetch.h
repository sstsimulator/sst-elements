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

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

class NextBlockPrefetcher : public SST::MemHierarchy::CacheListener {
    public:
	NextBlockPrefetcher(Params& params);
        ~NextBlockPrefetcher();

	void setOwningComponent(const SST::Component* owner);
        void notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType, const Addr addr, const uint32_t size);
	void registerResponseCallback(Event::HandlerBase *handler);
	void printStats(Output& out);

    private:
	const SST::Component* owner;
	std::vector<Event::HandlerBase*> registeredCallbacks;
	uint64_t blockSize;
	uint64_t prefetchEventsIssued;
	uint64_t missEventsProcessed;
	uint64_t hitEventsProcessed;
};

}
}

#endif
