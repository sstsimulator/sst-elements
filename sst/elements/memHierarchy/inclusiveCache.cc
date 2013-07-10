#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "cacheListener.h"
#include "bus.h"
#include "InclusiveCache.h"
#include "Cache.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

InclusiveCache::InclusiveCache(SST::ComponentId_t id, SST::Component::Params_t& params) :
		SST::MemHierarchy::Cache(id, params)
{
	//hi(5);

}

void InclusiveCache::hi(int i){
	std::cout << "HI " << std::endl;
}

InclusiveCache::~InclusiveCache(){}
