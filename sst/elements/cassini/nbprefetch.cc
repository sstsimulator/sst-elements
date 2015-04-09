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

#include "sst_config.h"
#include "nbprefetch.h"

#include <stdint.h>
#include <vector>

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/serialization.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

NextBlockPrefetcher::NextBlockPrefetcher(Component* owner, Params& params) : CacheListener(owner, params) {
	Simulation::getSimulation()->requireEvent("memHierarchy.MemEvent");

	blockSize = (uint64_t) params.find_integer("cache_line_size", 64);

	statPrefetchEventsIssued = registerStatistic<uint64_t>("prefetches_issued");
	statMissEventsProcessed  = registerStatistic<uint64_t>("miss_events_processed");
	statHitEventsProcessed   = registerStatistic<uint64_t>("hit_events_processed");
}

NextBlockPrefetcher::~NextBlockPrefetcher() {}

void NextBlockPrefetcher::notifyAccess(const CacheListenerNotification& notify) {
	notifyAccess(notify.getAccessType(),
		notify.getResultType(),
		notify.getPhysicalAddress(),
		notify.getSize());
}

void NextBlockPrefetcher::notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType, const Addr addr, const uint32_t size)
{
	if(notifyResType == MISS) {
	    	statMissEventsProcessed->addData(1);

	    	Addr nextBlockAddr = (addr - (addr % blockSize)) + blockSize;
	    	std::vector<Event::HandlerBase*>::iterator callbackItr;
	    	statPrefetchEventsIssued->addData(1);

		// Cycle over each registered call back and notify them that we want to issue a prefetch request
		for(callbackItr = registeredCallbacks.begin(); callbackItr != registeredCallbacks.end(); callbackItr++) {
			// Create a new read request, we cannot issue a write because the data will get
			// overwritten and corrupt memory (even if we really do want to do a write)
            		MemEvent* newEv = new MemEvent(parent, nextBlockAddr, nextBlockAddr, GetS);
            		newEv->setSrc("Prefetcher");
            		newEv->setSize(blockSize);
            		newEv->setPrefetchFlag(true);
			(*(*callbackItr))(newEv);
		}
	} else {
		statHitEventsProcessed->addData(1);
	}
}

void NextBlockPrefetcher::registerResponseCallback(Event::HandlerBase *handler) {
	registeredCallbacks.push_back(handler);
}

void NextBlockPrefetcher::printStats(Output& out) {

}
