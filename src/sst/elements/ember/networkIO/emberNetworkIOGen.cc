#include <sst_config.h>
#include "emberNetworkIOGen.h"

using namespace SST::Ember;

EmberNetworkIOGenerator::EmberNetworkIOGenerator(ComponentId_t id, Params& params, std::string name) 
    : EmberGenerator(id, params, name), m_networkIOLib(nullptr)
{
    m_shmemLib = NULL;
}

void EmberNetworkIOGenerator::setup()
{
    m_shmemLib = static_cast<EmberShmemLib*>(getLib("shmem"));
    m_networkIOLib = static_cast<EmberNetworkIOLib*>(getLib("networkIO"));
    assert(m_shmemLib);
    assert(m_networkIOLib);
}