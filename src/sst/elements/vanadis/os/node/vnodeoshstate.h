
#ifndef _H_VANADIS_OS_HANDLER_STATE
#define _H_VANADIS_OS_HANDLER_STATE

#include <sst/core/interfaces/simpleMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisHandlerState {
public:
        VanadisHandlerState( uint32_t verb ) {
		output = new SST::Output( "[os-func-handler]: ", verb, 0, SST::Output::STDOUT );
	}

        ~VanadisHandlerState() {
		delete output;
	}

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {}

protected:
	SST::Output* output;

};

}
}

#endif
