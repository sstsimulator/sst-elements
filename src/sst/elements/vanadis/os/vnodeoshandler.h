
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

		resetSyscallNothing();
	}

	~VanadisNodeOSCoreHandler() {
		delete output;
	}

	void resetSyscallNothing() {
		current_syscall = std::bind( &VanadisNodeOSCoreHandler::processSyscallNothing, this );
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
				output->verbose(CALL_INFO, 16, 0, "---> generating syscall memory updates...\n");

				std::vector<uint8_t> uname_payload;
				uname_payload.resize( 65 * 5, (uint8_t) '\0' );

				const char* uname_sysname = "Linux";
				const char* uname_node    = "bigsystem.sstsimulator.org";
				const char* uname_release = "1.0.0";
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
				current_syscall = std::bind( &VanadisNodeOSCoreHandler::processSyscallUname, this );
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

	void sendBlockToMemory( const uint64_t start_address, std::vector<uint8_t>& data_block ) {
		uint64_t line_offset = 64 - (start_address % 64);

		std::vector<uint8_t> offset_payload;
		for( uint64_t i = 0; i < line_offset; ++i ) {
			offset_payload.push_back( data_block[i] );
		}

		sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
			start_address, offset_payload.size(), offset_payload) );

		uint64_t remainder = (data_block.size() - line_offset) % 64;
		uint64_t blocks    = (data_block.size() - line_offset - remainder) / 64;

		for( uint64_t i = 0; i < blocks; ++i ) {
			std::vector<uint8_t> block_payload;

			for( uint64_t j = 0; j < 64; ++j ) {
				block_payload.push_back( data_block[ line_offset + (i*64) + j] );
			}

			sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
				start_address + line_offset + (i*64), block_payload.size(), block_payload) );
		}

		std::vector<uint8_t> remainder_payload;
		for( uint64_t i = 0; i < remainder; ++i ) {
			remainder_payload.push_back( data_block[ line_offset + (blocks * 64) + i ] );
		}

		sendMemRequest( new SimpleMem::Request( SimpleMem::Request::Write,
			start_address + line_offset + (blocks * 64), remainder_payload.size(), remainder_payload) );
	}

	void handleIncomingMemory( SimpleMem::Request* ev ) {
		auto find_event = pending_mem.find(ev->id);

		if( find_event != pending_mem.end() ) {
			pending_mem.erase( find_event );
		}

		current_syscall();

		delete ev;
	}

	void setSendMemoryCallback( std::function<void( SimpleMem::Request*, uint32_t )> callback ) {
		output->verbose(CALL_INFO, 16, 0, "set new send memory callback\n");
		sendMemEventCallback = callback;
	}

	void sendMemRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "sending request: addr: %" PRIu64 " / size: %" PRIu64 "\n",
			req->addr, req->size);

		pending_mem.insert( req->id );
		sendMemEventCallback( req, core_id );
	}

	void processSyscallUname() {
		output->verbose(CALL_INFO, 16, 0, "processing callback for uname, pending memory events: %" PRIu32 "\n",
			pending_mem.size());

		if( 0 == pending_mem.size() ) {
			output->verbose(CALL_INFO, 16, 0, "completed, returning response to calling core.\n");
			VanadisSyscallResponse* resp = new VanadisSyscallResponse();
                	core_link->send( resp );

			resetSyscallNothing();
		}
	}

	void processSyscallNothing() {

	}

protected:
	std::function<void( SimpleMem::Request*, uint32_t )> sendMemEventCallback;
	std::function<void()> current_syscall;
	std::unordered_set< SimpleMem::Request::id_t > pending_mem;

	SST::Link* core_link;
	SST::Output* output;
	const uint32_t core_id;
};

}
}

#endif
