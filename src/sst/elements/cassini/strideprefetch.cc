// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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


#define CASSINI_MIN(a,b) (((a)<(b)) ? a : b)

using namespace SST;
using namespace SST::Cassini;

void StridePrefetcher::notifyAccess(const CacheListenerNotification& notify) {
        const NotifyResultType notifyResType = notify.getResultType();
        const Addr addr = notify.getPhysicalAddress();

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

					statPrefetchOpportunities->addData(1);

					// Check next address is aligned to a cache line boundary
					assert((targetAddress + (strideReach * stride)) % blockSize == 0);

					ev = new MemEvent(parent, targetAddress + (strideReach * stride), targetAddress + (strideReach * stride), GetS);
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
						ev = new MemEvent(parent, targetPrefetchAddress, targetPrefetchAddress, GetS);
						statPrefetchOpportunities->addData(1);
					} else {
						output->verbose(CALL_INFO, 2, 0, "Cancel prefetch issue, request exceeds physical page limit\n");
						output->verbose(CALL_INFO, 4, 0, "Target address: %" PRIx64 ", page=%" PRIx64 ", Prefetch address: %" PRIx64 ", page=%" PRIx64 "\n", targetAddress, targetAddressPhysPage, targetPrefetchAddress, targetPrefetchAddressPage);

						statPrefetchIssueCanceledByPageBoundary->addData(1);
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
		statPrefetchEventsIssued->addData(1);

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
	            MemEvent* newEv = new MemEvent(parent, ev->getAddr(), ev->getAddr(), GetS);
            	    newEv->setSize(blockSize);
            	    newEv->setPrefetchFlag(true);

        	    (*(*callbackItr))(newEv);
	        }

        	delete ev;
	} else {
		statPrefetchIssueCanceledByHistory->addData(1);
		output->verbose(CALL_INFO, 2, 0, "Prefetch canceled - same cache line is found in the recent prefetch history.\n");
	        delete ev;
        }
    }
}

StridePrefetcher::StridePrefetcher(Component* owner, Params& params) : CacheListener(owner, params) {
	Simulation::getSimulation()->requireEvent("memHierarchy.MemEvent");

	verbosity = params.find<int>("verbose", 0);

	char* new_prefix = (char*) malloc(sizeof(char) * 128);
        sprintf(new_prefix, "StridePrefetcher[%s | @f:@p:@l] ", parent->getName().c_str());
	output = new Output(new_prefix, verbosity, 0, Output::STDOUT);
	free(new_prefix);

	recheckCountdown = 0;
        blockSize = params.find<uint64_t>("cache_line_size", 64);

	prefetchHistoryCount = params.find<uint32_t>("history", 16);
	prefetchHistory = new std::deque<uint64_t>();

	strideReach = params.find<uint32_t>("reach", 2);
        strideDetectionRange = params.find<uint64_t>("detect_range", 4);
	recentAddrListCount = params.find<uint32_t>("address_count", 64);
	pageSize = params.find<uint64_t>("page_size", 4096);

	uint32_t overrunPB = params.find<uint32_t>("overrun_page_boundaries", 0);
	overrunPageBoundary = (overrunPB == 0) ? false : true;

	nextRecentAddressIndex = 0;
	recentAddrList = (Addr*) malloc(sizeof(Addr) * recentAddrListCount);

	for(uint32_t i = 0; i < recentAddrListCount; ++i) {
		recentAddrList[i] = (Addr) 0;
	}

        output->verbose(CALL_INFO, 1, 0, "StridePrefetcher created, cache line: %" PRIu64 ", page size: %" PRIu64 "\n",
		blockSize, pageSize);

        missEventsProcessed = 0;
        hitEventsProcessed = 0;

	statPrefetchOpportunities = registerStatistic<uint64_t>("prefetch_opportunities");
	statPrefetchEventsIssued = registerStatistic<uint64_t>("prefetches_issued");
	statPrefetchIssueCanceledByPageBoundary = registerStatistic<uint64_t>("prefetches_canceled_by_page_boundary");
	statPrefetchIssueCanceledByHistory = registerStatistic<uint64_t>("prefetches_canceled_by_history");
}

StridePrefetcher::~StridePrefetcher() {
	free(recentAddrList);
}

void StridePrefetcher::registerResponseCallback(Event::HandlerBase* handler) {
	registeredCallbacks.push_back(handler);
}

void StridePrefetcher::printStats(Output &out) {
}
