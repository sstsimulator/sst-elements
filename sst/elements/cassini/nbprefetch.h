
#ifndef _H_SST_NEXT_BLOCK_PREFETCH
#define _H_SST_NEXT_BLOCK_PREFETCH

#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/memEvent.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace std;

namespace SST {
namespace Cassini {

typedef struct {
	Addr memAddr;
	int size;
	int evID;
} RequestEntry;

class NextBlockPrefetcher : public SST::Component {

	public:
		NextBlockPrefetcher(SST::ComponentId_t id, SST::Component::Params_t& params);
		NextBlockPrefetcher(const NextBlockPrefetcher&);
		void operator=(const NextBlockPrefetcher&);

		bool clockTick(Cycle_t curCycle);
		void finish();

	private:
		SST::Link* cpuLink;
		SST::Link* memoryLink;

		SST::Link* cacheCPULink;
		SST::Link* cacheMemoryLink;

		set<Addr> pendingPrefAddr;
		uint32_t maximumPending;

		void handleCPULinkEvent(SST::Event* event);
		void handleMemoryLinkEvent(SST::Event* event);

		void handleCacheToCPUEvent(SST::Event* event);
		void handleCacheToMemoryEvent(SST::Event* event);

  		friend class boost::serialization::access;
  		template<class Archive>
  		void save(Archive & ar, const unsigned int version) const
  		{
	            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        	}

  		template<class Archive>
  		void load(Archive & ar, const unsigned int version) 
  		{
            		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  		}

  		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

}
}

#endif
