
#ifndef _H_SST_STRIDE_PREFETCH
#define _H_SST_STRIDE_PREFETCH

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

class StridePrefetcher : public SST::MemHierarchy::CacheListener {
    public:
	StridePrefetcher(Params& params);
        ~StridePrefetcher();

        void setOwningComponent(const SST::Component* owner);
        void notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr);
        void registerResponseCallback(Event::HandlerBase *handler);
    private:
	const SST::Component* owner;
        std::vector<Event::HandlerBase*> registeredCallbacks;
        uint64_t blockSize;
};

}
}

#endif
