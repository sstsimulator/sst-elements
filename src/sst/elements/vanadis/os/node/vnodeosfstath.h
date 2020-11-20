
#ifndef _H_VANADIS_OS_FSTAT_HANDLER
#define _H_VANADIS_OS_FSTAT_HANDLER

#include <cstdint>
#include <functional>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisFstatHandlerState : public VanadisHandlerState {
public:
        VanadisFstatHandlerState( uint32_t verbosity, int fd, uint64_t stat_a ) :
		VanadisHandlerState( verbosity ), file_handle(fd), stat_addr(stat_a) {

		completed = true;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "-> [syscall-fstat] handle incoming event for fstat(), req->size: %" PRIu32 "\n", 
			(uint32_t) req->size);
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( 0 );
	}

private:
	const int32_t file_handle;
	const uint64_t stat_addr;

};

}
}

#endif
