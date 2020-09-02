
#include <sst_config.h>
#include <sst/core/component.h>

#include <functional>

#include "os/voscallresp.h"
#include "os/voscallev.h"
#include "os/vnodeos.h"

using namespace SST::Vanadis;

VanadisNodeOSComponent::VanadisNodeOSComponent( SST::ComponentId_t id, SST::Params& params ) :
	SST::Component(id) {

	const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
	output = new SST::Output("[node-os]: ", verbosity, 0, SST::Output::STDOUT);

	const uint32_t core_count = params.find<uint32_t>("cores", 0);

	output->verbose(CALL_INFO, 1, 0, "Configuring the memory interface...\n");
	mem_if = loadUserSubComponent<Interfaces::SimpleMem>("mem_interface", ComponentInfo::SHARE_NONE, getTimeConverter("1ps"),
		new SimpleMem::Handler<SST::Vanadis::VanadisNodeOSComponent>( this, &VanadisNodeOSComponent::handleIncomingMemory ) );

	output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " core links...\n", core_count);
	core_links.reserve(core_count);

	char* port_name_buffer = new char[128];

	for( uint32_t i = 0; i < core_count; ++i ) {
		snprintf( port_name_buffer, 128, "core%" PRIu32 "", i );
		output->verbose(CALL_INFO, 1, 0, "---> processing link %s...\n", port_name_buffer);

		SST::Link* core_link = configureLink( port_name_buffer, "0ns",
			new Event::Handler<VanadisNodeOSComponent>( this, &VanadisNodeOSComponent::handleIncomingSysCall ) );

		if( nullptr == core_link ) {
			output->fatal(CALL_INFO, -1, "Error: unable to configure link: %s\n", port_name_buffer );
		} else {
			output->verbose(CALL_INFO, 8, 0, "configuring link %s...\n", port_name_buffer );
			core_links.push_back( core_link );
		}

		VanadisNodeOSCoreHandler* new_core_handler = new VanadisNodeOSCoreHandler( verbosity, i );
		new_core_handler->setLink( core_link );

		std::function<void( SimpleMem::Request*, uint32_t )> core_callback = std::bind( &VanadisNodeOSComponent::sendMemoryEvent,
			this, std::placeholders::_1, std::placeholders::_2 );
		new_core_handler->setSendMemoryCallback( core_callback );

		core_handlers.push_back( new_core_handler );
	}

	delete[] port_name_buffer;
}

VanadisNodeOSComponent::~VanadisNodeOSComponent() {
	for( VanadisNodeOSCoreHandler* next_core_handler : core_handlers ) {
		delete next_core_handler;
	}

	delete output;
}

void VanadisNodeOSComponent::init( unsigned int phase ) {
	if( 0 == phase ) {
		mem_if->init( phase );
	}
}

void VanadisNodeOSComponent::handleIncomingSysCall( SST::Event* ev ) {
	VanadisSyscallEvent* sys_ev = dynamic_cast<VanadisSyscallEvent*>( ev );

	if( nullptr == sys_ev ) {
		output->fatal(CALL_INFO, -1, "Error - received an event in the OS, but cannot cast it to a system-call event.\n");
	}

	output->verbose(CALL_INFO, 16, 0, "Call from core: %" PRIu32 ", thr: %" PRIu32 " -> calling handler...\n",
		sys_ev->getCoreID(), sys_ev->getThreadID());

	core_handlers[ sys_ev->getCoreID() ]->handleIncomingSyscall( sys_ev );
}

