
#include <vector>
#include "stdlib.h"

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/element.h"

#include "strideprefetch.h"

#define CASSINI_MIN(a,b) (((a)<(b)) ? a : b)

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Cassini;

void StridePrefetcher::notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr) {
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
	MemEvent* ev = NULL;
	uint32_t stride;
	int32_t addrStride;
	bool foundStride = true;
	Addr targetAddress = 0;
	uint32_t strideIndex;

	for(uint32_t i = 0; i < recentAddrListCount - 1; ++i) {
		for(uint32_t j = i + 1; j < recentAddrListCount; ++j) {
			stride = j - i;
			addrStride = getAddressByIndex(j) - getAddressByIndex(i);
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
				targetAddress += strideReach * stride;
				ev = new MemEvent(owner, targetAddress, RequestData);
				break;
			}
		}

		if(ev != NULL) {
			break;
		}
	}

	if(ev != NULL) {
		std::vector<Event::HandlerBase*>::iterator callbackItr;

		std::cout << "StridePrefetcher: created prefetch for address " <<
			ev->getAddr() << std::endl;
		prefetchEventsIssued++;

                // Cycle over each registered call back and notify them that we want to issue a prefet$
                for(callbackItr = registeredCallbacks.begin(); callbackItr != registeredCallbacks.end(); callbackItr++) {
                        // Create a new read request, we cannot issue a write because the data will get
                        // overwritten and corrupt memory (even if we really do want to do a write)
                        MemEvent* newEv = new MemEvent(owner, ev->getAddr(), RequestData);
            		newEv->setSize(blockSize);

                        (*(*callbackItr))(newEv);
                }
	}
}

StridePrefetcher::StridePrefetcher(Params& params) {
	recheckCountdown = 0;
        blockSize = (uint64_t) params.find_integer("prefetcher:blocksize", 64);

	strideReach = (uint32_t) params.find_integer("strideprefetcher:reach", 2);
        strideDetectionRange = (uint64_t) params.find_integer("strideprefetcher:detectrange", 4);
	recentAddrListCount = (uint32_t) params.find_integer("strideprefetcher:addresscount", 64);
	nextRecentAddressIndex = 0;
	recentAddrList = (Addr*) malloc(sizeof(Addr) * recentAddrListCount);

	for(uint32_t i = 0; i < recentAddrListCount; ++i) {
		recentAddrList[i] = (Addr) 0;
	}

        prefetchEventsIssued = 0;
        missEventsProcessed = 0;
        hitEventsProcessed = 0;
}

StridePrefetcher::~StridePrefetcher() {
	free(recentAddrList);	
}

void StridePrefetcher::setOwningComponent(const SST::Component* own) {
	owner = own;
}

void StridePrefetcher::registerResponseCallback(Event::HandlerBase* handler) {
	registeredCallbacks.push_back(handler);
}

void StridePrefetcher::printStats() {
	std::cout << "--------------------------------------------------------------------" << std::endl;
        std::cout << "Stride Prefetch Engine:" << std::endl;
        std::cout << "Cache Miss Events:         " << missEventsProcessed << std::endl;
        std::cout << "Cache Hit Events:          " << hitEventsProcessed << std::endl;
        std::cout << "Cache Miss Rate (%):       " << ((missEventsProcessed / ((double) (missEventsProcessed + hitEventsProcessed))) * 100.0) << std::endl;
        std::cout << "Cache Hit Rate (%):        " << ((hitEventsProcessed / ((double) (missEventsProcessed + hitEventsProcessed))) * 100.0) << std::endl;
        std::cout << "Prefetches Issued:         " << prefetchEventsIssued << std::endl;
        std::cout << "--------------------------------------------------------------------" << std::endl;
}
