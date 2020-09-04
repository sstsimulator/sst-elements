
#ifndef _H_VANADIS_OS_HANDLER_STATE
#define _H_VANADIS_OS_HANDLER_STATE

#include <sst/core/interfaces/simpleMem.h>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisHandlerState {
public:
        VanadisHandlerState() {}
        ~VanadisHandlerState() {}

        virtual void handleIncomingRequest( SimpleMem::Request* req ) {}
};

}
}

#endif
