
#ifndef _H_SST_MEMH_VAULT_SIM_BACKEND
#define _H_SST_MEMH_VAULT_SIM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class VaultSimMemory : public MemBackend {
public:
    VaultSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
private:
    void handleCubeEvent(SST::Event *event);

    typedef std::map<MemEvent::id_type,MemController::DRAMReq*> memEventToDRAMMap_t;
    memEventToDRAMMap_t outToCubes; // map of events sent out to the cubes
    SST::Link *cube_link;
};

}
}

#endif
