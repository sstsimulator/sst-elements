
#ifndef _H_SST_NEXT_BLOCK_PREFETCH
#define _H_SST_NEXT_BLOCK_PREFETCH

#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

class NextBlockPrefetcher : public SST::MemHierarchy::CacheListener {
    public:
	NextBlockPrefetcher(Params& params);
        ~NextBlockPrefetcher();

        void notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr);
	void registerResponseCallback(const SST::Component* owner, Event::HandlerBase *handler);
    private:
	std::vector<std::pair<const SST::Component*, Event::HandlerBase*> > registeredCallbacks;
	uint64_t blockSize;
};

}
}

#endif
