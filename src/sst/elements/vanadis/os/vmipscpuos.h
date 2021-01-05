
#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include <functional>
#include "os/vcpuos.h"
#include "os/voscallev.h"
#include "os/callev/voscallall.h"
#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"

#define VANADIS_SYSCALL_READ             4003
#define VANADIS_SYSCALL_WRITE		 4004
#define VANADIS_SYSCALL_ACCESS           4033
#define VANADIS_SYSCALL_BRK              4045
#define VANADIS_SYSCALL_IOCTL		 4054
#define VANADIS_SYSCALL_READLINK         4085
#define VANADIS_SYSCALL_MMAP		 4090
#define VANADIS_SYSCALL_UNAME            4122
#define VANADIS_SYSCALL_WRITEV           4146
#define VANADIS_SYSCALL_RT_SETSIGMASK    4195
#define VANADIS_SYSCALL_MMAP2            4210
#define VANADIS_SYSCALL_FSTAT		 4215
#define VANADIS_SYSCALL_MADVISE          4218
#define VANADIS_SYSCALL_FUTEX		 4238
#define VANADIS_SYSCALL_SET_TID		 4252
#define VANADIS_SYSCALL_EXIT_GROUP       4246
#define VANADIS_SYSCALL_SET_THREAD_AREA  4283
#define VANADIS_SYSCALL_RM_INOTIFY       4286
#define VANADIS_SYSCALL_OPENAT		 4288
#define VANADIS_SYSCALL_GETTIME64        4403

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

	SST_ELI_DOCUMENT_PARAMS(
		{ "brk_zero_memory", 	"Zero memory during OS calls to brk",	"0" }
	)

	VanadisMIPSOSHandler( ComponentId_t id, Params& params ) :
		VanadisCPUOSHandler(id, params) {

		os_link = configureLink( "os_link", "0ns", new Event::Handler<VanadisMIPSOSHandler>(this,
			&VanadisMIPSOSHandler::recvOSEvent ) );

		brk_zero_memory = params.find<bool>("brk_zero_memory", false);
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

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to brk( value: %" PRIu64 " ), zero: %s\n",
					newBrk, brk_zero_memory ? "yes" : "no");
				call_ev = new VanadisSyscallBRKEvent( core_id, hw_thr, newBrk, brk_zero_memory );
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
		case VANADIS_SYSCALL_FSTAT:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
				int32_t file_handle = regFile->getIntReg<int32_t>( phys_reg_4 );

				const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
				uint64_t fstat_addr = regFile->getIntReg<uint64_t>( phys_reg_5 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to fstat( %" PRId32 ", %" PRIu64 " )\n",
					file_handle, fstat_addr);

				call_ev = new VanadisSyscallFstatEvent( core_id, hw_thr, file_handle, fstat_addr );
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

		case VANADIS_SYSCALL_EXIT_GROUP:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t exit_code = regFile->getIntReg<int64_t>( phys_reg_4 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to exit_group( %" PRId64 " )\n", exit_code );
				call_ev = new VanadisSyscallExitGroupEvent( core_id, hw_thr, exit_code );
			}
			break;

		case VANADIS_SYSCALL_WRITE:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t write_fd = regFile->getIntReg<int64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t write_buff = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                uint64_t write_count = regFile->getIntReg<int64_t>( phys_reg_6);

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to write( %" PRId64 ", 0x%llx, %" PRIu64 " )\n",
					write_fd, write_buff, write_count);
				call_ev = new VanadisSyscallWriteEvent( core_id, hw_thr, write_fd, write_buff, write_count );
			}
			break;

		case VANADIS_SYSCALL_SET_TID:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t new_tid = regFile->getIntReg<int64_t>( phys_reg_4 );

				setThreadID( new_tid );
				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found call to set_tid( %" PRId64 " )\n",
					new_tid);

				recvOSEvent( new VanadisSyscallResponse( new_tid ) );
			}
			break;

		case VANADIS_SYSCALL_MADVISE:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t advise_addr = regFile->getIntReg<int64_t>( phys_reg_4 );

				const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t advise_len = regFile->getIntReg<int64_t>( phys_reg_5 );

				const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                uint64_t advise_advice = regFile->getIntReg<int64_t>( phys_reg_6 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found call to madvise( 0x%llx, %" PRIu64 ", %" PRIu64 " )\n",
					advise_addr, advise_len, advise_advice);

				// output->fatal(CALL_INFO, -1, "STOP\n");
				recvOSEvent( new VanadisSyscallResponse( 0 ) );
			}
			break;

		case VANADIS_SYSCALL_FUTEX:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t futex_addr = regFile->getIntReg<uint64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                int32_t futex_op = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                uint32_t futex_val = regFile->getIntReg<int32_t>( phys_reg_6);

                                const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
                                uint64_t futex_timeout_addr = regFile->getIntReg<int32_t>( phys_reg_7);

                                const uint16_t phys_reg_sp = isaTable->getIntPhysReg(29);
                                uint64_t stack_ptr = regFile->getIntReg<uint64_t>( phys_reg_sp );

                                output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to futex( 0x%llx, %" PRId32 ", %" PRIu32 ", %" PRIu64 ", sp: 0x%llx (arg-count is greater than 4))\n",
					futex_addr, futex_op, futex_val, futex_timeout_addr, stack_ptr );

				recvOSEvent( new VanadisSyscallResponse( 0 ) );
			}
			break;

		case VANADIS_SYSCALL_IOCTL:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t fd = regFile->getIntReg<int64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t io_req = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                uint64_t ptr = regFile->getIntReg<uint64_t>( phys_reg_6 );

				uint64_t access_type = (io_req & 0xE0000000) >> 29;

				bool is_read       = (access_type & 0x1) != 0;
				bool is_write      = (access_type & 0x2) != 0;

				uint64_t data_size = ((io_req) & 0x1FFF0000) >> 16;
				uint64_t io_op     = ((io_req) & 0xFF);

				uint64_t io_driver = ((io_req) &  0xFF00) >> 8;

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to ioctl( %" PRId64 ", %" PRIu64 " / 0x%llx, %" PRIu64 " / 0x%llx )\n",
					fd, io_req, io_req, ptr, ptr);
				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] -> R: %c W: %c / size: %" PRIu64 " / op: %" PRIu64 " / drv: %" PRIu64 "\n",
					is_read ? 'y' : 'n', is_write ? 'y' : 'n', data_size, io_op, io_driver );

				call_ev = new VanadisSyscallIOCtlEvent( core_id, hw_thr, fd, is_read, is_write, io_op, io_driver, ptr, data_size );
			}
			break;

		case VANADIS_SYSCALL_MMAP:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t map_addr = regFile->getIntReg<uint64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t map_len = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int32_t map_prot = regFile->getIntReg<int32_t>( phys_reg_6);

                                const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
                                int32_t map_flags = regFile->getIntReg<int32_t>( phys_reg_7);

				const uint16_t phys_reg_sp = isaTable->getIntPhysReg(29);
				uint64_t stack_ptr = regFile->getIntReg<uint64_t>( phys_reg_sp );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to mmap( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32 ", sp: 0x%llx (> 4 arguments) )\n",
					map_addr, map_len, map_prot, map_flags, stack_ptr);

				if( (0 == map_addr) && (0==map_len) ) {
					recvOSEvent( new VanadisSyscallResponse(-22) );
				} else {
					output->fatal(CALL_INFO, -1, "STOP\n");
				}
			}
			break;

		case VANADIS_SYSCALL_MMAP2:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                uint64_t map_addr = regFile->getIntReg<uint64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t map_len = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                int32_t map_prot = regFile->getIntReg<int32_t>( phys_reg_6);

                                const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
                                int32_t map_flags = regFile->getIntReg<int32_t>( phys_reg_7);

				const uint16_t phys_reg_sp = isaTable->getIntPhysReg(29);
				uint64_t stack_ptr = regFile->getIntReg<uint64_t>( phys_reg_sp );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to mmap2( 0x%llx, %" PRIu64 ", %" PRId32 ", %" PRId32 ", sp: 0x%llx (> 4 arguments) )\n",
					map_addr, map_len, map_prot, map_flags, stack_ptr);

				call_ev = new VanadisSyscallMemoryMapEvent( core_id, hw_thr, map_addr, map_len, map_prot, map_flags, stack_ptr, 4096 );
			}
			break;

		case VANADIS_SYSCALL_GETTIME64:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int64_t clk_type = regFile->getIntReg<int64_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t time_addr = regFile->getIntReg<uint64_t>( phys_reg_5 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to clock_gettime64( %" PRId64 ", 0x%llx )\n",
					clk_type, time_addr );

				call_ev = new VanadisSyscallGetTime64Event( core_id, hw_thr, clk_type, time_addr );
			}
			break;

		case VANADIS_SYSCALL_RT_SETSIGMASK:
			{
				const uint16_t phys_reg_4 = isaTable->getIntPhysReg(4);
                                int32_t how = regFile->getIntReg<int32_t>( phys_reg_4 );

                                const uint16_t phys_reg_5 = isaTable->getIntPhysReg(5);
                                uint64_t signal_set_in = regFile->getIntReg<uint64_t>( phys_reg_5 );

                                const uint16_t phys_reg_6 = isaTable->getIntPhysReg(6);
                                uint64_t signal_set_out = regFile->getIntReg<uint64_t>( phys_reg_6);

				const uint16_t phys_reg_7 = isaTable->getIntPhysReg(7);
				int32_t signal_set_size = regFile->getIntReg<int32_t>( phys_reg_7 );

				output->verbose(CALL_INFO, 8, 0, "[syscall-handler] found a call to rt_sigprocmask( %" PRId32 ", 0x%llx, 0x%llx, %" PRId32 ")\n",
					how, signal_set_in, signal_set_out, signal_set_size);

				recvOSEvent( new VanadisSyscallResponse( 0 ) );
			}
			break;

		default:
			{
				const uint16_t phys_reg_31 = isaTable->getIntPhysReg(31);
				uint64_t link_reg = regFile->getIntReg<int32_t>( phys_reg_31 );

				output->fatal(CALL_INFO, -1, "[syscall-handler] Error: unknown code %" PRIu64 " (ins: 0x%llx, link-reg: 0x%llx)\n",
					os_code, syscallIns->getInstructionAddress(), link_reg);
			}
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
		output->verbose(CALL_INFO, 8, 0, "-> recv os response\n");

		VanadisSyscallResponse* os_resp = dynamic_cast<VanadisSyscallResponse*>( ev );

		if( nullptr != os_resp ) {
			output->verbose(CALL_INFO, 8, 0, "syscall return-code: %" PRId64 " (success: %3s)\n", os_resp->getReturnCode(),
				os_resp->isSuccessful() ? "yes" : "no" );
			output->verbose(CALL_INFO, 8, 0, "-> issuing call-backs to clear syscall ROB stops...\n");

			// Set up the return code (according to ABI, this goes in r2)
			const uint16_t rc_reg   = isaTable->getIntPhysReg(2);
			const int64_t  rc_val = (int64_t) os_resp->getReturnCode();
			regFile->setIntReg( rc_reg, rc_val );

			if( os_resp->isSuccessful() ) {
				if( rc_val < 0 ) {
					writeSyscallResult( false );
				} else {
					// Generate correct markers for OS return code checks
					writeSyscallResult( os_resp->isSuccessful() );
				}
			} else {
				writeSyscallResult( false );
			}

			for( int i = 0; i < returnCallbacks.size(); ++i ) {
				returnCallbacks[i](hw_thr);
			}
		} else {
			VanadisExitResponse* os_exit = dynamic_cast<VanadisExitResponse*>( ev );

			output->verbose(CALL_INFO, 8, 0, "received an exit command from the operating system (return-code: %" PRId64 " )\n",
				os_exit->getReturnCode() );

			haltThrCallBack( hw_thr, os_exit->getReturnCode() );
		}

		delete ev;
	}

	SST::Link* os_link;
	bool brk_zero_memory;

};

}
}

#endif
