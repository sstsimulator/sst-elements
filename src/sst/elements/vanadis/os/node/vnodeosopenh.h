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

#ifndef _H_VANADIS_OS_OPEN_HANDLER
#define _H_VANADIS_OS_OPEN_HANDLER

#include <cstdint>
#include <cinttypes>

#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisOpenHandlerState : public VanadisHandlerState {
public:
        VanadisOpenHandlerState( uint32_t verbosity, uint64_t path_ptr, uint64_t flags, uint64_t mode,
		std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* fds,
		std::function<void(SimpleMem::Request*)> send_m ) :
                VanadisHandlerState( verbosity ),
                open_path_ptr(path_ptr), open_flags(flags), open_mode(mode),
		file_descriptors(fds) {

		send_mem_req = send_m;
		opened_fd_handle = 3;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "[syscall-open] request processing...\n");

		bool found_null = false;

		for( size_t i = 0; i < req->size; ++i ) {
			open_path.push_back( req->data[i] );

			if( req->data[i] == '\0' ) {
				found_null = true;
			}
		}

		if( found_null ) {
			const char* open_path_cstr = (const char*) &open_path[0];

			output->verbose(CALL_INFO, 16, 0, "[syscall-open] path at: \"%s\"\n", open_path_cstr );

			while( file_descriptors->find( opened_fd_handle ) !=
				file_descriptors->end() ) {
				opened_fd_handle++;
			}

			FILE* file_ptr = nullptr;

			switch( open_mode ) {
			case 0:
				{
					file_ptr = fopen( open_path_cstr, "r" );
				}
				break;
			case 1:
				{
					file_ptr = fopen( open_path_cstr, "w" );
				}
				break;
			case 2:
				{
					file_ptr = fopen( open_path_cstr, "rw" );
				}
				break;
			default:
				{
					// set return code to invalid flags (EINVAL)
					opened_fd_handle = -22;
				}
			}

			if( nullptr == file_ptr ) {
				output->verbose(CALL_INFO, 16, 0, "[syscall-open] file open at %s failed, did not find an appropriate access mode or file operation for that mode failed (handle: %" PRId32 ")\n",
					open_path_cstr, opened_fd_handle);
				if( opened_fd_handle > 0 ) {
					// set return code to invalid access (EACCES)
					opened_fd_handle = -13;
				}
			} else {
				output->verbose(CALL_INFO, 16, 0, "[syscall-open] new descriptor at %" PRIu32 "\n", opened_fd_handle );

				file_descriptors->insert( std::pair<uint32_t, VanadisOSFileDescriptor*>( opened_fd_handle,
					new VanadisOSFileDescriptor( opened_fd_handle, open_path_cstr, file_ptr ) ) );
			}

			markComplete();
		} else {
			send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
				open_path_ptr + open_path.size(), 64 ));
		}
        }

	virtual VanadisSyscallResponse* generateResponse() {
		VanadisSyscallResponse* resp = new VanadisSyscallResponse( opened_fd_handle );

		if( opened_fd_handle < 0 ) {
			resp->markFailed();
		}

		return resp;
	}

protected:
        const uint64_t open_path_ptr;
        const uint64_t open_flags;
	const uint64_t open_mode;

        std::vector<uint8_t> open_path;
	int32_t opened_fd_handle;
	std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* file_descriptors;
	std::function<void(SimpleMem::Request*)> send_mem_req;

};

}
}

#endif
