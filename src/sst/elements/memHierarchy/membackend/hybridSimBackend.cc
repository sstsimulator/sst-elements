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
#include "membackend/memBackend.h"
#include "membackend/hybridSimBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

HybridSimMemory::HybridSimMemory(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    output->init("@R:HybridSimMemory::@p():@l " + getName() + ": ", 0, 0,
                         (Output::output_location_t)params.find<int>("debug", 0));
    std::string hybridIniFilename = params.find<std::string>("system_ini", NO_STRING_DEFINED);
    if(hybridIniFilename == NO_STRING_DEFINED)
        output->fatal(CALL_INFO, -1, "XML must define a 'system_ini' file parameter\n");

    memSystem = HybridSim::getMemorySystemInstance( 1, hybridIniFilename);

    typedef HybridSim::Callback <HybridSimMemory, void, uint, uint64_t, uint64_t> hybridsim_callback_t;
    HybridSim::TransactionCompleteCB *read_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    HybridSim::TransactionCompleteCB *write_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    memSystem->RegisterCallbacks(read_cb, write_cb);
}



bool HybridSimMemory::issueRequest( ReqId reqId, Addr addr, bool isWrite, unsigned )
{
    bool ok = memSystem->WillAcceptTransaction();
    if(!ok) return false;
    ok = memSystem->addTransaction(isWrite, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Issued transaction for address %" PRIx64 "\n", (Addr)addr);
#endif
    dramReqs[addr].push_back(reqId);
    return true;
}



bool HybridSimMemory::clock(Cycle_t cycle){
    memSystem->update();
    return false;
}



void HybridSimMemory::finish(){
    memSystem->printLogfile();
}



void HybridSimMemory::hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    std::deque<ReqId> &reqs = dramReqs[addr];
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", addr, reqs.size());
#endif
    if (reqs.size() == 0) output->fatal(CALL_INFO, -1, "Error: reqs.size() is 0 at DRAMSimMemory done\n");
    ReqId req = reqs.front();
    reqs.pop_front();
    if(reqs.size() == 0)
        dramReqs.erase(addr);

    handleMemResponse(req);
}
