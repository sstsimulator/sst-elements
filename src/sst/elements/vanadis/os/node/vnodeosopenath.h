
#ifndef _H_VANADIS_OS_WRITEAT_HANDLER
#define _H_VANADIS_OS_WRITEAT_HANDLER

#include <cstdint>
#include <cinttypes>

#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisOpenAtHandlerState : public VanadisHandlerState {
public:
        VanadisOpenAtHandlerState( uint32_t verbosity, int64_t dirfd,
                uint64_t path_ptr, int64_t flags,
		std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* fds,
		std::function<void(SimpleMem::Request*)> send_m ) :
                VanadisHandlerState( verbosity ), openat_dirfd(dirfd),
                openat_path_ptr(path_ptr), openat_flags(flags) {

		send_mem_req = send_m;
		opened_fd_handle = 3;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "[syscall-openat] request processing...\n");

		bool found_null = false;

		for( size_t i = 0; i < req->size; ++i ) {
			openat_path.push_back( req->data[i] );

			if( req->data[i] == '\0' ) {
				found_null = true;
			}
		}

		if( found_null ) {
			output->verbose(CALL_INFO, 16, 0, "[syscall-openat] path at: \"%s\"\n",
				(const char*) &openat_path[0] );

			while( file_descriptors->find( opened_fd_handle ) !=
				file_descriptors->end() ) {
				opened_fd_handle++;
			}

			output->verbose(CALL_INFO, 16, 0, "[syscall-openat] new descriptor at %" PRIu32 "\n", opened_fd_handle );

			file_descriptors->insert( std::pair<uint32_t, VanadisOSFileDescriptor*>( opened_fd_handle,
				new VanadisOSFileDescriptor( opened_fd_handle, (const char*) &openat_path[0] ) ) );

			markComplete();
		} else {
			send_mem_req( new SimpleMem::Request( SimpleMem::Request::Read,
				openat_path_ptr + openat_path.size(), 64 ));
		}
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( opened_fd_handle );
	}

protected:
        const int64_t openat_dirfd;
        const uint64_t openat_path_ptr;
        const int64_t openat_flags;
        std::vector<uint8_t> openat_path;
	int32_t opened_fd_handle;
	std::unordered_map<uint32_t, VanadisOSFileDescriptor*>* file_descriptors;
	std::function<void(SimpleMem::Request*)> send_mem_req;

};

}
}

#endif
