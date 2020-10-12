
#ifndef _H_VANADIS_OS_BRK_HANDLER
#define _H_VANADIS_OS_BRK_HANDLER

#include <cstdint>
#include <functional>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisBRKHandlerState : public VanadisHandlerState {
public:
        VanadisBRKHandlerState( uint32_t verbosity, uint64_t return_v ) :
		VanadisHandlerState( verbosity ), rc(return_v) {

		completed = true;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {
		output->verbose(CALL_INFO, 16, 0, "-> [syscall-brk] handle incoming event for brk(), req->size: %" PRIu32 "\n",
			(uint32_t) req->size);
        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( rc );
	}

protected:
	uint64_t rc;

};

}
}

#endif
