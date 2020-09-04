
#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include <functional>
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include "os/callev/voscallall.h"
#include "os/voscallresp.h"

#define VANADIS_SYSCALL_READ             4003
#define VANADIS_SYSCALL_ACCESS           4033
#define VANADIS_SYSCALL_BRK              4045
#define VANADIS_SYSCALL_UNAME            4122
#define VANADIS_SYSCALL_SET_THREAD_AREA  4283
#define VANADIS_SYSCALL_RM_INOTIFY       4286
#define VANADIS_SYSCALL_OPENAT		 4288

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

		const uint32_t hw_thr = syscallIns->getHWThread();

		// MIPS puts codes in GPR r2
		const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(2);
		uint64_t os_code = *((uint64_t*) regFile->getIntReg( os_code_phys_reg ) );

		output->verbose(CALL_INFO, 8, 0, "--> [SYSCALL-handler] syscall-ins: 0x%0llx / call-code: %" PRIu64 "\n",
			syscallIns->getInstructionAddress(), os_code);
		VanadisSyscallEvent* call_ev = nullptr;

		switch(os_code) {
		case VANADIS_SYSCALL_READ:
			{
                                const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t read_fd = 0;
                                regFile->getIntReg( phys_reg_4, &read_fd );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t read_buff_ptr = 0;
                                regFile->getIntReg( phys_reg_5, &read_buff_ptr );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int64_t read_count = 0;
                                regFile->getIntReg( phys_reg_6, &read_count );

				call_ev = new VanadisSyscallReadEvent( core_id, hw_thr, read_fd, read_buff_ptr, read_count );
			}
			break;
		case VANADIS_SYSCALL_ACCESS:
			{
				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access()\n");
				call_ev = new VanadisSyscallAccessEvent( core_id, hw_thr );
			}
			break;
		case VANADIS_SYSCALL_BRK:
			{
				const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);

				uint64_t newBrk = 0;
				regFile->getIntReg( phys_reg_4, &newBrk );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " )\n",
					newBrk);
				call_ev = new VanadisSyscallBRKEvent( core_id, hw_thr, newBrk );
			}
			break;
		case VANADIS_SYSCALL_SET_THREAD_AREA:
			{
				const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);

				uint64_t thread_area_ptr = 0;
				regFile->getIntReg( phys_reg_4, &thread_area_ptr );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to set_thread_area( value: %" PRIu64 " )\n",
					thread_area_ptr);

				if( tls_address != nullptr ) {
					(*tls_address) = thread_area_ptr;
				}

				call_ev = new VanadisSyscallSetThreadAreaEvent( core_id, hw_thr, thread_area_ptr );
			}
			break;
		case VANADIS_SYSCALL_RM_INOTIFY:
			{
				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to inotify_rm_watch(), by-passing and removing.\n");
				const uint16_t rc_reg = isaTable->getIntPhysReg(7);
				regFile->setIntReg( rc_reg, (uint64_t) 0 );

				for( int i = 0; i < returnCallbacks.size(); ++i ) {
		                        returnCallbacks[i](hw_thr);
                		}
			}
			break;
		case VANADIS_SYSCALL_UNAME:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);

				uint64_t uname_addr = 0;
				regFile->getIntReg( phys_reg_4, &uname_addr );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to uname()\n");

				call_ev = new VanadisSyscallUnameEvent( core_id, hw_thr, uname_addr );
			}
			break;
		case VANADIS_SYSCALL_OPENAT:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
				uint64_t openat_dirfd = 0;
				regFile->getIntReg( phys_reg_4, &openat_dirfd );

				const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
				uint64_t openat_path_ptr = 0;
				regFile->getIntReg( phys_reg_5, &openat_path_ptr );

				const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
				uint64_t openat_flags = 0;
				regFile->getIntReg( phys_reg_6, &openat_flags );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to openat()\n");
				call_ev = new VanadisSyscallOpenAtEvent( core_id, hw_thr, openat_dirfd, openat_path_ptr, openat_flags );
			}
			break;

		default:
			output->fatal(CALL_INFO, -1, "[syscall-handler] Error: unknown code %" PRIu64 "\n", os_code);
			break;
		}

		if( nullptr != call_ev ) {
			output->verbose(CALL_INFO, 8, 0, "Sending event to operating system...\n");
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

		output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 "\n", os_resp->getReturnCode() );
		output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

		// Set up the return code (according to ABI, this goes in r2)
		const uint16_t rc_reg = isaTable->getIntPhysReg(2);
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
