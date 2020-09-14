
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
		completed = false;
	}

        virtual ~VanadisHandlerState() {
		delete output;
	}

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {}
	virtual VanadisSyscallResponse* generateResponse() = 0;
	virtual bool isComplete() const { return completed; }
	virtual void markComplete() { completed = true; }

protected:
	SST::Output* output;
	bool completed;

};

}
}

#endif
