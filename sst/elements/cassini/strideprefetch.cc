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
#include "strideprefetch.h"

#include <vector>
#include "stdlib.h"

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/serialization.h"


#define CASSINI_MIN(a,b) (((a)<(b)) ? a : b)

using namespace SST;
using namespace SST::Cassini;

void StridePrefetcher::notifyAccess(const NotifyAccessType notifyType, const NotifyResultType notifyResType, const Addr addr, const uint32_t size) {
	// Put address into our recent address list
	recentAddrList[nextRecentAddressIndex] = addr;
	nextRecentAddressIndex = (nextRecentAddressIndex + 1) % recentAddrListCount;

	recheckCountdown = (recheckCountdown + 1) % strideDetectionRange;

	notifyResType == MISS ? missEventsProcessed++ : hitEventsProcessed++;

	if(recheckCountdown == 0)
		DetectStride();
}

Addr StridePrefetcher::getAddressByIndex(uint32_t index) {
	return recentAddrList[(nextRecentAddressIndex + 1 + index) % recentAddrListCount];
}

void StridePrefetcher::DetectStride() {
        /*  Needs to be updated with current MemHierarchy Commands/States, MemHierarchyInterface */
	MemEvent* ev = NULL;
	uint32_t stride;
	bool foundStride = true;
	Addr targetAddress = 0;
	uint32_t strideIndex;

	for(uint32_t i = 0; i < recentAddrListCount - 1; ++i) {
		for(uint32_t j = i + 1; j < recentAddrListCount; ++j) {
			stride = j - i;
			strideIndex = 1;
			foundStride = true;

			for(uint32_t k = j + stride; k < recentAddrListCount; k += stride, strideIndex++) {
				targetAddress = getAddressByIndex(k);

				if(getAddressByIndex(k) - getAddressByIndex(i) != (strideIndex * stride)) {
					foundStride = false;
					break;
				}
			}

			if(foundStride) {
				Addr targetPrefetchAddress = targetAddress + (strideReach * stride);
				targetPrefetchAddress = targetPrefetchAddress - (targetPrefetchAddress % blockSize);

				if(overrunPageBoundary) {
					output->verbose(CALL_INFO, 2, 0, 
						"Issue prefetch, target address: %" PRIx64 ", prefetch address: %" PRIx64 " (reach out: %" PRIu32 ", stride=%" PRIu32 "), prefetchAddress=%" PRIu64 "\n",
						targetAddress, targetAddress + (strideReach * stride),
						(strideReach * stride), stride, targetPrefetchAddress);

					prefetchOpportunities++;

					// Check next address is aligned to a cache line boundary
					assert((targetAddress + (strideReach * stride)) % blockSize == 0);

					ev = new MemEvent(owner, targetAddress + (strideReach * stride), targetAddress + (strideReach * stride), GetS);
				} else {
					const Addr targetAddressPhysPage = targetAddress / pageSize;
					const Addr targetPrefetchAddressPage = targetPrefetchAddress / pageSize;

					// Check next address is aligned to a cache line boundary
					assert(targetPrefetchAddress % blockSize == 0);

					// if the address we found and the next prefetch address are on the same
					// we can safely prefetch without causing a page fault, otherwise we
					// choose to not prefetch the address
					if(targetAddressPhysPage == targetPrefetchAddressPage) {
					    output->verbose(CALL_INFO, 2, 0, "Issue prefetch, target address: %" PRIx64 ", prefetch address: %" PRIx64 " (reach out: %" PRIu32 ", stride=%" PRIu32 ")\n",
							targetAddress, targetPrefetchAddress, (strideReach * stride), stride);
						ev = new MemEvent(owner, targetPrefetchAddress, targetPrefetchAddress, GetS);
						prefetchOpportunities++;
					} else {
						output->verbose(CALL_INFO, 2, 0, "Cancel prefetch issue, request exceeds physical page limit\n");
						output->verbose(CALL_INFO, 4, 0, "Target address: %" PRIx64 ", page=%" PRIx64 ", Prefetch address: %" PRIx64 ", page=%" PRIx64 "\n", targetAddress, targetAddressPhysPage, targetPrefetchAddress, targetPrefetchAddressPage);

						prefetchIssueCanceledByPageBoundary++;
						ev = NULL;
					}
				}

				break;
			}
		}

		if(ev != NULL) {
			break;
		}
	}

    if(ev != NULL) {
        std::vector<Event::HandlerBase*>::iterator callbackItr;

	Addr prefetchCacheLineBase = ev->getAddr() - (ev->getAddr() % blockSize);
	bool inHistory = false;
	const uint32_t currentHistCount = prefetchHistory->size();

	output->verbose(CALL_INFO, 2, 0, "Checking prefetch history for cache line at base %" PRIx64 ", valid prefetch history entries=%" PRIu32 "\n", prefetchCacheLineBase,
		currentHistCount);

	for(uint32_t i = 0; i < currentHistCount; ++i) {
		if(prefetchHistory->at(i) == prefetchCacheLineBase) {
			inHistory = true;
			break;
		}
	}

	if(! inHistory) {
	        prefetchEventsIssued++;

		// Remove the oldest cache line
		if(currentHistCount == prefetchHistoryCount) {
			prefetchHistory->pop_front();
		}

		// Put the cache line at the back of the queue
		prefetchHistory->push_back(prefetchCacheLineBase);

		assert((ev->getAddr() % blockSize) == 0);

	        // Cycle over each registered call back and notify them that we want to issue a prefet$
	        for(callbackItr = registeredCallbacks.begin(); callbackItr != registeredCallbacks.end(); callbackItr++) {
	            // Create a new read request, we cannot issue a write because the data will get
	            // overwritten and corrupt memory (even if we really do want to do a write)
	            MemEvent* newEv = new MemEvent(owner, ev->getAddr(), ev->getAddr(), GetS);
            	    newEv->setSize(blockSize);
            	    newEv->setPrefetchFlag(true);

        	    (*(*callbackItr))(newEv);
	        }

        	delete ev;
	} else {
		prefetchIssueCanceledByHistory++;
		output->verbose(CALL_INFO, 2, 0, "Prefetch canceled - same cache line is found in the recent prefetch history.\n");
	}
    }
}

StridePrefetcher::StridePrefetcher(Params& params) {
	Simulation::getSimulation()->requireEvent("memHierarchy.MemEvent");

	verbosity = params.find_integer("verbose", 0);
	output = new Output("StridePrefetcher: ", verbosity, 0, Output::STDOUT);

	recheckCountdown = 0;
        blockSize = (uint64_t) params.find_integer("cache_line_size", 64);

	prefetchHistoryCount = (uint32_t) params.find_integer("history", 16);
	prefetchHistory = new std::deque<uint64_t>();

	strideReach = (uint32_t) params.find_integer("reach", 2);
        strideDetectionRange = (uint64_t) params.find_integer("detect_range", 4);
	recentAddrListCount = (uint32_t) params.find_integer("address_count", 64);
	pageSize = (uint64_t) params.find_integer("page_size", 4096);

	uint32_t overrunPB = (uint32_t) params.find_integer("overrun_page_boundaries", 0);
	overrunPageBoundary = (overrunPB == 0) ? false : true;

	nextRecentAddressIndex = 0;
	recentAddrList = (Addr*) malloc(sizeof(Addr) * recentAddrListCount);

	for(uint32_t i = 0; i < recentAddrListCount; ++i) {
		recentAddrList[i] = (Addr) 0;
	}

        output->verbose(CALL_INFO, 1, 0, "StridePrefetcher created, cache line: %" PRIu64 ", page size: %" PRIu64 "\n",
		blockSize, pageSize);

        prefetchEventsIssued = 0;
        missEventsProcessed = 0;
        hitEventsProcessed = 0;
	prefetchIssueCanceledByPageBoundary = 0;
	prefetchIssueCanceledByHistory = 0;
	prefetchOpportunities = 0;
}

StridePrefetcher::~StridePrefetcher() {
	free(recentAddrList);
}

void StridePrefetcher::setOwningComponent(const SST::Component* own) {
	owner = own;

	char* new_prefix = (char*) malloc(sizeof(char) * 128);
	sprintf(new_prefix, "StridePrefetcher[%s | @f:@p:@l] ", owner->getName().c_str());
	output->setPrefix(new_prefix);
}

void StridePrefetcher::registerResponseCallback(Event::HandlerBase* handler) {
	registeredCallbacks.push_back(handler);
}

void StridePrefetcher::printStats(Output &out) {
	out.output("--------------------------------------------------------------------\n");
	out.output("Stride Prefetch Engine Statistics (Owner: %s):\n", owner->getName().c_str());
	out.output("--------------------------------------------------------------------\n");
	out.output("Cache Miss Events:                      %" PRIu64 "\n", missEventsProcessed);
	out.output("Cache Hit Events :                      %" PRIu64 "\n", hitEventsProcessed);
	out.output("Cache Miss Rate (%%):                    %f\n", ((missEventsProcessed
               	/ ((double) (missEventsProcessed + hitEventsProcessed))) * 100.0));
	out.output("Cache Hit Rate (%%):                     %f\n", ((hitEventsProcessed / ((double) (missEventsProcessed +
                       	hitEventsProcessed))) * 100.0));
        out.output("Prefetches Opportunities:               %" PRIu64 "\n", prefetchOpportunities);
        out.output("Prefetches Issued:                      %" PRIu64 "\n", prefetchEventsIssued);
	out.output("Prefetches canceled by page boundary:   %" PRIu64 "\n", prefetchIssueCanceledByPageBoundary);
	out.output("Prefetches canceled by history:         %" PRIu64 "\n", prefetchIssueCanceledByHistory);
	out.output("--------------------------------------------------------------------\n");
}
