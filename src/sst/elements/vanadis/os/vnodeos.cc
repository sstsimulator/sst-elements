
#include <sst_config.h>
#include <sst/core/component.h>

#include "os/vnodeos.h"

using namespace SST::Vanadis;

VanadisNodeOSComponent::VanadisNodeOSComponent( SST::ComponentId_t id, SST::Params& params ) {

	const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
	output = new SST::Output("[node-os]: ", verbosity, 0, SST::Output::STDOUT);

	const uint32_t core_count = params.find<uint32_t>("cores", 0);

	output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " core links...\n", core_count);
	core_links.reserve(core_count);

}

VanadisNodeOSComponent::~VanadisNodeOSComponent() {
	delete output;

	for( SST::Link* next_link : core_links ) {
		if( nullptr != next_link ) {
			delete next_link;
		}
	}
}

void VanadisNodeOSComponent::init( unsigned int phase ) {

}

void VanadisNodeOSComponent::handleIncomingSysCall() {

}

void VanadisNodeOSComponent::handleIncomingMemory( SimpleMem::Request* ev ) {

}


