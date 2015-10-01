
#ifndef _H_SST_SAVANNAH_COMP_H
#define _H_SST_SAVANNAH_COMP_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/memHierarchy/memoryController.h"
#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include "sst/elements/memHierarchy/memResponseHandler.h"
#include "sst/elements/memHierarchy/DRAMReq.h"

#include "savarb.h"
#include "savevent.h"

using namespace SST;
using namespace SST::MemHierarchy;

namespace SST {
namespace Savannah {

class SavannahComponent : public SST::Component, public MemResponseHandler {

public:
	SavannahComponent(ComponentId_t id, Params &params);
	void handleIncomingEvent(SST::Event* ev);
	bool tick(Cycle_t cycle);
	virtual void handleMemResponse(DRAMReq* resp);
	~SavannahComponent();

private:
	SavannahComponent();				// Serialization Only
	SavannahComponent(const SavannahComponent&);	// Do not impl.
	void operator=(const SavannahComponent&);	// Do not impl.

	SST::Link** incomingLinks;
	SavannahIssueArbitrator* arbitrator;
	std::map<DRAMReq*, SavannahRequestEvent*> linkRequestMap;
	std::queue<SavannahRequestEvent*> reqQueue;

	uint32_t    incomingLinkCount;
	Output*     output;
	MemBackend* backend_;
};

}
}

#endif
