// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_READ_STATE
#define _H_VANADIS_OS_READ_STATE

#include <cstdio>
#include <functional>

#include "os/node/vnodeoshstate.h"
#include "util/vlinesplit.h"
#include <sst/core/interfaces/stdMem.h>


using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisReadHandlerState : public VanadisHandlerState {
public:
    VanadisReadHandlerState(uint32_t verbosity, int file_descriptor, uint64_t buffer_addr, uint64_t buffer_count,
                             std::function<void(StandardMem::Request*)> send_r)
        : VanadisHandlerState(verbosity), file_descriptor(file_descriptor), read_buff(buffer_addr),
          read_count(buffer_count) {

        send_mem_req = send_r;
        total_read = 0;

        // get the length of the first write to memory, it cannot span a cacheline  
        uint64_t memWriteLen = vanadis_line_remainder(read_buff,64);
        memWriteLen = read_count < memWriteLen ? read_count : memWriteLen;

        // read enough from the file to fill the mem request
		std::vector<uint8_t> payload(memWriteLen);
        ssize_t numRead = read( file_descriptor, &payload[0], memWriteLen );

        send_mem_req( new StandardMem::Write(buffer_addr, numRead, payload) );

		total_read += numRead;

        // if we have reached the end of the file
		if ( numRead < memWriteLen ) {
            read_count = total_read;
		}
    }

    ~VanadisReadHandlerState() { }

    virtual void handleIncomingRequest(StandardMem::Request* req) {
        
        assert( req->getSuccess() );

        if ( total_read < read_count  ) {

            const uint64_t read_len = ((read_count - total_read) < 64) ? read_count - total_read : 64;

			std::vector<uint8_t> payload(read_len);
        	ssize_t numRead = read( file_descriptor, &payload[0], read_len );
        	send_mem_req( new StandardMem::Write(read_buff + total_read, read_len, payload) );
			total_read += numRead;

            // if we have reached the end of the file
			if ( numRead < read_len ) {
                read_count = total_read;
			}
        } else {
            markComplete();
        }
    }

    virtual VanadisSyscallResponse* generateResponse() { return new VanadisSyscallResponse(total_read); }

protected:
    const uint64_t  read_buff;
    uint64_t        read_count;
    uint64_t        total_read;
    int             file_descriptor;

    std::function<void(StandardMem::Request*)> send_mem_req;
};

} // namespace Vanadis
} // namespace SST

#endif
