
#ifndef _H_VANADIS_OS_UNAME_HANDLER
#define _H_VANADIS_OS_UNAME_HANDLER

#include <cstdint>
#include <functional>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisUnameHandlerState : public VanadisHandlerState {
public:
        VanadisUnameHandlerState( uint32_t verbosity ) :
		VanadisHandlerState( verbosity ) {

		completed = true;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "-> [syscall-uname] handle incoming event for uname(), req->size: %" PRIu32 "\n",
			(uint32_t) req->size);
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( 0 );
	}
};

}
}

#endif
