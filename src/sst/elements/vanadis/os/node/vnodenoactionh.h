
#ifndef _H_VANADIS_OS_NO_ACTION_HANDLER
#define _H_VANADIS_OS_NO_ACTION_HANDLER

#include <cstdint>
#include <functional>
#include "os/node/vnodeoshstate.h"

namespace SST {
namespace Vanadis {

class VanadisNoActionHandlerState : public VanadisHandlerState {
public:
        VanadisNoActionHandlerState( uint32_t verbosity ) :
		VanadisHandlerState( verbosity ), return_code(0) {

		completed = true;
        }

        VanadisNoActionHandlerState( uint32_t verbosity, int64_t rc ) :
		VanadisHandlerState( verbosity ), return_code(rc) {

		completed = true;
        }

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {

        }

	virtual VanadisSyscallResponse* generateResponse() {
		return new VanadisSyscallResponse( return_code );
	}

protected:
	int64_t return_code;

};

}
}

#endif
