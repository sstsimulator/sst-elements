// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_CORE_HANDLER
#define _H_VANADIS_NODE_OS_CORE_HANDLER

#include <cstdint>
#include <cinttypes>

#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/output.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "os/memmgr/vmemmgr.h"

#include "os/callev/voscallall.h"
#include "os/resp/voscallresp.h"
#include "os/resp/vosexitresp.h"
#include "os/voscallev.h"

#include "os/node/vnodeoshstate.h"
#include "os/node/vnodeosfd.h"
#include "os/node/vnodeosfstath.h"
#include "os/node/vnodeosunameh.h"
#include "os/node/vnodeoswritevh.h"
#include "os/node/vnodeoswriteh.h"
#include "os/node/vnodeosopenath.h"
#include "os/node/vnodeosopenh.h"
#include "os/node/vnodeosreadlink.h"
#include "os/node/vnodeosaccessh.h"
#include "os/node/vnodeosbrk.h"
#include "os/node/vnodemmaph.h"
#include "os/node/vnodenoactionh.h"

#include "os/node/vnodeosstattype.h"
#include "util/vlinesplit.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisNodeOSCoreHandler {
public:
	VanadisNodeOSCoreHandler( uint32_t verbosity, uint32_t core, const char* stdin_path,
		const char* stdout_path, const char* stderr_path ) :
		core_id(core), next_file_id(16) {

		core_link = nullptr;

		char* out_prefix = new char[64];
		snprintf( out_prefix, 64, "[node-os-core-%6" PRIu32 "]: ", core );
		output = new SST::Output( out_prefix, verbosity, 0, SST::Output::STDOUT );

		handler_state = nullptr;

		if( stdin_path != nullptr )
			file_descriptors.insert( std::pair< uint32_t, VanadisOSFileDescriptor*>( 0, new VanadisOSFileDescriptor( 0, stdin_path  ) ) );
		if( stdout_path != nullptr )
			file_descriptors.insert( std::pair< uint32_t, VanadisOSFileDescriptor*>( 1, new VanadisOSFileDescriptor( 1, stdout_path ) ) );
		if( stderr_path != nullptr )
			file_descriptors.insert( std::pair< uint32_t, VanadisOSFileDescriptor*>( 2, new VanadisOSFileDescriptor( 2, stderr_path ) ) );

		current_brk_point = 0;

		handlerSendMemCallback = std::bind( &VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );
	}

	~VanadisNodeOSCoreHandler() {
		delete output;

		for( auto next_file = file_descriptors.begin(); next_file != file_descriptors.end(); next_file++ ) {
			delete next_file->second;
		}
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

				output->verbose(CALL_INFO, 16, 0, "[syscall-uname] ---> uname struct is at address 0x%0llx\n", uname_ev->getUnameInfoAddress());
				output->verbose(CALL_INFO, 16, 0, "[syscall-uname] ---> generating syscall memory updates...\n");

				std::vector<uint8_t> uname_payload;
				uname_payload.resize( 65 * 5, (uint8_t) '\0' );

				const char* uname_sysname = "Linux";
				const char* uname_node    = "sim.sstsimulator.org";
				const char* uname_release = "4.19.136";
				const char* uname_version = "#1 Wed Sep 2 23:59:59 MST 2020";
				const char* uname_machine = "mips";

				for( size_t i = 0; i < std::strlen( uname_sysname ); ++i ) {
					uname_payload[i] = uname_sysname[i];
				}

				for( size_t i = 0; i < std::strlen( uname_node ); ++i ) {
					uname_payload[65 + i] = uname_node[i];
				}

				for( size_t i = 0; i < std::strlen( uname_release ); ++i ) {
					uname_payload[65 + 65 +i] = uname_release[i];
				}

				for( size_t i = 0; i < std::strlen( uname_version ); ++i ) {
					uname_payload[65 + 65 + 65 +i] = uname_version[i];
				}

				for( size_t i = 0; i < std::strlen( uname_machine ); ++i ) {
					uname_payload[65 + 65 + 65 + 65 + i] = uname_machine[i];
				}

				sendBlockToMemory( uname_ev->getUnameInfoAddress(), uname_payload );

				handler_state = new VanadisUnameHandlerState( output->getVerboseLevel() );
			}
			break;

		case SYSCALL_OP_FSTAT:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is fstat()\n");
				VanadisSyscallFstatEvent* fstat_ev = dynamic_cast<VanadisSyscallFstatEvent*>( sys_ev );

				if( nullptr == fstat_ev ) {
					output->fatal(CALL_INFO, -1, "-> error: unable to case syscall to a fstat event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] ---> file-handle: %" PRId32 "\n",
					fstat_ev->getFileHandle() );
				output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] ---> stat-struct-addr: %" PRIu64 " / 0x%llx\n",
					fstat_ev->getStructAddress(), fstat_ev->getStructAddress() );

				bool success = false;
				struct vanadis_stat32 v32_stat_output;
				struct stat stat_output;
				int fstat_return_code = 0;

				if( fstat_ev->getFileHandle() <= 2 ) {
					if( 0 == fstat( fstat_ev->getFileHandle(), &stat_output ) ) {
						vanadis_copy_native_stat( &stat_output, &v32_stat_output );
						success = true;
					} else {
						fstat_return_code = -13;
					}
				} else {
					auto vos_descriptor_itr = file_descriptors.find( fstat_ev->getFileHandle() );

					if( vos_descriptor_itr == file_descriptors.end() ) {
						// Fail - cannot find the descriptor, so its not open
						fstat_return_code = -9;
					} else {
						VanadisOSFileDescriptor* vos_descriptor = vos_descriptor_itr->second;

						if( 0 == fstat( fileno( vos_descriptor->getFileHandle() ), &stat_output ) ) {
							vanadis_copy_native_stat( &stat_output, &v32_stat_output );
	                                               	success = true;
						} else {
							fstat_return_code = -13;
						}
					}
				}

				if( success ) {
					std::vector<uint8_t> stat_write_payload;
					stat_write_payload.reserve( sizeof( v32_stat_output ) );

					uint8_t* stat_output_ptr = (uint8_t*)( &v32_stat_output );
					for( int i = 0; i < sizeof(stat_output); ++i ) {
						stat_write_payload.push_back( stat_output_ptr[i] );
					}

					sendBlockToMemory( fstat_ev->getStructAddress(), stat_write_payload );

					handler_state = new VanadisFstatHandlerState( output->getVerboseLevel(),
						fstat_ev->getFileHandle(), fstat_ev->getStructAddress() );
				} else {
					output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] - response is operation failed code: %" PRId32 "\n", fstat_return_code);

					VanadisSyscallResponse* resp = new VanadisSyscallResponse( fstat_return_code );
					resp->markFailed();

                                       	core_link->send( resp );
				}
			}
			break;

		case SYSCALL_OP_OPENAT:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is openat()\n");
				VanadisSyscallOpenAtEvent* openat_ev = dynamic_cast< VanadisSyscallOpenAtEvent* >(sys_ev);

				if( nullptr == openat_ev ) {
					output->fatal(CALL_INFO, -1, "[syscall-openat] -> error unable ot cast syscall to an openat event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-openat] -> call is openat( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
					openat_ev->getDirectoryFileDescriptor(),
					openat_ev->getPathPointer(), openat_ev->getFlags() );

				handler_state = new VanadisOpenAtHandlerState( output->getVerboseLevel(), openat_ev->getDirectoryFileDescriptor(),
					openat_ev->getPathPointer(), openat_ev->getFlags(), &file_descriptors, handlerSendMemCallback );

				const uint64_t start_read_len = (openat_ev->getPathPointer() % 64) == 0 ? 64 : openat_ev->getPathPointer() % 64;

				SimpleMem::Request* openat_start_req = new SimpleMem::Request( SimpleMem::Request::Read,
					openat_ev->getPathPointer(), start_read_len );

				sendMemRequest( openat_start_req );
			}
			break;

		case SYSCALL_OP_CLOSE:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is close()\n");
				VanadisSyscallCloseEvent* close_ev = dynamic_cast< VanadisSyscallCloseEvent* >(sys_ev);

				if( nullptr == close_ev ) {
					output->fatal(CALL_INFO, -1, "[syscall-close] -> error unable to cast syscall to a close event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-close] -> call is close( %" PRIu32 " )\n",
					close_ev->getFileDescriptor());

				auto close_file_itr = file_descriptors.find( close_ev->getFileDescriptor() );
				VanadisSyscallResponse* resp = nullptr;

				if( close_file_itr == file_descriptors.end() ) {
					// return failure of a EBADF (bad file descriptor)
					resp = new VanadisSyscallResponse( -9 );
					resp->markFailed();
				} else {
					// this will automatically close any open file handles
					delete close_file_itr->second;

					// remove from the descriptor listing
					file_descriptors.erase(close_file_itr);

					resp = new VanadisSyscallResponse( 0 );
				}

                              	core_link->send( resp );
			}
			break;

		case SYSCALL_OP_OPEN:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is open()\n");
				VanadisSyscallOpenEvent* open_ev = dynamic_cast< VanadisSyscallOpenEvent* >(sys_ev);

				if( nullptr == open_ev ) {
					output->fatal(CALL_INFO, -1, "[syscall-open] -> error unable ot cast syscall to an open event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-open] -> call is open( 0x%0llx, %" PRId64 " )\n",
					open_ev->getPathPointer(), open_ev->getFlags() );

				handler_state = new VanadisOpenHandlerState( output->getVerboseLevel(), open_ev->getPathPointer(),
					open_ev->getFlags(), open_ev->getMode(), &file_descriptors, handlerSendMemCallback );

				const uint64_t start_read_len = (open_ev->getPathPointer() % 64) == 0 ? 64 : open_ev->getPathPointer() % 64;

				SimpleMem::Request* open_start_req = new SimpleMem::Request( SimpleMem::Request::Read,
					open_ev->getPathPointer(), start_read_len );

				sendMemRequest( open_start_req );
			}
			break;

		case SYSCALL_OP_READ:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is read()\n");
				VanadisSyscallReadEvent* read_ev = dynamic_cast< VanadisSyscallReadEvent* >(sys_ev);

				if( nullptr == read_ev ) {
					output->fatal(CALL_INFO, -1, "-> error unable to cast syscall to a read event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "-> call is read( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
					read_ev->getFileDescriptor(), read_ev->getBufferAddress(),
					read_ev->getCount() );

				for( auto next_file = file_descriptors.begin() ; next_file != file_descriptors.end();
					next_file++ ) {

					output->verbose(CALL_INFO, 16, 0, "---> file: %" PRIu32 " (path: %s)\n",
						next_file->first, next_file->second->getPath());
				}

				output->fatal(CALL_INFO, -1, "NOT IMPLEMENTED\n");

				if( 0 == read_ev->getCount() ) {
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( 0 );
		                        core_link->send( resp );
				} else {
					output->fatal(CALL_INFO, -1, "Non-zeros not implemented.\n");
				}
			}
			break;

		case SYSCALL_OP_READLINK:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is readlink()\n");
				VanadisSyscallReadLinkEvent* readlink_ev = dynamic_cast< VanadisSyscallReadLinkEvent* >( sys_ev );

				if( nullptr == readlink_ev ) {
					output->fatal(CALL_INFO, -1, "[syscall-readlink] -> error unable ot cast system call to a readlink event.\n");
				}

				if( readlink_ev->getBufferSize() > 0 ) {
					output->verbose(CALL_INFO, 16, 0, "[syscall-readlink] -> readlink( 0x%0llx, 0x%llx, %" PRId64 " )\n",
						readlink_ev->getPathPointer(), readlink_ev->getBufferPointer(), readlink_ev->getBufferSize() );

					std::function<void(SimpleMem::Request*)> send_req_func = std::bind(
       	                                                        	&VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );
					std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block_func = std::bind(
						&VanadisNodeOSCoreHandler::sendBlockToMemory, this, std::placeholders::_1, std::placeholders::_2 );

					handler_state = new VanadisReadLinkHandlerState( output->getVerboseLevel(), readlink_ev->getPathPointer(),
						readlink_ev->getBufferPointer(), readlink_ev->getBufferSize(), send_req_func, send_block_func );

					uint64_t line_remain = vanadis_line_remainder( readlink_ev->getPathPointer(), 64 );

					sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Read,
							readlink_ev->getPathPointer(), line_remain ) );

				} else if( readlink_ev->getBufferSize() == 0 ) {
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( 0 );
		                        core_link->send( resp );
				} else {
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( -22 );
		                        core_link->send( resp );
				}

			}
			break;

		case SYSCALL_OP_WRITEV:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is writev()\n");
				VanadisSyscallWritevEvent* writev_ev = dynamic_cast< VanadisSyscallWritevEvent* >(sys_ev);

				if( nullptr == writev_ev ) {
					output->fatal(CALL_INFO, -1, "-> error unable to cast syscall to a writev event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-writev] -> call is writev( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
					writev_ev->getFileDescriptor(), writev_ev->getIOVecAddress(),
					writev_ev->getIOVecCount() );

				auto file_des = file_descriptors.find( writev_ev->getFileDescriptor() );

				if( file_des == file_descriptors.end() ) {

					output->verbose(CALL_INFO, 16, 0, "[syscall-writev] -> file handle %" PRId64 " is not currently open, return an error code.\n",
						writev_ev->getFileDescriptor());

					// EINVAL = 22
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( -22 );
                                                core_link->send( resp );
				} else {

					if( writev_ev->getIOVecCount() > 0 ) {
						std::function<void(SimpleMem::Request*)> send_req_func = std::bind(
								&VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );

						handler_state = new VanadisWritevHandlerState( output->getVerboseLevel(),
							writev_ev->getFileDescriptor(), writev_ev->getIOVecAddress(),
       		                                	writev_ev->getIOVecCount(), file_des->second->getFileHandle(), send_req_func );

						// Launch read of the initial iovec info
						sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Read,
							writev_ev->getIOVecAddress(), 4) );
					} else if( writev_ev->getIOVecCount() == 0 ) {
						VanadisSyscallResponse* resp = new VanadisSyscallResponse( 0 );
			                        core_link->send( resp );
					} else {
						// EINVAL = 22
						VanadisSyscallResponse* resp = new VanadisSyscallResponse( -22 );
			                        core_link->send( resp );
					}

				}
			}
			break;

		case SYSCALL_OP_WRITE:
			{
				output->verbose(CALL_INFO, 16, 0, "-> call is write()\n");
				VanadisSyscallWriteEvent* write_ev = dynamic_cast< VanadisSyscallWriteEvent* >(sys_ev);

				if( nullptr == write_ev ) {
					output->fatal(CALL_INFO, -1, "-> error unable to cast syscall to a write event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-write] -> call is write( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
					write_ev->getFileDescriptor(), write_ev->getBufferAddress(),
					write_ev->getBufferCount() );

				auto file_des = file_descriptors.find( write_ev->getFileDescriptor() );

				if( file_des == file_descriptors.end() ) {
					output->verbose(CALL_INFO, 16, 0, "[syscall-write] -> file handle %" PRId64 " is not currently open, return an error code.\n",
						write_ev->getFileDescriptor());

					// EINVAL = 22
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( -22 );
                                                core_link->send( resp );
				} else {
					if( write_ev->getBufferCount() > 0 ) {
						const uint64_t line_offset = (write_ev->getBufferAddress() % 64);
						const uint64_t start_read_len = ((line_offset + write_ev->getBufferCount()) < 64) ?
							write_ev->getBufferCount() : 64 - line_offset;

						std::function<void(SimpleMem::Request*)> send_req_func = std::bind(
								&VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );

						handler_state = new VanadisWriteHandlerState( output->getVerboseLevel(),
							file_des->second->getFileHandle(),
							write_ev->getFileDescriptor(), write_ev->getBufferAddress(),
							write_ev->getBufferCount(), send_req_func );

						sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Read,
							write_ev->getBufferAddress(), start_read_len ));
					} else {
						VanadisSyscallResponse* resp = new VanadisSyscallResponse( 0 );
                                                core_link->send( resp );
					}
				}
			}
			break;

		case SYSCALL_OP_BRK:
			{
				VanadisSyscallBRKEvent* brk_ev = dynamic_cast< VanadisSyscallBRKEvent* >( sys_ev );
				output->verbose(CALL_INFO, 16, 0, "[syscall-brk] recv brk( 0x%0llx ) call.\n", brk_ev->getUpdatedBRK() );

				if( brk_ev->getUpdatedBRK() < current_brk_point ) {
					output->verbose(CALL_INFO, 16, 0, "[syscall-brl]: brk provided (0x%llx) is less than existing brk point (0x%llx), so returning current brk point\n",
						brk_ev->getUpdatedBRK(), current_brk_point );

					// Linux syscall for brk returns the BRK point on success
					VanadisSyscallResponse* resp = new VanadisSyscallResponse( current_brk_point );
					core_link->send( resp );
				} else {
					uint64_t old_brk  = current_brk_point;
					current_brk_point = brk_ev->getUpdatedBRK();

					if( brk_ev->requestZeroMemory() ) {
						output->verbose(CALL_INFO, 16, 0, "[syscall-brk] - zeroing memory requested, producing a zero payload for write\n");

						std::vector<uint8_t> payload;
						payload.resize( brk_ev->getUpdatedBRK() - old_brk, 0 );

						sendBlockToMemory( old_brk, payload );

						handler_state = new VanadisBRKHandlerState( output->getVerboseLevel(), current_brk_point );
					} else {
						output->verbose(CALL_INFO, 16, 0, "[syscall-brk] - zeroing is not requested, immediate return.\n");
						VanadisSyscallResponse* resp = new VanadisSyscallResponse( current_brk_point );
						core_link->send( resp );
					}

					output->verbose(CALL_INFO, 16, 0, "[syscall-brk] old brk: 0x%llx -> new brk: 0x%llx (diff: %" PRIu64 ")\n",
						old_brk, current_brk_point, (current_brk_point - old_brk) );
				}
			}
			break;

		case SYSCALL_OP_IOCTL:
			{
				VanadisSyscallIOCtlEvent* ioctl_ev = dynamic_cast< VanadisSyscallIOCtlEvent* >( sys_ev );
				if( nullptr == ioctl_ev ) {
					output->fatal(CALL_INFO, -1, "Unable to convert to an ioctl event.\n");
				}

				output->verbose(CALL_INFO, 16, 0, "[syscall-ioctl] ioctl( %" PRId64 ", r: %c / w: %c / ptr: 0x%llx / size: %" PRIu64 " / op: %" PRIu64 " / drv: %" PRIu64 " )\n",
					ioctl_ev->getFileDescriptor(), ioctl_ev->isRead() ? 'y' : 'n',
					ioctl_ev->isWrite() ? 'y' : 'n', ioctl_ev->getDataPointer(),
					ioctl_ev->getDataLength(), ioctl_ev->getIOOperation(),
					ioctl_ev->getIODriver() );

				//if( 1 == ioctl_ev->getFileDescriptor() ) {
				//	VanadisSyscallResponse* resp = new VanadisSyscallResponse( 0 );
        	                //        core_link->send( resp );
				//} else {
				//	output->fatal(CALL_INFO, -1, "Not implemented\n");
				//}

				VanadisSyscallResponse* resp = new VanadisSyscallResponse( -25 );
				resp->markFailed();

				core_link->send( resp );
			}
			break;

		case SYSCALL_OP_ACCESS:
			{
				VanadisSyscallAccessEvent* access_ev = dynamic_cast< VanadisSyscallAccessEvent* >( sys_ev );
				output->verbose(CALL_INFO, 16, 0, "[syscall-access] access( 0x%llx, %" PRIu64 " )\n",
					access_ev->getPathPointer(), access_ev->getAccessMode() );

				std::function<void(SimpleMem::Request*)> send_req_func = std::bind(
       	                        	&VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );

				handler_state = new VanadisAccessHandlerState( output->getVerboseLevel(), access_ev->getPathPointer(), access_ev->getAccessMode(),
					send_req_func );

				uint64_t line_remain = vanadis_line_remainder( access_ev->getPathPointer(), 64 );
				sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Read,
					access_ev->getPathPointer(), line_remain ) );
			}
			break;

		case SYSCALL_OP_GETTIME64:
			{
				VanadisSyscallGetTime64Event* gettime_ev = dynamic_cast< VanadisSyscallGetTime64Event* >( sys_ev );
				output->verbose(CALL_INFO, 16, 0, "[syscall-gettime64] gettime64( %" PRId64 ", 0x%llx )\n",
					gettime_ev->getClockType(), gettime_ev->getTimeStructAddress() );

				// Need to do a handler state to force memory writes to complete before we return
				handler_state = new VanadisNoActionHandlerState( output->getVerboseLevel(), 0 );

				uint64_t sim_time_ns = getSimTimeNano();
				uint64_t sim_seconds = (uint64_t)( sim_time_ns / 1000000000ULL );
				uint32_t sim_ns      = (uint32_t)( sim_time_ns % 1000000000ULL );

				output->verbose(CALL_INFO, 16, 0, "[syscall-gettime64] --> sim-time: %" PRIu64 " ns -> %" PRIu64 " secs + %" PRIu32 " us\n",
					sim_time_ns, sim_seconds, sim_ns);

				std::vector<uint8_t> seconds_payload;
				seconds_payload.resize( sizeof(sim_seconds), 0 );

				uint8_t* sec_ptr    = (uint8_t*) &sim_seconds;
				for( size_t i = 0; i < sizeof(sim_seconds); ++i ) {
					seconds_payload[i] = sec_ptr[i];
				}

				sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
					gettime_ev->getTimeStructAddress(), sizeof(sim_seconds),
					seconds_payload ) );

				std::vector<uint8_t> ns_payload;
				ns_payload.resize( sizeof( sim_ns ), 0 );

				uint8_t* ns_ptr     = (uint8_t*) &sim_ns;
				for( size_t i = 0; i < sizeof(sim_ns); ++i ) {
					ns_payload[i] = ns_ptr[i];
				}

				sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
					gettime_ev->getTimeStructAddress() + sizeof(sim_seconds), sizeof(sim_ns),
					ns_payload ) );
			}
			break;

		case SYSCALL_OP_UNMAP:
			{
				VanadisSyscallMemoryUnMapEvent* unmap_ev = dynamic_cast< VanadisSyscallMemoryUnMapEvent*>( sys_ev );
				assert( unmap_ev != NULL );

				output->verbose(CALL_INFO, 16, 0, "[syscall-unmap] --> called.\n");

				uint64_t unmap_address = unmap_ev->getDeallocationAddress();
				uint64_t unmap_len     = unmap_ev->getDeallocationLength();

				int unmap_result = memory_mgr->deallocateRange( unmap_address, unmap_len );

				switch( unmap_result ) {
				case 0:
					{
						output->verbose(CALL_INFO, 16, 0, "[syscall-unmap] --> success, returning response.\n");
						VanadisSyscallResponse* resp = new VanadisSyscallResponse();
		                                core_link->send( resp );
					}
					break;
				default:
					{
						output->verbose(CALL_INFO, 16, 0, "[syscall-unmap] --> call failed, returning -22 as EINVAL\n");
						VanadisSyscallResponse* resp = new VanadisSyscallResponse( -22 );
						resp->markFailed();
		                                core_link->send( resp );
					}
					break;
				}
			}
			break;

		case SYSCALL_OP_MMAP:
			{
				VanadisSyscallMemoryMapEvent* mmap_ev = dynamic_cast< VanadisSyscallMemoryMapEvent* >( sys_ev );
				assert( mmap_ev != NULL );

				output->verbose(CALL_INFO, 16, 0, "[syscall-mmap] --> \n");

				uint64_t map_address = mmap_ev->getAllocationAddress();
				uint64_t map_length  = mmap_ev->getAllocationLength();
				int64_t  map_protect = mmap_ev->getProtectionFlags();
				int64_t  map_flags   = mmap_ev->getAllocationFlags();
				uint64_t call_stack  = mmap_ev->getStackPointer();
				uint64_t map_offset_units = mmap_ev->getOffsetUnits();

				std::function<void(SimpleMem::Request*)> send_req_func = std::bind(
                                                                &VanadisNodeOSCoreHandler::sendMemRequest, this, std::placeholders::_1 );
				std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block_func = std::bind(
                                                &VanadisNodeOSCoreHandler::sendBlockToMemory, this, std::placeholders::_1, std::placeholders::_2 );

				handler_state = new VanadisMemoryMapHandlerState( output->getVerboseLevel(),
					map_address, map_length, map_protect, map_flags, call_stack, map_offset_units,
					send_req_func, send_block_func, memory_mgr);

				// need to read in the file descriptor so get a memory read in place
				// file descriptor is the 5th argument (each is 4 bytes, so 5 * 4)
				sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Read,
                                        call_stack + 4 * 4, 4 ) );
			}
			break;

		case SYSCALL_OP_SET_THREAD_AREA:
			{
				VanadisSyscallResponse* resp = new VanadisSyscallResponse();
				core_link->send( resp );
			}
			break;

		case SYSCALL_OP_EXIT_GROUP:
			{
				VanadisExitResponse* exit_resp = new VanadisExitResponse();
				core_link->send( exit_resp );
			}
			break;

		default:
			{
				output->fatal(CALL_INFO, -1, "Error - unknown operating system call (code: %" PRIu64 ")\n",
					(uint64_t) sys_ev->getOperation());
			}
			break;
		}

	}

	void sendBlockToMemory( const uint64_t start_address, std::vector<uint8_t>& data_block ) {
		output->verbose(CALL_INFO, 16, 0, "bulk memory write addr: 0x%0llx / len: %" PRIu64 "\n",
				start_address, (uint64_t) data_block.size());

		uint64_t prolog_size = 0;

		if( data_block.size() < 64 ) {
			// Less than one cache line, but may still span 2 lines
			uint64_t start_address_offset = (start_address % 64);

			if( data_block.size() < (64 - start_address_offset) ) {
				prolog_size = data_block.size();
			} else {
				prolog_size = 64 - start_address_offset;
			}
		} else {
			prolog_size = 64 - (start_address % 64);
		}

		output->verbose(CALL_INFO, 16, 0, "prolog is %" PRIu64 " bytes.\n", prolog_size );

		std::vector<uint8_t> offset_payload;
		for( uint64_t i = 0; i < prolog_size; ++i ) {
			offset_payload.push_back( data_block[i] );
		}

		sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
			start_address, offset_payload.size(), offset_payload) );

		uint64_t remainder = (data_block.size() - prolog_size) % 64;
		uint64_t blocks    = (data_block.size() - prolog_size - remainder) / 64;

		output->verbose(CALL_INFO, 16, 0, "requires %" PRIu64 " lines to be written.\n", blocks);
		output->verbose(CALL_INFO, 16, 0, "requires %" PRIu64 " bytes in remainder\n", remainder );

		for( uint64_t i = 0; i < blocks; ++i ) {
			std::vector<uint8_t> block_payload;

			for( uint64_t j = 0; j < 64; ++j ) {
				block_payload.push_back( data_block[ prolog_size + (i*64) + j] );
			}

			sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
				start_address + prolog_size + (i*64), block_payload.size(), block_payload) );
		}

		if( remainder > 0 ) {
			std::vector<uint8_t> remainder_payload;
			for( uint64_t i = 0; i < remainder; ++i ) {
				remainder_payload.push_back( data_block[ prolog_size + (blocks * 64) + i ] );
			}

			sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
				start_address + prolog_size + (blocks * 64), remainder_payload.size(), remainder_payload) );
		}
	}

	void handleIncomingMemory( SimpleMem::Request* ev ) {
		output->verbose(CALL_INFO, 16, 0, "recv memory event (addr: 0x%llx, size: %" PRIu64 ")\n",
			ev->addr, (uint64_t) ev->size);

		auto find_event = pending_mem.find(ev->id);

		if( find_event != pending_mem.end() ) {
			pending_mem.erase( find_event );

			// if we have a state registered, pass off the event and payload
			if( nullptr != handler_state ) {
				if( handler_state->isComplete() ) {
					// Completed?
				} else {
					handler_state->handleIncomingRequest( ev );
				}
			} else {
				// will this ever happen?
			}

			// if we are completed and all the memory events are processed
			if( handler_state->isComplete() && (pending_mem.size() == 0) ) {
				VanadisSyscallResponse* resp = handler_state->generateResponse();
				output->verbose(CALL_INFO, 16, 0, "handler is completed and all memory events are processed, sending response (= %" PRId64 " / 0x%llx, %s) to core.\n",
					resp->getReturnCode(), resp->getReturnCode(),
					resp->isSuccessful() ? "success" : "failed" );

				core_link->send( resp );

				delete handler_state;
				handler_state = nullptr;
			} else {
				if( handler_state->isComplete() ) {
					output->verbose(CALL_INFO, 16, 0, "handler completed but waiting on memory operations (%" PRIu32 " pending)\n",
						(uint32_t) pending_mem.size() );
				}
			}
		}

		delete ev;
	}

	void setSendMemoryCallback( std::function<void( SimpleMem::Request*, uint32_t )> callback ) {
		output->verbose(CALL_INFO, 16, 0, "set new send memory callback\n");
		sendMemEventCallback = callback;
	}

	void sendMemRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "sending request: addr: %" PRIu64 " 0x%0llx / size: %" PRIu64 "\n",
			req->addr, req->addr, (uint64_t) req->size);

		pending_mem.insert( req->id );
		sendMemEventCallback( req, core_id );
	}

	void init( unsigned int phase ) {
		output->verbose( CALL_INFO, 8, 0, "performing init phase %u...\n", phase );

		while( SST::Event* ev = core_link->recvInitData() ) {
			VanadisSyscallEvent* sys_ev = dynamic_cast< VanadisSyscallEvent* >( ev );

			if( nullptr == sys_ev ) {
				output->fatal( CALL_INFO, -1, "Error - event cannot be converted to syscall\n" );
			}

			switch( sys_ev->getOperation() ) {
			case SYSCALL_OP_INIT_BRK:
				{
					VanadisSyscallInitBRKEvent* init_brk_ev = dynamic_cast< VanadisSyscallInitBRKEvent* >( sys_ev );
					output->verbose( CALL_INFO, 8, 0, "setting initial brk to 0x%llx\n", init_brk_ev->getUpdatedBRK() );

					current_brk_point = init_brk_ev->getUpdatedBRK();
				}
				break;

			default:
				output->fatal( CALL_INFO, -1, "Error - not implemented an an init event.\n" );
			}
		}
	}

	void setSimTimeNano( std::function<uint64_t()>& sim_time ) {
		getSimTimeNano = sim_time;
	}

	void setMemoryManager( VanadisMemoryManager* mem_m ) {
		memory_mgr = mem_m;
	}

protected:
	std::function<void( SimpleMem::Request* )> handlerSendMemCallback;
	std::function<void( SimpleMem::Request*, uint32_t )> sendMemEventCallback;
	std::function<uint64_t()> getSimTimeNano;

	std::unordered_set< SimpleMem::Request::id_t > pending_mem;
	std::unordered_map<uint32_t, VanadisOSFileDescriptor*> file_descriptors;

	VanadisMemoryManager* memory_mgr;
	VanadisHandlerState* handler_state;

	SST::Link* core_link;
	SST::Output* output;
	const uint32_t core_id;
	uint32_t next_file_id;

	uint64_t current_brk_point;
};

}
}

#endif
