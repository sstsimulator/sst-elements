// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/ramulator2Backend.h"

using namespace SST;
using namespace SST::MemHierarchy;


ramulator2Memory::ramulator2Memory(ComponentId_t id, Params &params) :
    SimpleMemBackend(id, params),
    callBackFunc(std::bind(&ramulator2Memory::ramulator2Done, this, std::placeholders::_1))
{
    config_path = params.find<std::string>("configFile",
                                            NO_STRING_DEFINED);
    if (config_path == NO_STRING_DEFINED) {
        output->fatal(CALL_INFO, -1, "Ramulator2 Backend must define a 'configFile' file parameter\n");
    }
    YAML::Node config = Ramulator::Config::parse_config_file(config_path, {});
    ramulator2_frontend = Ramulator::Factory::create_frontend(config);
    ramulator2_memorysystem = Ramulator::Factory::create_memory_system(config);

    configs.set_core_num(1); // this is from the ramulator code and even there it had a "?"... do I keep it?

    ramulator2_frontend->connect_memory_system(ramulator2_memorysystem);
    ramulator2_memorysystem->connect_frontend(ramulator2_frontend);

    output->output(CALL_INFO, "Instantiated Ramulator2 from config file %s\n", ramulator2Cfg.c_str());
}

bool ramulator2Memory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes){
    Ramulator::Request::Type reqType = (isWrite) ? Ramulator::Request::Type::Write 
                                        : Ramulator::Request::Type::Read;
    bool enqueue_success = ramulator2_frontend->receive_external_requests(reqType, addr, reqId, 
    [this](Ramulator::Request& req) {
      // your read request callback 
    });

    if (!enqueue_success) {
        // error?
    }
    /*

    ramulator::Request request(addr,
                               type,
                               callBackFunc,
                               0);  /* context or core ID. ? */
    */
    bool ok = memSystem->send(request);
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Ramulator2Backend: Attempting to issue %s request for %" PRIx64 ". Accepted: %d\n", (isWrite ? "WRITE" : "READ"), addr, ok);
#endif
    if(!ok) return false;

    // save this DRAM Request
    if (isWrite)
        writes.insert(reqId);
    else
        dramReqs[addr].push_back(reqId);

    return ok;
}

bool ramulator2Memory::clock(Cycle_t cycle){
    memSystem->tick();
    // Ack writes since ramulator won't
    while (!writes.empty()) {
        handleMemResponse(*writes.begin());
        writes.erase(writes.begin());
    }
    return false;
}

void ramulator2Memory::finish(){
    ramulator2_frontend->finalize();
    ramulator2_memorysystem->finalize();
}


void ramulator2Memory::ramulator2Done(ramulator::Request& ramReq) {
    uint64_t addr = ramReq.addr;
    std::deque<ReqId> &reqs = dramReqs[addr];

#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Ramulator2Backend: Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
#endif
    // Clean up dramReqs
    if (reqs.size() < 1)
        output->fatal(CALL_INFO, -1, "Ramulator2Backend: Error - ramulator2Done called but dramReqs[addr] is empty. Addr: %" PRIx64 "\n", (Addr)addr);

    ReqId req = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    handleMemResponse(req);
}
