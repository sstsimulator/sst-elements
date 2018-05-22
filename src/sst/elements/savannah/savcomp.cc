// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <savcomp.h>

using namespace SST;
using namespace SST::Savannah;

#if 0
bool SavannahComponent::tick(Cycle_t cycle) {
	// Poll each link ONCE for a new pending event
	for(uint32_t i = 0; i < incomingLinkCount; i++) {
		Event* ev = incomingLinks[i]->recv();

		// NULL if nothing on the link to recv
		if(NULL == ev) {
			SavannahRequestEvent* savEv = dynamic_cast<SavannahRequestEvent*>(ev);
			reqQueue.push(savEv);

			// Set the link we are polling event from
			savEv->setLink(i);

			linkRequestMap.insert(std::pair<DRAMReq*,
				SavannahRequestEvent*>(savEv->getRequestPtr(), savEv));
		}
	}

	// Provide queue and backend to arbitrator to decide how to issue requests
	arbitrator->issue(reqQueue, backend_);

	// Keep ticking me
	return false;
}

void SavannahComponent::handleMemResponse(DRAMReq* resp) {
	std::map<DRAMReq*, SavannahRequestEvent*>::iterator respMatch =
		linkRequestMap.find(resp);

	if(linkRequestMap.end() == respMatch) {
		// Not found.
		output->fatal(CALL_INFO, -1, "Response was not found in request table.\n");
	}

	// Find the link and then return to the caller
	SavannahRequestEvent* respEv = respMatch->second;
	incomingLinks[respEv->getLink()]->send(respEv);
}
#endif

SavannahComponent::SavannahComponent(ComponentId_t id, Params &params) :
	Component(id) {

#if 0
	const int verbose = params.find("verbose", 0);
	output = new SST::Output("Savannah[@p:@l]: ",
		verbose, 0, SST::Output::STDOUT);

	std::string backend = params.find<std::string>("backend", "memHierarchy.simpleMem");
	output->verbose(CALL_INFO, 1, 0, "Loading backend: %s ...\n", backend.c_str());

	Params backendParams = params.find_prefix_params("backend.");
	backend_ = static_cast<MemBackend*>(loadSubComponent(backend, this, backendParams));

	if(NULL == backend_) {
		output->fatal(CALL_INFO, -1, "Error: unable to load backend %s.\n",
			backend.c_str());
	} else {
		output->verbose(CALL_INFO, 1, 0, "Backend loaded successfully.\n");
	}

	std::string arbModule = params.find<std::string>("arbitrator", "savannah.SavannahFIFOArbitrator");
	Params arbParams = params.find_prefix_params("arbitrator.");

	output->verbose(CALL_INFO, 1, 0, "Loading arbitrator: %s ...\n", arbModule.c_str());
	arbitrator = static_cast<SavannahIssueArbitrator*>(loadSubComponent(arbModule, this, arbParams));
	if(NULL == arbitrator) {
		output->fatal(CALL_INFO, -1, "Error: unable to load arbitrator: %s\n", arbModule.c_str());
	} else {
		output->verbose(CALL_INFO, 1, 0, "Loaded arbitrator (%s) successfully.\n", arbModule.c_str());
	}

	incomingLinkCount = (uint32_t) params.find("link_count", 0);
	output->verbose(CALL_INFO, 1, 0, "Will search for %" PRIu32 " links.", incomingLinkCount);

	incomingLinks = (SST::Link**) malloc( sizeof(SST::Link*) * incomingLinkCount);
	char* linkNameBuffer = (char*) malloc( sizeof(char) * 128 );

	for(uint32_t i = 0; i < incomingLinkCount; i++) {
		sprintf(linkNameBuffer, "link%" PRIu32, i);
		incomingLinks[i] = configureLink(linkNameBuffer);

		if(NULL == incomingLinks[i]) {
			output->fatal(CALL_INFO, -1, "Link: %s could not be configured.\n",
				linkNameBuffer);
		}
	}

	free(linkNameBuffer);

	output->verbose(CALL_INFO, 1, 0, "Link configuration completed.\n");

	std::string pollClock = params.find<std::string>("clock", "625MHz");
	output->verbose(CALL_INFO, 1, 0, "Register clock at %s\n", pollClock.c_str());
	registerClock( pollClock, new Clock::Handler<SavannahComponent>(this, &SavannahComponent::tick) );
	output->verbose(CALL_INFO, 1, 0, "Clock registration done.\n");

	// Tell user we are all done
	output->verbose(CALL_INFO, 1, 0, "Initialization of Savannah complete.\n");
#endif
}

#if 0
SavannahComponent::~SavannahComponent() {
	delete backend_;
	delete output;
}
#endif
