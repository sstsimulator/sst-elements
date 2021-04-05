// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/elements/memHierarchy/util.h"
#include "membackend/ramulatorBackend.h"

#include "Config.h"
#include "Request.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace ramulator;


ramulatorMemory::ramulatorMemory(ComponentId_t id, Params &params) :
    SimpleMemBackend(id, params),
    callBackFunc(std::bind(&ramulatorMemory::ramulatorDone, this, std::placeholders::_1))
{
    std::string ramulatorCfg = params.find<std::string>("configFile",
                                                        NO_STRING_DEFINED);
    if (ramulatorCfg == NO_STRING_DEFINED) {
        output->fatal(CALL_INFO, -1, "Ramulator Backend must define a 'configFile' file parameter\n");
    }
    ramulator::Config configs(ramulatorCfg);

    configs.set_core_num(1); // ?

    memSystem = new Gem5Wrapper(configs, m_reqWidth); // default cache line to 64 byte

    output->output(CALL_INFO, "Instantiated Ramulator from config file %s\n", ramulatorCfg.c_str());
}

bool ramulatorMemory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes){
    ramulator::Request::Type type = (isWrite)
        ? (ramulator::Request::Type::WRITE) : (ramulator::Request::Type::READ);

    ramulator::Request request(addr,
                               type,
                               callBackFunc,
                               0);  /* context or core ID. ? */

    bool ok = memSystem->send(request);
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "RamulatorBackend: Attempting to issue %s request for %" PRIx64 ". Accepted: %d\n", (isWrite ? "WRITE" : "READ"), addr, ok);
#endif
    if(!ok) return false;

    // save this DRAM Request
    if (isWrite)
        writes.insert(reqId);
    else
        dramReqs[addr].push_back(reqId);

    return ok;
}

bool ramulatorMemory::clock(Cycle_t cycle){
    memSystem->tick();
    // Ack writes since ramulator won't
    while (!writes.empty()) {
        handleMemResponse(*writes.begin());
        writes.erase(writes.begin());
    }
    return false;
}

void ramulatorMemory::finish(){
    memSystem->finish();
}


void ramulatorMemory::ramulatorDone(ramulator::Request& ramReq) {
    uint64_t addr = ramReq.addr;
    std::deque<ReqId> &reqs = dramReqs[addr];

#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "RamulatorBackend: Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
#endif
    // Clean up dramReqs
    if (reqs.size() < 1)
        output->fatal(CALL_INFO, -1, "RamulatorBackend: Error - ramulatorDone called but dramReqs[addr] is empty. Addr: %" PRIx64 "\n", (Addr)addr);

    ReqId req = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    handleMemResponse(req);
}
