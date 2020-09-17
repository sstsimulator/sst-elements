
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
#define VANADIS_SYSCALL_READLINK         4085
#define VANADIS_SYSCALL_UNAME            4122
#define VANADIS_SYSCALL_WRITEV           4146
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

	virtual void registerInitParameter( VanadisCPUOSInitParameter paramType, void* param_val ) {
		switch( paramType ) {
		case SYSCALL_INIT_PARAM_INIT_BRK:
			{
				uint64_t* param_val_64 = (uint64_t*) param_val;
				output->verbose(CALL_INFO, 8, 0, "set initial brk point (init) event (0x%llx)\n", (*param_val_64) );
				os_link->sendInitData( new VanadisSyscallInitBRKEvent( core_id, hw_thr, (*param_val_64) ) );
			}
			break;
		}
	}

	virtual void handleSysCall( VanadisSysCallInstruction* syscallIns ) {
		output->verbose(CALL_INFO, 8, 0, "System Call (syscall-ins: 0x%0llx)\n", syscallIns->getInstructionAddress() );

		const uint32_t hw_thr = syscallIns->getHWThread();

		// MIPS puts codes in GPR r2
		const uint16_t os_code_phys_reg = isaTable->getIntPhysReg(2);
		const uint64_t os_code = regFile->getIntReg<uint64_t>( os_code_phys_reg );

		output->verbose(CALL_INFO, 8, 0, "--> [SYSCALL-handler] syscall-ins: 0x%0llx / call-code: %" PRIu64 "\n",
			syscallIns->getInstructionAddress(), os_code);
		VanadisSyscallEvent* call_ev = nullptr;

		switch(os_code) {
		case VANADIS_SYSCALL_READLINK:
			{
                                const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t readlink_path = regFile->getIntReg<uint64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t readlink_buff_ptr = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int64_t readlink_size = regFile->getIntReg<uint64_t>( phys_reg_6 );

				call_ev = new VanadisSyscallReadLinkEvent( core_id, hw_thr, readlink_path, readlink_buff_ptr, readlink_size );
			}
			break;

		case VANADIS_SYSCALL_READ:
			{
                                const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t read_fd = regFile->getIntReg<int64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t read_buff_ptr = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int64_t read_count = regFile->getIntReg<int64_t>( phys_reg_6 );

				call_ev = new VanadisSyscallReadEvent( core_id, hw_thr, read_fd, read_buff_ptr, read_count );
			}
			break;
		case VANADIS_SYSCALL_ACCESS:
			{
                                const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t path_ptr = regFile->getIntReg<uint64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t access_mode = regFile->getIntReg<uint64_t>( phys_reg_5 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to access( 0x%llx, %" PRIu64 " )\n",
					path_ptr, access_mode);
				call_ev = new VanadisSyscallAccessEvent( core_id, hw_thr, path_ptr, access_mode );
			}
			break;
		case VANADIS_SYSCALL_BRK:
			{
				const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
				uint64_t newBrk = regFile->getIntReg<uint64_t>( phys_reg_4 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " )\n",
					newBrk);
				call_ev = new VanadisSyscallBRKEvent( core_id, hw_thr, newBrk );
			}
			break;
		case VANADIS_SYSCALL_SET_THREAD_AREA:
			{
				const uint64_t phys_reg_4 = isaTable->getIntPhysReg(4);
				uint64_t thread_area_ptr = regFile->getIntReg<uint64_t>( phys_reg_4 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to set_thread_area( value: %" PRIu64 " / 0x%llx )\n",
					thread_area_ptr, thread_area_ptr);

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

				writeSyscallResult( true );

				for( int i = 0; i < returnCallbacks.size(); ++i ) {
		                        returnCallbacks[i](hw_thr);
                		}
			}
			break;
		case VANADIS_SYSCALL_UNAME:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
				uint64_t uname_addr = regFile->getIntReg<uint64_t>( phys_reg_4 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to uname()\n");

				call_ev = new VanadisSyscallUnameEvent( core_id, hw_thr, uname_addr );
			}
			break;
		case VANADIS_SYSCALL_OPENAT:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
				uint64_t openat_dirfd = regFile->getIntReg<uint64_t>( phys_reg_4 );

				const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
				uint64_t openat_path_ptr = regFile->getIntReg<uint64_t>( phys_reg_5 );

				const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
				uint64_t openat_flags = regFile->getIntReg<uint64_t>( phys_reg_6 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to openat()\n");
				call_ev = new VanadisSyscallOpenAtEvent( core_id, hw_thr, openat_dirfd, openat_path_ptr, openat_flags );
			}
			break;
		case VANADIS_SYSCALL_WRITEV:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t writev_fd = regFile->getIntReg<int64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t writev_iovec_ptr = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int64_t writev_iovec_count = regFile->getIntReg<int64_t>( phys_reg_6);

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to writev( %" PRId64 ", 0x%llx, %" PRId64 " )\n",
					writev_fd, writev_iovec_ptr, writev_iovec_count);
				call_ev = new VanadisSyscallWritevEvent( core_id, hw_thr, writev_fd, writev_iovec_ptr, writev_iovec_count );
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

	void writeSyscallResult( const bool success ) {
		const uint64_t os_success = 0;
                const uint64_t os_failed  = 1;
		const uint16_t succ_reg = isaTable->getIntPhysReg(7);

		if( success ) {
			output->verbose(CALL_INFO, 8, 0, "syscall - generating successful (v: 0) result (isa-reg: 7, phys-reg: %" PRIu16 ")\n",
				succ_reg);

	                regFile->setIntReg( succ_reg, os_success );
		} else {
			output->verbose(CALL_INFO, 8, 0, "syscall - generating failed (v: 1) result (isa-reg: 7, phys-reg: %" PRIu16 ")\n",
				succ_reg);

	                regFile->setIntReg( succ_reg, os_failed );
		}
	}

	void recvOSEvent( SST::Event* ev ) {
		output->verbose(CALL_INFO, 8, 0, "Recv OS Event\n");

		VanadisSyscallResponse* os_resp = dynamic_cast<VanadisSyscallResponse*>( ev );

		if( nullptr == os_resp ) {
			output->fatal(CALL_INFO, -1, "Error - got a OS response, but cannot convert to a correct response.\n");
		}

		output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 "\n", os_resp->getReturnCode() );
		output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

		// Set up the return code (according to ABI, this goes in r2)
		const uint16_t rc_reg   = isaTable->getIntPhysReg(2);
		const int64_t  rc_val = (int64_t) os_resp->getReturnCode();
		regFile->setIntReg( rc_reg, rc_val );

		if( rc_val < 0 ) {
			writeSyscallResult( false );
		} else {
			// Generate correct markers for OS return code checks
			writeSyscallResult( os_resp->isSuccessful() );
		}

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
