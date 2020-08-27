
#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include <functional>
#include "os/vcpuos.h"

namespace SST {
namespace Vanadis {

class VanadisMIPSOSHandler : public VanadisCPUOSHandler {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisMIPSOSHandler,
		"vanadis",
		"VanadisMIPSOSHandler",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Provides SYSCALL handling for a MIPS-based decoding core",
		SST::Vanadis::VanadisCPUOSHandler
	)

	VanadisMIPSOSHandler( ComponentId_t id, Params& params ) :
		VanadisCPUOSHandler(id, params) {

		os_link = configureLink( "os_link", "0ns", new Event::Handler<VanadisMIPSOSHandler>(this,
			&VanadisMIPSOSHandler::recvOSEvent ) );
	}

	virtual ~VanadisMIPSOSHandler() {
		delete os_link;
	}

//	virtual void handleSysCall( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable, const uint64_t ip ) {
//		output->verbose(CALL_INFO, 8, 0, "System Call\n");
//	}
//
//	virtual void handleReturn( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable, VanadisSysCallResponse* resp ) {
//		output->verbose(CALL_INFO, 8, 0, "System Call Return\n");
//	}

	virtual void handleSysCall( VanadisSysCallInstruction* syscallIns ) {
		output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx)\n", syscallIns->getInstructionAddress() );
	}

protected:

	void recvOSEvent( SST::Event* ev ) {
		output->verbose(CALL_INFO, 8, 0, "Recv OS Event\n");

		for( int i = 0; i < returnCallbacks.size(); ++i ) {
			returnCallbacks[i](hw_thr);
		}
	}

	SST::Link* os_link;

};

}
}

#endif
