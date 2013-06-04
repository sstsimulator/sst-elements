
#include <vector>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include "nbprefetch.h"
#include <stdint.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

NextBlockPrefetcher::NextBlockPrefetcher(Params& params) {
	blockSize = (uint64_t) params.find_integer("prefetcher:blocksize", 64);
}

NextBlockPrefetcher::~NextBlockPrefetcher() {

}

void NextBlockPrefetcher::notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr)
{
	if(notifyResType == MISS) {
		Addr nextBlockAddr = (addr - (addr % blockSize)) + blockSize;
		std::vector<std::pair<const SST::Component*,  void (*)(MemEvent*)> >::iterator callbackItr;

		// Cycle over each registered call back and notify them that we want to issue a prefetch request
		for(callbackItr = registeredCallbacks.begin(); callbackItr != registeredCallbacks.end(); callbackItr++) {
			// Create a new read request, we cannot issue a write because the data will get
			// overwritten and corrupt memory (even if we really do want to do a write)
			MemEvent* newEv = new MemEvent(callbackItr->first, nextBlockAddr, ReadReq);

			void (*callee)(MemEvent*) = callbackItr->second;
			callee(newEv);
		}
	}
}

void NextBlockPrefetcher::registerResponseCallback(const SST::Component* owner, void (*callee)(MemEvent* memEvent)) 
{
	registeredCallbacks.push_back(std::make_pair(owner, callee));
}
