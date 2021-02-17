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

#ifndef _H_VANADIS_OS_WRITE_STATE
#define _H_VANADIS_OS_WRITE_STATE

#include <cstdio>
#include <functional>

#include <sst/core/interfaces/simpleMem.h>
#include "os/node/vnodeoshstate.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisWriteHandlerState : public VanadisHandlerState {
public:
        VanadisWriteHandlerState( uint32_t verbosity, FILE* handle, int64_t fd, uint64_t buffer_addr,
		uint64_t buffer_count, std::function<void(SimpleMem::Request*)> send_r) :
			VanadisHandlerState(verbosity), file_handle(handle),
			write_fd(fd), write_buff(buffer_addr), write_count(buffer_count) {

		send_mem_req = send_r;
		total_written = 0;
	}

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "[syscall-write] processing incoming request (addr: 0x%llx, size: %" PRIu64 ")\n",
			req->addr, (uint64_t) req->size );

		if( req->size > 0 ) {
			fwrite( &req->data[0], 1, req->size, file_handle );
			total_written += req->size;
		}

		if( total_written < write_count ) {
			const uint64_t read_len = ( (write_count - total_written) < 64 ) ?
				write_count - total_written : 64;

			send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
				write_buff + total_written, read_len ) );
		} else {
			// call flush so data becomes visible
			fflush( file_handle );
			markComplete();
		}
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( total_written );
	}

protected:
	const int64_t write_fd;
	const uint64_t write_buff;
	const uint64_t write_count;

	uint64_t total_written;

        FILE* file_handle;
        std::function<void(SimpleMem::Request*)> send_mem_req;
};

}
}

#endif
