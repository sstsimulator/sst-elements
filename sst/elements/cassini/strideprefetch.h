
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

#include <sst/core/output.h>

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
	void printStats(Output &out);

    private:
	Output* output;
	const SST::Component* owner;
        std::vector<Event::HandlerBase*> registeredCallbacks;
	std::deque<uint64_t>* prefetchHistory;
	uint32_t prefetchHistoryCount;
        uint64_t blockSize;
	bool overrunPageBoundary;
	uint64_t pageSize;
	Addr* recentAddrList;
	uint32_t recentAddrListCount;
	uint32_t nextRecentAddressIndex;
	void DetectStride();
	uint32_t strideDetectionRange;
	uint32_t strideReach;
	Addr getAddressByIndex(uint32_t index);
	uint32_t recheckCountdown;
        uint64_t prefetchEventsIssued;
        uint64_t missEventsProcessed;
        uint64_t hitEventsProcessed;
	uint64_t prefetchIssueCanceledByPageBoundary;
	uint32_t verbosity;
	uint64_t prefetchIssueCanceledByHistory;
	uint64_t prefetchOpportunities;
};

} //namespace Cassini
} //namespace SST

#endif
