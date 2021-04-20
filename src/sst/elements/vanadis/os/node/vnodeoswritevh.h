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

#ifndef _H_VANADIS_OS_WRITEV_STATE
#define _H_VANADIS_OS_WRITEV_STATE

#include <cstdio>
#include <vector>

#include <sst/core/interfaces/simpleMem.h>
#include "os/node/vnodeoshstate.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisWritevHandlerState : public VanadisHandlerState {
public:
        VanadisWritevHandlerState( uint32_t verbosity, int64_t fd, uint64_t iovec_addr,
                int64_t iovec_count, FILE* handle, std::function<void(SimpleMem::Request*)> send_r ) :
                VanadisHandlerState(verbosity), writev_fd(fd), writev_iovec_addr(iovec_addr),
                writev_iovec_count(iovec_count), file_handle(handle),
                send_mem_req(send_r) {

                reset_iovec();
                current_offset = 0;
                current_iovec  = 0;
                total_bytes_written = 0;

                state = 0;
        }

        void reset_iovec() {
                current_iovec_base_addr = UINT64_MAX;
                current_iovec_length    = INT64_MAX;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "[syscall-writev] processing incoming request (addr: 0x%llx, size: %" PRIu64 ")\n",
			req->addr, (uint64_t) req->size );

                switch( state ) {
                case 0:
                        {
                                current_iovec_base_addr = (uint64_t) (*( (uint32_t*)( &req->data[0] ) ));
				output->verbose(CALL_INFO, 16, 0, "iovec-data-address: 0x%llx\n", current_iovec_base_addr);

                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                        writev_iovec_addr + (current_iovec * 8) + 4, 4 ) );
                                state++;
                        }
                        break;
                case 1:
                        {
                                current_iovec_length = (int64_t)( *( (int32_t*)( &req->data[0] ) ) );
                                uint64_t base_addr_offset = (current_iovec_base_addr % 64);

				output->verbose(CALL_INFO, 16, 0, "iovec-data-len: %" PRIu64 "\n",
					current_iovec_length);

				if( current_iovec_length > 0 ) {
	                                if( (base_addr_offset + current_iovec_length) <= 64 ) {
		                        	// we only need to do one read and we are done
       	                                 	send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                	current_iovec_base_addr, current_iovec_length ) );
                                        	state++;
                                	} else {
                                        	send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                	current_iovec_base_addr, 64 - base_addr_offset ) );
                                        	state++;
                                	}
				} else {
					current_iovec++;

                                        if( current_iovec < writev_iovec_count ) {
                                                current_offset = 0;

                                                // Launch the next iovec read
                                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                        writev_iovec_addr + (current_iovec * 8), 4 ) );
                                                state = 0;
                                        } else {
                                                output->verbose(CALL_INFO, 16, 0, "iovec processing is completed.\n");
                                                printStatus();

                                                state = 3;
						dump_buffer();
                                                markComplete();
                                        }
				}
                        }
                        break;
                case 2:
                        {
                                // Write out the payload
				output->verbose(CALL_INFO, 16, 0, "--> update buffer data-offset: %" PRIu64 " + payload: %" PRIu64 " (iovec-data-len: %" PRIu64 ")\n",
					current_offset, (uint64_t) req->size, current_iovec_length );

				merge_to_buffer( req->data );
                                current_offset += req->size;

                                if( current_offset < current_iovec_length ) {
                                        if( ( current_offset + 64 ) < current_iovec_length ) {
                                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                         current_iovec_base_addr + current_offset, 64 ) );
                                        } else {
                                                uint64_t remainder = current_iovec_length - current_offset;
                                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                         current_iovec_base_addr + current_offset, remainder ) );
                                        }
                                } else {
                                        current_iovec++;

                                        if( current_iovec < writev_iovec_count ) {
                                                current_offset = 0;

                                                // Launch the next iovec read
                                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                        writev_iovec_addr + (current_iovec * 8), 4 ) );
                                                state = 0;
                                        } else {
						output->verbose(CALL_INFO, 16, 0, "iovec processing is completed.\n");
						printStatus();

                                                state = 3;
						dump_buffer();
						markComplete();
                                        }
                                }
                        }
                        break;
                case 3:
                        {
				dump_buffer();

                                // We are done here, don't do anything.
				markComplete();
                        }
                        break;
                }

        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( total_bytes_written );
	}

	void merge_to_buffer( std::vector<uint8_t>& payload ) {
		for( size_t i = 0; i < payload.size(); ++i ) {
			buffer.push_back( payload[i] );
		}

		//print_buffer();
	}

	void clear_buffer() {
		buffer.clear();
	}

	void dump_buffer() {
		if( buffer.size() > 0 ) {
			fwrite( &buffer[0], buffer.size(), 1, file_handle );
			fflush( file_handle );

			total_bytes_written += buffer.size();
		}

		buffer.clear();
	}

	void print_buffer() {
		for( size_t i = 0; i < buffer.size(); ++i ) {
			printf("%c", buffer[i]);
		}

		printf("\n");
	}

	void printStatus() {
		output->verbose(CALL_INFO, 16, 0, "writev Handler Status\n");
		output->verbose(CALL_INFO, 16, 0, "-> fd:                %" PRId64 "\n", writev_fd);
		output->verbose(CALL_INFO, 16, 0, "-> iovec_addr:        0x%llx\n", writev_iovec_addr);
		output->verbose(CALL_INFO, 16, 0, "-> iovec_count:       %" PRId64 "\n", writev_iovec_count);
		output->verbose(CALL_INFO, 16, 0, "-> current iovec state:\n");
		output->verbose(CALL_INFO, 16, 0, "---> current_base:    0x%llx\n", current_iovec_base_addr);
		output->verbose(CALL_INFO, 16, 0, "---> current_len:     %" PRId64 "\n", current_iovec_length);
		output->verbose(CALL_INFO, 16, 0, "---> current_iovec:   %" PRId64 "\n", current_iovec);
		output->verbose(CALL_INFO, 16, 0, "---> current_offset:  %" PRId64 "\n", current_offset);
		output->verbose(CALL_INFO, 16, 0, "-> total-bytes:       %" PRId64 "\n", total_bytes_written);
		output->verbose(CALL_INFO, 16, 0, "-> current state:     %" PRId32 "\n", state);
	}

protected:
        int64_t  writev_fd;
        uint64_t writev_iovec_addr;
        int64_t  writev_iovec_count;

        uint64_t current_iovec_base_addr;
        int64_t  current_iovec_length;

        int64_t  current_iovec;
        int64_t  current_offset;

        int64_t  total_bytes_written;
        int32_t  state;

        FILE* file_handle;
        std::function<void(SimpleMem::Request*)> send_mem_req;
	std::vector<uint8_t> buffer;
};

}
}

#endif
