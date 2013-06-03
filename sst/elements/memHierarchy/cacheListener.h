
#ifndef _H_MEMHIERARCHY_CACHE_LISTENER
#define _H_MEMHIERARCHY_CACHE_LISTENER

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "cache.h"

using namespace SST;

namespace SST {
namespace MemHierarchy {

enum NotifyAccessType {
	READ,
	WRITE
}

enum NotifyResultType {
	HIT,
	MISS
}

class CacheListener : public Module {
    public:
	CacheListener() {}
	virtual ~CacheListener() {}

	virtual void notifyAccess(NotifyType notifyType, NotifyResultType notifyResType, Addr addr) { }
	virtual void registerResponseCallback(void (*callee)(MemEvent* memEvent)) { }
};

}
}

#endif

