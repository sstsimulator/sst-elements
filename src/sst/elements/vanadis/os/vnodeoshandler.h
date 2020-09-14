
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

#include "os/node/vnodeoshstate.h"
#include "os/node/vnodeosfd.h"
#include "os/node/vnodeoswritevh.h"
#include "os/node/vnodeosopenath.h"
#include "os/node/vnodeosreadlink.h"
#include "os/node/vnodeosaccessh.h"

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
				output->fatal(CALL_INFO, -1, "uname handler not implemented.\n");
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
		case SYSCALL_OP_BRK:
			{
				VanadisSyscallBRKEvent* brk_ev = dynamic_cast< VanadisSyscallBRKEvent* >( sys_ev );
				output->verbose(CALL_INFO, 16, 0, "[syscall-brk] recv brk( 0x%0llx ) call.\n", brk_ev->getUpdatedBRK() );

				uint64_t old_brk  = current_brk_point;
				current_brk_point = brk_ev->getUpdatedBRK();

				output->verbose(CALL_INFO, 16, 0, "[syscall-brk] old brk: 0x%llx -> new brk: 0x%llx (diff: %" PRIu64 ")\n",
					old_brk, current_brk_point, (current_brk_point - old_brk) );

				// Linux syscall for brk returns the BRK point on success
				VanadisSyscallResponse* resp = new VanadisSyscallResponse( current_brk_point );
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

		case SYSCALL_OP_SET_THREAD_AREA:
			{
				VanadisSyscallResponse* resp = new VanadisSyscallResponse();
				core_link->send( resp );
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

		std::vector<uint8_t> remainder_payload;
		for( uint64_t i = 0; i < remainder; ++i ) {
			remainder_payload.push_back( data_block[ prolog_size + (blocks * 64) + i ] );
		}

		sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
			start_address + prolog_size + (blocks * 64), remainder_payload.size(), remainder_payload) );
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
				output->verbose(CALL_INFO, 16, 0, "handler is completed and all memory events are processed, sending response to core.\n");

				VanadisSyscallResponse* resp = handler_state->generateResponse();
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

protected:
	std::function<void( SimpleMem::Request* )> handlerSendMemCallback;
	std::function<void( SimpleMem::Request*, uint32_t )> sendMemEventCallback;

	std::unordered_set< SimpleMem::Request::id_t > pending_mem;
	std::unordered_map<uint32_t, VanadisOSFileDescriptor*> file_descriptors;

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