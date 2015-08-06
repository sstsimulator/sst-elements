// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "membackend/dramSimBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

DRAMSimMemory::DRAMSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string deviceIniFilename = params.find_string("device_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == deviceIniFilename)
        ctrl->dbg.fatal(CALL_INFO, -1, "Model must define a 'device_ini' file parameter\n");
    std::string systemIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == systemIniFilename)
        ctrl->dbg.fatal(CALL_INFO, -1, "Model must define a 'system_ini' file parameter\n");


    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
    if(0 == ramSize) {
	ctrl->dbg.fatal(CALL_INFO, -1, "DRAMSim backend.mem_size parameter set to zero. Not allowed, must be power of two.\n");
    }

    memSystem = DRAMSim::getMemorySystemInstance(
            deviceIniFilename, systemIniFilename, "", "", ramSize);

    DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>
        *readDataCB, *writeDataCB;

    readDataCB = new DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &DRAMSimMemory::dramSimDone);
    writeDataCB = new DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &DRAMSimMemory::dramSimDone);

    memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
}



bool DRAMSimMemory::issueRequest(DRAMReq *req){
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    bool ok = memSystem->willAcceptTransaction(addr);
    if(!ok) return false;
    ok = memSystem->addTransaction(req->isWrite_, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(_L10_, "Issued transaction for address %" PRIx64 "\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}



void DRAMSimMemory::clock(){
    memSystem->update();
}



void DRAMSimMemory::finish(){
    memSystem->printStats(true);
}



void DRAMSimMemory::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    std::deque<DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
    assert(reqs.size());
    DRAMReq *req = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}
