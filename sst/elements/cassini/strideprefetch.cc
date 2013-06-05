
#include <vector>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include "strideprefetch.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Cassini;

void StridePrefetcher::notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr) {
	
}

StridePrefetcher::StridePrefetcher(Params& params) {
        blockSize = (uint64_t) params.find_integer("prefetcher:blocksize", 64);
}

void StridePrefetcher::setOwningComponent(const SST::Component* own) {
	owner = own;
}

void StridePrefetcher::registerResponseCallback(Event::HandlerBase* handler) {
	registeredCallbacks.push_back(handler);
}

