
#ifndef _H_VANADIS_OS_READLINK_STATE
#define _H_VANADIS_OS_READLINK_STATE

#include <cstdio>
#include <sst/core/interfaces/simpleMem.h>
#include "os/node/vnodeoshstate.h"

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisReadLinkHandlerState : public VanadisHandlerState {
public:
        VanadisReadLinkHandlerState( uint32_t verbosity, uint64_t path_buff, uint64_t out_buff,
		int64_t buff_size, std::function<void(SimpleMem::Request*)> send_mem,
		std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block  ) :
		VanadisHandlerState( verbosity ),
		readlink_path_buff( path_buff ), readlink_out_buff( out_buff ),
		readlink_buff_size( buff_size ) {

		send_mem_req = send_mem;
		send_block_mem = send_block;
		state = 0;
		readlink_buff_bytes_written = 0;
		completed = false;
        }

	bool isCompleted() const { return completed; }
	uint64_t getPathPtr() const { return readlink_path_buff; }
	uint64_t getBufferPtr() const { return readlink_out_buff; }
	int64_t  getBufferSize() const { return readlink_buff_size; }
	int64_t getBytesWritten() const { return readlink_buff_bytes_written; }

	virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "-> handle incoming for readlink()\n");

		switch( state ) {
		case 0:
			{
				bool found_null = false;

				for( size_t i = 0; i < req->size; ++i ) {
					path.push_back(req->data[i]);

					if( req->data[i] == '\0' ) {
						found_null = true;
						break;
					}
				}

				if( found_null ) {
					output->verbose(CALL_INFO, 16, 0, "path: %s\n", (char*) (&path[0]));

					if( strcmp( (char*) (&path[0]), "/proc/self/exe" ) == 0 ) {
						const char* path_self = "/tmp/self";
						std::vector<uint8_t> real_path;

						for( int i = 0; i < strlen(path_self); ++i ) {
							real_path.push_back( path_self[i] );
						}

						output->verbose(CALL_INFO, 16, 0, "output: %s len: %" PRIu32 "\n",
							path_self, (uint32_t) real_path.size());

						send_block_mem( readlink_out_buff, real_path );
						readlink_buff_bytes_written = strlen( path_self );
					} else {
						output->fatal(CALL_INFO, -1, "Haven't implemented this yet.\n");
					}

					state = 1;
					completed = true;
				} else {
					// Read the next cache line
					send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
                                                readlink_path_buff + path.size(), 64 ) );
				}

			}
			break;
		case 1:
			{
			}
			break;
		}
        }

protected:
	bool completed;
	int state;
	uint64_t readlink_path_buff;
	uint64_t readlink_out_buff;
	int64_t readlink_buff_size;
	int64_t readlink_buff_bytes_written;
        std::function<void(SimpleMem::Request*)> send_mem_req;
	std::function<void(const uint64_t, std::vector<uint8_t>&)> send_block_mem;
	std::vector<uint8_t> path;
};

}
}

#endif
