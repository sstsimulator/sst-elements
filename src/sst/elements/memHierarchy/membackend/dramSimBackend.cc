// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
    std::string deviceIniFilename = params.find<std::string>("device_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == deviceIniFilename)
        ctrl->dbg.fatal(CALL_INFO, -1, "Model must define a 'device_ini' file parameter\n");
    std::string systemIniFilename = params.find<std::string>("system_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == systemIniFilename)
        ctrl->dbg.fatal(CALL_INFO, -1, "Model must define a 'system_ini' file parameter\n");


    UnitAlgebra ramSize = UnitAlgebra(params.find<std::string>("mem_size", "0B"));
    if (ramSize.getRoundedValue() % (1024*1024) != 0) {
        ctrl->dbg.fatal(CALL_INFO, -1, "For DRAMSim, backend.mem_size must be a multiple of 1MiB. Note: for units in base-10 use 'MB', for base-2 use 'MiB'. You specified '%s'\n", ramSize.toString().c_str());
    }
    unsigned int ramSizeMiB = ramSize.getRoundedValue() / (1024*1024);

    memSystem = DRAMSim::getMemorySystemInstance(
            deviceIniFilename, systemIniFilename, "", "", ramSizeMiB);

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
#ifdef __SST_DEBUG_OUTPUT__
    ctrl->dbg.debug(_L10_, "Issued transaction for address %" PRIx64 "\n", (Addr)addr);
#endif
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
