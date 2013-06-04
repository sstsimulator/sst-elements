
#ifndef _H_MEMHIERARCHY_CACHE_LISTENER
#define _H_MEMHIERARCHY_CACHE_LISTENER

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/module.h>

using namespace SST;
using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class CacheListener : public Module {
public:
    enum NotifyAccessType {
        READ,
        WRITE
    };

    enum NotifyResultType {
        HIT,
        MISS
    };


    CacheListener() {}
    virtual ~CacheListener() {}

    virtual void setOwningComponent(const SST::Component* owner) {}
    virtual void notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr) { }
    virtual void registerResponseCallback(Event::HandlerBase *handler) { delete handler; }
};

}
}

#endif

