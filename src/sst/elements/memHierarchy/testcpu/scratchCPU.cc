// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "testcpu/scratchCPU.h"
#include "util.h"

#include <sst/core/interfaces/stringEvent.h>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

ScratchCPU::ScratchCPU(ComponentId_t id, Params& params) : Component(id), rng(id, 13)
{
    // Restart the RNG to ensure completely consistent results 
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", params.find<int>("verbose", 0), 0, Output::STDOUT);

    // Memory parameters
    scratchSize = params.find<uint64_t>("scratchSize", 0);
    maxAddr = params.find<uint64_t>("maxAddr", 0);
    scratchLineSize = params.find<uint64_t>("scratchLineSize", 64);
    memLineSize = params.find<uint64_t>("memLineSize", 64);

    if (!scratchSize) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchSize' - must be at least 1", getName().c_str());
    if (maxAddr <= scratchSize) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'maxAddr' - must be larger than 'scratchSize'", getName().c_str());
    if (scratchLineSize < 1) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchLineSize' - must be greater than 0\n", getName().c_str());
    if (memLineSize < scratchLineSize) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'memLineSize' - must be greater than or equal to 'scratchLineSize'\n", getName().c_str());
    if (!isPowerOfTwo(scratchLineSize)) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchLineSize' - must be a power of 2\n", getName().c_str());
    if (!isPowerOfTwo(memLineSize)) out.fatal(CALL_INFO, -1, "Error (%s): invalid param 'memLineSize' - must be a power of 2\n", getName().c_str());

    log2ScratchLineSize = log2Of(scratchLineSize);
    log2MemLineSize = log2Of(memLineSize);

    // CPU parameters
    UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");
    clockHandler = new Clock::Handler<ScratchCPU>(this, &ScratchCPU::tick);
    clockTC = registerClock( clock, clockHandler );

    reqQueueSize = params.find<uint32_t>("maxOutstandingRequests", 8);
    reqPerCycle = params.find<uint32_t>("maxRequestsPerCycle", 2);

    reqsToIssue = params.find<uint64_t>("reqsToIssue", 1000);

    
    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    std::string size = params.find<std::string>("scratchSize", "0");
    size += "B";
    params.insert("scratchpad_size", size);

    memory = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.scratchInterface", this, params));
    if ( !memory ) {
        out.fatal(CALL_INFO, -1, "Unable to load scratchInterface subcomponent\n");
    }

    memory->initialize("mem_link",
			new Interfaces::SimpleMem::Handler<ScratchCPU>(this, &ScratchCPU::handleEvent) );

    // Initialize local variables
    timestamp = 0;
    num_events_issued = num_events_returned = 0;
}

// SST Component functions
void ScratchCPU::init(unsigned int phase) {
    memory->init(phase);
}

void ScratchCPU::finish() {
    out.output("ScratchCPU %s Finished after %" PRIu64 " issued memory events, %" PRIu64 " returned, %" PRIu64 " cycles\n",
            getName().c_str(), num_events_issued, num_events_returned, timestamp);
}

// Clock tick - create and send events here
bool ScratchCPU::tick(Cycle_t time) {
    timestamp++;

    if (num_events_issued == reqsToIssue) {
        if (requests.empty()) {
            primaryComponentOKToEndSim(); // Have issued all our requests and all have returned -> DONE!
            return true;
        }
    } else {
        // Can we issue requests this cycle?
        if (requests.size() < reqQueueSize) {
            // Determine how many requests to issue this cycle
            uint32_t reqCount = rng.generateNextUInt32() % (reqPerCycle + 1);
            
            // Create and send requests
            for (int i = 0; i < reqCount; i++) {
                
                // Determine what kind of request to send -> 6 options
                uint32_t instType = rng.generateNextUInt32() % 6;
                
                Interfaces::SimpleMem::Request * req;
                if (instType == 0) { // Scratch read
                    // Generate request size
                    uint32_t log2Size = rng.generateNextUInt32() % (log2ScratchLineSize + 1);
                    uint32_t size = 1 << log2Size;
                    // Generate address aligned to request size
                    Interfaces::SimpleMem::Addr addr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % scratchSize) >> log2Size) << log2Size);
                    
                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Read, addr, size);
                    out.debug(_L3_, "ScratchCPU (%s) sending Read. Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), addr, size);
                } else if (instType == 1) { // Scratch write
                    // Generate request size
                    uint32_t log2Size = rng.generateNextUInt32() % (log2ScratchLineSize + 1);
                    uint32_t size = 1 << log2Size;
                    Interfaces::SimpleMem::Addr addr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % scratchSize) >> log2Size) << log2Size);
                    std::vector<uint8_t> data;
                    data.resize(size, 0);
                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Write, addr, size, data);
                    out.debug(_L3_, "ScratchCPU (%s) sending Write. Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), addr, size);
                } else if (instType == 2) { // Scratch Get (copy from memory to scratch)
                    uint32_t log2Size = rng.generateNextUInt32() % (log2MemLineSize + 1);
                    uint32_t size = 1 << log2Size;
                    
                    Interfaces::SimpleMem::Addr srcAddr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % (maxAddr - scratchSize)) >> log2Size ) << log2Size);
                    Interfaces::SimpleMem::Addr dstAddr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % scratchSize) >> log2Size) << log2Size );
                    srcAddr += scratchSize;

                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Read, dstAddr, size);
                    req->addAddress(srcAddr);
                    out.debug(_L3_, "ScratchCPU (%s) sending ScratchGet. Dst Addr: %" PRIu64 ", Src Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), dstAddr, srcAddr, size);
                } else if (instType == 3) { // Scratch Put (copy from scratch to memory)
                    uint32_t log2Size = rng.generateNextUInt32() % (log2MemLineSize + 1);
                    uint32_t size = 1 << log2Size;

                    Interfaces::SimpleMem::Addr srcAddr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % scratchSize) >> log2Size) << log2Size);
                    Interfaces::SimpleMem::Addr dstAddr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % (maxAddr - scratchSize)) >> log2Size ) << log2Size);
                    dstAddr += scratchSize;

                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Write, dstAddr, size);
                    req->addAddress(srcAddr);
                    out.debug(_L3_, "ScratchCPU (%s) sending ScratchPut. Dst Addr: %" PRIu64 ", Src Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), dstAddr, srcAddr, size);
                } else if (instType == 4) { // Memory read
                    uint32_t log2Size = rng.generateNextUInt32() % (log2MemLineSize + 1);
                    uint32_t size = 1 << log2Size;

                    Interfaces::SimpleMem::Addr addr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % (maxAddr - scratchSize)) >> log2Size ) << log2Size);
                    addr += scratchSize;
                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Read, addr, size);
                    out.debug(_L3_, "ScratchCPU (%s) sending mem Read. Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), addr, size);
                } else { // Memory write
                    uint32_t log2Size = rng.generateNextUInt32() % (log2MemLineSize + 1);
                    uint32_t size = 1 << log2Size;

                    Interfaces::SimpleMem::Addr addr = (Interfaces::SimpleMem::Addr) (((rng.generateNextUInt64() % (maxAddr - scratchSize)) >> log2Size ) << log2Size);
                    addr += scratchSize;
                    
                    std::vector<uint8_t> data;
                    data.resize(size, 0);
                    
                    req = new Interfaces::SimpleMem::Request(Interfaces::SimpleMem::Request::Write, addr, size, data);
                    out.debug(_L3_, "ScratchCPU (%s) sending mem Write. Addr: %" PRIu64 ", Size: %u\n\n", getName().c_str(), addr, size);
                }

		// Send request
                memory->sendRequest(req);
		requests[req->id] = timestamp;

                // Update counter info
                num_events_issued++;

                // Check if we can issue more
                if (requests.size() == reqQueueSize) break;
                if (num_events_issued == reqsToIssue) break;
            }
        }
    }
    return false;
}

// Memory response handler
void ScratchCPU::handleEvent(Interfaces::SimpleMem::Request * response) {
    std::unordered_map<uint64_t, SimTime_t>::iterator i = requests.find(response->id);
    if (i == requests.end()) out.fatal (CALL_INFO, -1, "Received response but request not found! ID = %" PRIu64 "\n", response->id);
    requests.erase(i);
    num_events_returned++;
    delete response;
}

