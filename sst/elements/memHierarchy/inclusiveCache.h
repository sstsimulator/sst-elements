#ifndef _INCLUSIVECACHE_H
#define _INCLUSIVECACHE_H

//#include <deque>
//#include <map>
//#include <list>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

//#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include <sst/core/interfaces/memEvent.h>

//#include "memNIC.h"
#include <sst/core/params.h>
#include "cache.h"


using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class CacheListener;

class InclusiveCache : public  Cache {

private:
	void hi(int i);


public:
	InclusiveCache(SST::ComponentId_t id, SST::Component::Params_t& params);
	//virtual void init(unsigned int);
	//virtual void setup();
	//virtual void finish();
	virtual ~InclusiveCache();


};

}
}



#endif
