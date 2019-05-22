// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SCRATCHCPU_H
#define _SCRATCHCPU_H

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/rng/marsaglia.h>

#include <unordered_map>

using namespace std;

namespace SST {
namespace MemHierarchy {

class ScratchCPU : public Component {

public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(ScratchCPU, "memHierarchy", "ScratchCPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple test CPU for scratchpad interface", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
            {"scratchSize",             "(uint) Size of the scratchpad in bytes"},
            {"maxAddr",                 "(uint) Maximum address to generate (i.e., scratchSize + size of memory)"},
            {"rngseed",                 "(int) Set a seed for the random generator used to create requests", "7"},
            {"scratchLineSize",         "(uint) Line size for scratch, max request size for scratch", "64"},
            {"memLineSize",             "(uint) Line size for memory, max request size for memory", "64"},
            {"clock",                   "(string) Clock frequency in Hz or period in s", "1GHz"},
            {"maxOutstandingRequests",  "(uint) Maximum number of requests outstanding at a time", "8"},
            {"maxRequestsPerCycle",     "(uint) Maximum number of requests to issue per cycle", "2"},
            {"reqsToIssue",             "(uint) Number of requests to issue before ending simulation", "1000"} )

    SST_ELI_DOCUMENT_PORTS( {"mem_link", "Connection to cache", { "memHierarchy.MemEventBase" } } )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::SimpleMem"} )

/* Begin class definition */
    ScratchCPU(ComponentId_t id, Params& params);
    ~ScratchCPU() {}
    virtual void init(unsigned int phase);
    virtual void finish();
    virtual void setup() {}

private:
    void handleEvent( Interfaces::SimpleMem::Request *ev );
    virtual bool tick( Cycle_t );

    Output out;
   
    // Parameters
    uint64_t scratchSize;       // Size of scratchpad
    uint64_t maxAddr;           // Max address of memory
    uint64_t scratchLineSize;   // Line size for scratchpad -> controls maximum request size
    uint64_t memLineSize;       // Line size for memory -> controls maximum request size
    uint64_t log2ScratchLineSize;
    uint64_t log2MemLineSize;

    uint32_t reqPerCycle;   // Up to this many requests can be issued in a cycle
    uint32_t reqQueueSize;  // Maximum number of outstanding requests
    uint64_t reqsToIssue;   // Number of requests to issue before ending simulation

    // Local variables
    Interfaces::SimpleMem * memory;         // scratch interface
    std::unordered_map<uint64_t, SimTime_t> requests; // Request queue (outstanding requests)
    TimeConverter *clockTC;                 // Clock object
    Clock::HandlerBase *clockHandler;       // Clock handler
    SST::RNG::MarsagliaRNG rng;             // Random number generator for addresses, etc.

    uint64_t timestamp;     // current timestamp
    uint64_t num_events_issued;      // number of events that have been issued at a given time
    uint64_t num_events_returned;    // number of events that have returned
};

}
}
#endif /* _SCRATCHCPU_H */
