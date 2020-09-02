
#ifndef _H_VANADIS_NODE_OS_CORE_HANDLER
#define _H_VANADIS_NODE_OS_CORE_HANDLER

#include <cstdint>
#include <cinttypes>

#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/output.h>

#include "os/callev/voscallall.h"
#include "os/voscallresp.h"
#include "os/voscallev.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisNodeOSCoreHandler {
public:
	VanadisNodeOSCoreHandler( uint32_t verbosity, uint32_t core ):
		core_id(core) {
		core_link = nullptr;

		char* out_prefix = new char[64];
		snprintf( out_prefix, 64, "[node-os-core-%6" PRIu32 "]: ", core );
		output = new SST::Output( out_prefix, verbosity, 0, SST::Output::STDOUT );
	}

	~VanadisNodeOSCoreHandler() {
		delete output;
	}

	void setLink( SST::Link* new_link ) {  core_link = new_link; }

	uint32_t getCoreID()   const { return core_id;   }

	void handleIncomingSyscall( VanadisSyscallEvent* sys_ev ) {

		switch( sys_ev->getOperation() ) {
		case SYSCALL_OP_UNAME:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is uname()\n");
				VanadisSyscallUnameEvent* uname_ev = dynamic_cast<VanadisSyscallUnameEvent*>( sys_ev );

				if( nullptr == uname_ev ) {
					output->fatal(CALL_INFO, -1, "-> error: unable to case syscall to a uname event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "---> uname struct is at address 0x%0llx\n", uname_ev->getUnameInfoAddress());

			}
			break;

		default:
			{
				// Send default response
				VanadisSyscallResponse* resp = new VanadisSyscallResponse();
				core_link->send( resp );
			}
			break;
		}

	}

	void handleIncomingMemory( SimpleMem::Request* ev ) {
		delete ev;
	}

	void setSendMemoryCallback( std::function<void( SimpleMem::Request*, uint32_t )> callback ) {
		sendMemEventCallback = callback;
	}

protected:
	std::function<void( SimpleMem::Request*, uint32_t )> sendMemEventCallback;

	SST::Link* core_link;
	SST::Output* output;
	const uint32_t core_id;
};

}
}

#endif
