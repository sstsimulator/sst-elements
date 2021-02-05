
#ifndef _H_VANADIS_NODE_OS
#define _H_VANADIS_NODE_OS

#include <unordered_set>

#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>

#include "os/vnodeoshandler.h"
#include "os/callev/voscallall.h"

#include "os/memmgr/vmemmgr.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisNodeOSComponent : public SST::Component {

public:
	SST_ELI_REGISTER_COMPONENT(
        	VanadisNodeOSComponent,
       	 	"vanadis",
	        "VanadisNodeOS",
        	SST_ELI_ELEMENT_VERSION(1,0,0),
	        "Vanadis Generic Operating System Component",
        	COMPONENT_CATEGORY_PROCESSOR
    	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose", 		"Set the output verbosity, 0 is no output, higher is more." },
		{ "cores",		"Number of cores that can request OS services via a link."  },
		{ "stdout",		"File path to place stdout" },
		{ "stderr",		"File path to place stderr" },
		{ "stdin",		"File path to place stdin"  }
	)

	SST_ELI_DOCUMENT_PORTS(
		{ "core%(cores)d",	"Connects to a CPU core",	{} }
	)

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{ "mem_interface",      "Interface to memory system for data access",  "SST::Interfaces::SimpleMem" }
	)

	VanadisNodeOSComponent( SST::ComponentId_t id, SST::Params& params );
	~VanadisNodeOSComponent();

	virtual void init( unsigned int phase );
	void handleIncomingSysCall( SST::Event* ev );

	void handleIncomingMemory( SimpleMem::Request* ev ) {
		auto lookup_result = ev_core_map.find( ev->id );

		if( lookup_result == ev_core_map.end() ) {
			output->fatal( CALL_INFO, -1, "Error - received a call which does not have a mapping to a core.\n" );
		} else {
			output->verbose( CALL_INFO, 8, 0, "redirecting to core %" PRIu32 "...\n", lookup_result->second );
			core_handlers[ lookup_result->second ]->handleIncomingMemory( ev );
			ev_core_map.erase(lookup_result);
		}
	}

	void sendMemoryEvent( SimpleMem::Request* ev, uint32_t core ) {
		ev_core_map.insert( std::pair< SimpleMem::Request::id_t, uint32_t >( ev->id, core ) );
		mem_if->sendRequest(ev);
	}

	uint64_t getSimNanoSeconds() {
		return getCurrentSimTimeNano();
	}

private:
	VanadisNodeOSComponent();  // for serialization only
    	VanadisNodeOSComponent(const VanadisNodeOSComponent&); // do not implement
    	void operator=(const VanadisNodeOSComponent&); // do not implement

	std::function<uint64_t()> get_sim_nano;
	std::unordered_map< SimpleMem::Request::id_t, uint32_t > ev_core_map;
	std::vector< SST::Link* > core_links;
	std::vector< VanadisNodeOSCoreHandler* > core_handlers;

	SimpleMem* mem_if;
	VanadisMemoryManager* memory_mgr;

	SST::Output* output;
};

}
}

#endif
