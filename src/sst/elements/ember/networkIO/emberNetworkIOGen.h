#pragma once
#include "sst/elements/ember/embergen.h"
#include "sst/elements/ember/libs/emberLib.h"
#include "sst/elements/ember/libs/emberShmemLib.h"
#include "sst/elements/ember/libs/emberNetworkIOLib.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberNetworkIOGenerator : public EmberGenerator {
public:
    EmberNetworkIOGenerator(ComponentId_t id, Params& params, std::string name = "");
    ~EmberNetworkIOGenerator() {}
    virtual void completed(const SST::Output*, uint64_t time) {}
    virtual void setup();

protected:
    EmberShmemLib* m_shmemLib;
    
    
    EmberNetworkIOLib* m_networkIOLib;
    EmberNetworkIOLib& networkIO() { return *m_networkIOLib; }

    EmberShmemLib& shmem() { return *m_shmemLib; }
    
    // Barrier macro (same as EmberShmemGen)
    #define enQ_barrier_all shmem().barrier_all
    #define enQ_malloc shmem().malloc
    #define enQ_my_pe shmem().my_pe
};

}
}

