
#include <vector>
#include "stdlib.h"

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include "strideprefetch.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Cassini;

void StridePrefetcher::notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr) {
	if(notifyResType == MISS) {
		recentAddrList[nextRecentAddressIndex] = addr;
		nextRecentAddressIndex = (nextRecentAddressIndex + 1) % recentAddrListCount;

		std::cout << "Recent Address List:" << std::endl;
		for(uint32_t i = 0; i < recentAddrListCount; ++i) {
			std::cout << "-> Addr: " << recentAddrList[i] << std::endl;
		}
	}
}

StridePrefetcher::StridePrefetcher(Params& params) {
        blockSize = (uint64_t) params.find_integer("prefetcher:blocksize", 64);

	recentAddrListCount = (uint32_t) params.find_integer("strideprefetcher:addresscount", 64);
	nextRecentAddressIndex = 0;
	recentAddrList = (Addr*) malloc(sizeof(Addr) * recentAddrListCount);

	for(uint32_t i = 0; i < recentAddrListCount; ++i) {
		recentAddrList[i] = (Addr) 0;
	}
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

