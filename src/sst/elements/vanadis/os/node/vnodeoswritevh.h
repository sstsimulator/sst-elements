
#ifndef _H_VANADIS_OS_WRITEV_STATE
#define _H_VANADIS_OS_WRITEV_STATE

#include <cstdio>
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

        int64_t  getFileDescriptor() const { return writev_fd; }
        uint64_t getIOVecAddress() const { return writev_iovec_addr; }
        int64_t  getIOVecCount() const   { return writev_iovec_count; }

        void reset_iovec() {
                current_iovec_base_addr = UINT64_MAX;
                current_iovec_length    = INT64_MAX;
        }

        uint64_t getCurrentIOVecBase() const   { return current_iovec_base_addr; }
        int64_t  getCurrentIOVecLength() const { return current_iovec_length;    }

        void setCurrentIOVecBase( const uint64_t new_addr ) {
                current_iovec_base_addr = new_addr;
        }

        void setCurrentIOVecLength( const int64_t new_len ) {
                current_iovec_length = new_len;
        }

        int64_t getCurrentIOVec() const  { return current_iovec;   }
        int64_t getCurrentOffset() const { return current_offset;  }

        void incrementIOVec() { current_iovec++; }
        void addToCurrentOffset( int64_t add_amount ) { current_offset += add_amount; }
        void resetCurrentOffset() { current_offset = 0; }

        int64_t getTotalBytesWritten() const { return total_bytes_written; }
        void addToByteCount( int64_t bytes ) { total_bytes_written += bytes; }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {

		printStatus();

                switch( state ) {
                case 0:
                        {
                                current_iovec_base_addr = *( (uint64_t*)( &req->data[0] ) );
                                send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                        writev_iovec_addr + (current_iovec * 8) + 4, 4 ) );
                                state++;
                        }
                        break;
                case 1:
                        {
                                current_iovec_length = *( (int64_t*)( &req->data[0] ) );
                                uint64_t base_addr_offset = 64 - (current_iovec_base_addr % 64);

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

                        }
                        break;
                case 2:
                        {
                                // Write out the payload
				printf("fwrite, size: %" PRIu32 " handle: 0x%0llx\n", (uint32_t) req->size, file_handle);
                                fwrite( &req->data[0], req->size, 1, file_handle );
				fflush( file_handle );

                                total_bytes_written += req->size;
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
                                                state = 3;
                                        }
                                }
                        }
                        break;
                case 3:
                        {
                                // We are done here, don't do anything.
                        }
                        break;
                }

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
};

}
}

#endif
