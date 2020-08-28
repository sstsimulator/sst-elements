
#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include <functional>
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include "os/voscallresp.h"
#include "os/voscallaccessev.h"
#include "os/voscallbrk.h"
#include "os/vosresp.h"

#define VANADIS_SYSCALL_ACCESS 4033
#define VANADIS_SYSCALL_BRK    4045

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

	SST_ELI_DOCUMENT_PORTS(
		{ "os_link", "Connects this handler to the main operating system of the node", {} }
	)

	VanadisMIPSOSHandler( ComponentId_t id, Params& params ) :
		VanadisCPUOSHandler(id, params) {

		os_link = configureLink( "os_link", "0ns", new Event::Handler<VanadisMIPSOSHandler>(this,
			&VanadisMIPSOSHandler::recvOSEvent ) );
	}

	virtual ~VanadisMIPSOSHandler() {
	}

	virtual void handleSysCall( VanadisSysCallInstruction* syscallIns ) {
		output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx)\n", syscallIns->getInstructionAddress() );

		// MIPS puts codes in GPR r2
		const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(2);
		uint64_t os_code = *((uint64_t*) regFile->getIntReg( os_code_phys_reg ) );

		output->verbose(CALL_INFO, 8, 0, "--> [syscall-handler] call-code: %" PRIu64 "\n", os_code);
		VanadisSyscallEvent* call_ev = nullptr;

		switch(os_code) {
		case VANADIS_SYSCALL_ACCESS:
			{
				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access()\n");
				call_ev = new VanadisSyscallAccessEvent( core_id, hw_thr );
				break;
			}
		case VANADIS_SYSCALL_BRK:
			{
				const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
				const uint64_t newBrk = *((uint64_t*) regFile->getIntReg( phys_reg_4 ) );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " )\n",
					newBrk);
				call_ev = new VanadisSyscallBRKEvent( core_id, hw_thr, newBrk );
			}
			break;
		default:
			output->fatal(CALL_INFO, -1, "[syscall-handler] Error: unknown code %" PRIu64 "\n", os_code);
			break;
		}

		if( nullptr != call_ev ) {
			os_link->send( call_ev );
		}
	}

protected:

	void recvOSEvent( SST::Event* ev ) {
		output->verbose(CALL_INFO, 8, 0, "Recv OS Event\n");

		VanadisSyscallResponse* os_resp = dynamic_cast<VanadisSyscallResponse*>( ev );

		if( nullptr == os_resp ) {
			output->fatal(CALL_INFO, -1, "Error - got a OS response, but cannot convert to a correct response.\n");
		}

		output->verbose(CALL_INFO, 8, 0, "syscall return-code: %d\n", os_resp->getReturnCode() );
		output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

		// Set up the return code (according to ABI, this goes in r7)
		const uint16_t rc_reg = isaTable->getIntPhysReg(7);
		const int64_t  rc_val = (int64_t) os_resp->getReturnCode();
		regFile->setIntReg( rc_reg, rc_val );

		for( int i = 0; i < returnCallbacks.size(); ++i ) {
			returnCallbacks[i](hw_thr);
		}

		delete ev;
	}

	SST::Link* os_link;

};

}
}

#endif
