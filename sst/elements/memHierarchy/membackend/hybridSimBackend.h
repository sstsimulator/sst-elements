
#ifndef _H_SST_MEMH_HYBRIDSIM_BACKEND
#define _H_SST_MEMH_HYBRIDSIM_BACKEND

#include "membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <HybridSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class HybridSimMemory : public MemBackend {
public:
    HybridSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
    void clock();
    void finish();
private:
    void hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    HybridSim::HybridSystem *memSystem;
    std::map<uint64_t, std::deque<MemController::DRAMReq*> > dramReqs;
};

}
}

#endif
