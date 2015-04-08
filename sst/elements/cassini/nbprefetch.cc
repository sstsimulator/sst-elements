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
	prefetchEventsIssued = 0;
	missEventsProcessed = 0;
	hitEventsProcessed = 0;
}

NextBlockPrefetcher::~NextBlockPrefetcher() {}

void NextBlockPrefetcher::notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType, const Addr addr, const uint32_t size)
{
	if(notifyResType == MISS) {
	    missEventsProcessed++;

	    Addr nextBlockAddr = (addr - (addr % blockSize)) + blockSize;
	    std::vector<Event::HandlerBase*>::iterator callbackItr;
	    prefetchEventsIssued++;

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
		hitEventsProcessed++;
	}
}

void NextBlockPrefetcher::registerResponseCallback(Event::HandlerBase *handler)  
{
	registeredCallbacks.push_back(handler);
}

void NextBlockPrefetcher::printStats(Output& out) {
	out.output("--------------------------------------------------------------------\n");
        out.output("Next Block Prefetch Engine Statistics (Cache: %s):\n", parent->getName().c_str());
        out.output("--------------------------------------------------------------------\n");
        out.output("Cache Miss Events:                      %" PRIu64 "\n", missEventsProcessed);
        out.output("Cache Hit Events :                      %" PRIu64 "\n", hitEventsProcessed);
        out.output("Cache Miss Rate (%%):                    %f\n", ((missEventsProcessed
                / ((double) (missEventsProcessed + hitEventsProcessed))) * 100.0));
        out.output("Cache Hit Rate (%%):                     %f\n", ((hitEventsProcessed / ((double) (missEventsProcessed +
                        hitEventsProcessed))) * 100.0));
        out.output("Prefetches Issued:                      %" PRIu64 "\n", prefetchEventsIssued);
        out.output("--------------------------------------------------------------------\n");
}
