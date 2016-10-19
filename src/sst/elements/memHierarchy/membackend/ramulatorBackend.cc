// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include "membackend/ramulatorBackend.h"

#include "Config.h"
#include "Request.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace ramulator;

ramulatorMemory::ramulatorMemory(Component *comp, Params &params) : 
    MemBackend(comp, params),
    callBackFunc(std::bind(&ramulatorMemory::ramulatorDone, this, std::placeholders::_1))
{
    std::string ramulatorCfg = params.find<std::string>("configFile",
                                                        NO_STRING_DEFINED);
    if (ramulatorCfg == NO_STRING_DEFINED) {
        ctrl->dbg.fatal(CALL_INFO, -1, "Ramulator Backend must define a 'configFile' file parameter\n");
    } 
    ramulator::Config configs(ramulatorCfg);
    
    configs.set_core_num(1); // ?

    memSystem = new Gem5Wrapper(configs,64); // default cache line to 64 byte

    output->output(CALL_INFO, "Instantiated Ramulator from config file %s\n", ramulatorCfg.c_str());
}

bool ramulatorMemory::issueRequest(DRAMReq *req){
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    ramulator::Request::Type type = (req->isWrite_) 
        ? (ramulator::Request::Type::WRITE) : (ramulator::Request::Type::READ);

    ramulator::Request request(addr, 
                               type,
                               callBackFunc, 
                               0);  /* context or core ID. ? */
    bool ok = memSystem->send(request);
    if(!ok) return false;

    // save this DRAM Request
    dramReqs[addr].push_back(req); 

    return ok;
}

void ramulatorMemory::clock(){
    memSystem->tick();
}

void ramulatorMemory::finish(){
    memSystem->finish();
}


void ramulatorMemory::ramulatorDone(ramulator::Request& ramReq) {
    uint64_t addr = ramReq.addr;
    std::deque<DRAMReq *> &reqs = dramReqs[addr];

#ifdef __SST_DEBUG_OUTPUT__
    ctrl->dbg.debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
#endif
    assert(reqs.size());
    DRAMReq *req = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}
