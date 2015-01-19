
#ifndef _H_SST_MEMH_DRAMSIM_BACKEND
#define _H_SST_MEMH_DRAMSIM_BACKEND

#include "membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <DRAMSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class DRAMSimMemory : public MemBackend {
public:
    DRAMSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
    void clock();
    void finish();

private:
    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    DRAMSim::MultiChannelMemorySystem *memSystem;
    std::map<uint64_t, std::deque<MemController::DRAMReq*> > dramReqs;
};

}
}

#endif
