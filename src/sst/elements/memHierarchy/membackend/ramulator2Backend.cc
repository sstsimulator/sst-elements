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
    SimpleMemBackend(id, params)
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

    output->output(CALL_INFO, "Instantiated Ramulator2 from config file %s\n", config_path);
}

bool ramulator2Memory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes){
    bool enqueue_success = false;

    if (isWrite) {
        enqueue_success = ramulator2_frontend->receive_external_requests(1, addr, 0,
            [this](Ramulator::Request& req) {});
        if (enqueue_success) {
            writes.insert(reqId);
        }
    } else {
        enqueue_success = ramulator2_frontend->receive_external_requests(0, addr, 0,
            [this](Ramulator::Request& req) {
                output->debug(_L10_, "Ramulator2Backend: Read callback\n");
                std::deque<ReqId> &reqs = dramReqs[req.addr];

                if (reqs.empty())
                    output->fatal(CALL_INFO, -1, "Ramulator2Backend: Error - ramulator2Done called but dramReqs[addr] is empty. Addr: %" PRIx64 "\n", (Addr)req.addr);

                ReqId memreq = reqs.front();
                reqs.pop_front();
                if(0 == reqs.size())
                    dramReqs.erase(req.addr);

                handleMemResponse(memreq);
        });
        if (enqueue_success) {
            if (dramReqs.find(addr) != dramReqs.end()) dramReqs[addr].push_back(reqId);
            else {
                std::deque<ReqId> reqs;
                reqs.push_back(reqId);
                dramReqs.insert(std::make_pair(addr,reqs));
            }
        }
    }
    output->debug(_L10_, "Ramulator2Backend: enqueue %s\n", enqueue_success ? "successful" : "unsuccessful");

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