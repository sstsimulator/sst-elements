#pragma once

#include <strings.h>
#include <sst/core/rng/marsaglia.h>
#include "networkIO/emberNetworkIOGen.h"

namespace SST {
namespace Ember {

class EmberTestNetworkIOGenerator : public EmberNetworkIOGenerator 
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberTestNetworkIOGenerator,
        "ember",
        "TestNetworkIOMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Network IO Test",
        SST::Ember::EmberGenerator
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        {"arg.messageSize","Message size in bytes","1024"},
        {"arg.iterations","Number of iterations to perform","1"},
        {"arg.op","Operation type: read or write","write"},
        {"arg.fileSize","Storage file size in bytes","10485760"}
    )

    EmberTestNetworkIOGenerator(SST::ComponentId_t id, Params& params);
    
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    // Simulation control
    int m_phase;
    
    // Random number generation
    SST::RNG::MarsagliaRNG* m_rng;   // RNG instance for offset generation
    
    // Operation parameters
    uint32_t m_messageSize;          // Size of each read/write operation
    uint32_t m_iterations;           // Number of operations to perform
    uint64_t m_fileSize;             // Total file size for offset generation
    std::string m_opType;            // Operation type: "read" or "write"
    
    // Runtime state
    Hermes::MemAddr m_localBuffer;   // Buffer for data transfer
    
    // Performance measurement
    uint64_t m_startTime;
    uint64_t m_stopTime;
};
}
}
