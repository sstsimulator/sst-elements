
#ifndef _H_SST_SAVANNAH_ARBITRATOR
#define _H_SST_SAVANNAH_ARBITRATOR

#include <sst/elements/memHierarchy/memoryController.h>
#include <sst/elements/memHierarchy/membackend/memBackend.h>

#include <queue>

#include "savevent.h"

using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahIssueArbitrator : public SST::SubComponent {
public:
        SavannahIssueArbitrator(Component* comp, Params& params) : SubComponent(comp) {}
	~SavannahIssueArbitrator() {}
        virtual void issue(std::queue<SavannahRequestEvent*>& q, MemBackend* backend) = 0;
};

}
}

#endif
