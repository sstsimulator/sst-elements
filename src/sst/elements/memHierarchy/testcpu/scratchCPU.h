// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
