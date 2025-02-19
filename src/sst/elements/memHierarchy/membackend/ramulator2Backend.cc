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
#include "base/config.h"

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

    ramulator2_frontend->connect_memory_system(ramulator2_memorysystem);
    ramulator2_memorysystem->connect_frontend(ramulator2_frontend);

    //TODO: add call to memory_system->init or frontend->init?

    output->output(CALL_INFO, "Instantiated Ramulator2 from config file %s\n", config_path);
}

bool ramulator2Memory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes){
    // create request type variable
    auto req_type = isWrite ? Ramulator::Request::Type::Write : Ramulator::Request::Type::Read;
    // build Request for Ramulator
    Ramulator::Request req(addr, req_type, reqId, callBackFunc);
    // send request
    bool enqueue_success = ramulator2_frontend->receive_external_requests(req_type, addr, reqId,
    [this](Ramulator::Request& req) {
        if (req.type_id == Ramulator::Request::Type::Write) {
            // TODO: write request callback -- NOT BEING CALLED
#ifdef __SST_DEBUG_OUTPUT__
            output->debug(_L10_, "Ramulator2Backend: Write callback for %" PRIx64 ".\n", req.addr);
#endif
            this->writes.insert(req.source_id);
        } else {
            // TODO: read request callback -- NOT BEING CALLED
#ifdef __SST_DEBUG_OUTPUT__
            output->debug(_L10_, "Ramulator2Backend: Read callback for %" PRIx64 ".\n", req.addr);
#endif
            this->dramReqs[req.addr].push_back(req.source_id);
        }
    });
// TODO: why isn't this output when debug mode is active?
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Ramulator2Backend: Attempting to issue %s request for %" PRIx64 ". Accepted: %d\n", (isWrite ? "WRITE" : "READ"), addr, enqueue_success);
#endif
    return enqueue_success;
}

bool ramulator2Memory::clock(Cycle_t cycle){
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Ramulator2Backend: Ticking memory system.\n");
#endif
    ramulator2_frontend->tick();
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

// TODO: CONFIRMED THAT THIS FUNCTION IS CALLED UPON FINISH -- WHY DOES IT CAUSE A CRASH? COPMILE RAMULATOR IN DEBUG AND GDB INTO DRAMCONTROLLER LINE 311
void ramulator2Memory::ramulator2Done(Ramulator::Request& ramReq) {
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
