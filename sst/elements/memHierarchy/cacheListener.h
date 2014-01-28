
#ifndef _H_MEMHIERARCHY_CACHE_LISTENER
#define _H_MEMHIERARCHY_CACHE_LISTENER

#include <sst/core/serialization.h>
#include <sst/core/simulation.h>
#include <sst/core/event.h>
#include <sst/core/module.h>

using namespace SST;
using namespace SST::Interfaces;

namespace SST {
class Output;

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

    virtual void printStats(Output &out) {}
    virtual void setOwningComponent(const SST::Component* owner) {}
    virtual void notifyAccess(NotifyAccessType notifyType, NotifyResultType notifyResType, Addr addr) { }
    virtual void registerResponseCallback(Event::HandlerBase *handler) { delete handler; }
};

}
}

#endif

