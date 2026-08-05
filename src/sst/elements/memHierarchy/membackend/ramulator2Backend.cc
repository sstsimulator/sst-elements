// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

    int32_t admission_queue_size = params.find<int32_t>("admission_queue_size", 0);
    admission_queue_size_ = static_cast<int64_t>(admission_queue_size);
    if (admission_queue_size_ < 0) admission_queue_size_ = -1;
    admission_issue_budget_per_cycle_ = params.find<int32_t>("admission_issue_budget_per_cycle", -1);
    if (admission_issue_budget_per_cycle_ == 0) admission_issue_budget_per_cycle_ = -1;
    admission_queue_enable_ = (admission_queue_size_ != 0);

    output->output(CALL_INFO, "Instantiated Ramulator2 from config file %s\n", config_path);
    output->verbose(CALL_INFO, 1, 0,
        "[ramu2] admission_queue enable=%d size=%" PRId64 " issue_budget_per_cycle=%" PRId32 "\n",
        admission_queue_enable_ ? 1 : 0, admission_queue_size_, admission_issue_budget_per_cycle_);
}

bool ramulator2Memory::issueToRamulator_(const PendingReq& req){
    output->verbose(CALL_INFO, 1, 0,
        "[ramu2] issueRequest id=%" PRIu64 " addr=0x%" PRIx64 " isWrite=%d size=%u\n",
        (uint64_t)req.reqId, (uint64_t)req.addr, (int)req.isWrite, req.numBytes);

    bool enqueue_success = false;

    if (req.isWrite) {
        enqueue_success = ramulator2_frontend->receive_external_requests(1, req.addr, 0,
            [this](Ramulator::Request& req) {});
        if (enqueue_success) {
            writes.insert(req.reqId);
        }
    } else {
        enqueue_success = ramulator2_frontend->receive_external_requests(0, req.addr, 0,
            [this](Ramulator::Request& req) {
                output->verbose(CALL_INFO, 1, 0,
                    "[ramu2] Read callback addr=0x%" PRIx64 " outstanding=%zu\n",
                    (uint64_t)req.addr, dramReqs.count(req.addr) ? dramReqs.at(req.addr).size() : 0);
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
            if (dramReqs.find(req.addr) != dramReqs.end()) dramReqs[req.addr].push_back(req.reqId);
            else {
                std::deque<ReqId> reqs;
                reqs.push_back(req.reqId);
                dramReqs.insert(std::make_pair(req.addr,reqs));
            }
        }
    }
    output->verbose(CALL_INFO, 1, 0,
        "[ramu2] backend enqueue %s (dramReqMap=%zu writes=%zu admissionQ=%zu)\n",
        enqueue_success ? "successful" : "unsuccessful", dramReqs.size(), writes.size(), admission_queue_.size());

    return enqueue_success;
}

bool ramulator2Memory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes){
    PendingReq req{reqId, addr, isWrite, numBytes};
    if (!admission_queue_enable_) {
        return issueToRamulator_(req);
    }

    if (admission_queue_size_ >= 0 && admission_queue_.size() >= static_cast<size_t>(admission_queue_size_)) {
        output->verbose(CALL_INFO, 1, 0,
            "[ramu2] admission queue full, reject id=%" PRIu64 " size=%zu cap=%" PRId64 "\n",
            (uint64_t)reqId, admission_queue_.size(), admission_queue_size_);
        return false;
    }

    admission_queue_.push_back(req);
    output->verbose(CALL_INFO, 2, 0,
        "[ramu2] admission enqueue ok id=%" PRIu64 " qsize=%zu\n",
        (uint64_t)reqId, admission_queue_.size());
    return true;
}

bool ramulator2Memory::clock(Cycle_t cycle){
    output->verbose(CALL_INFO, 2, 0,
        "[ramu2] clock cycle=%" PRIu64 " pending_reads=%zu pending_writes=%zu admission_q=%zu\n",
        (uint64_t)cycle, dramReqs.size(), writes.size(), admission_queue_.size());

    int issued_this_cycle = 0;
    while (admission_queue_enable_ && !admission_queue_.empty()) {
        if (admission_issue_budget_per_cycle_ > 0 &&
            issued_this_cycle >= admission_issue_budget_per_cycle_) {
            break;
        }
        const PendingReq& req = admission_queue_.front();
        if (!issueToRamulator_(req)) {
            break;
        }
        admission_queue_.pop_front();
        issued_this_cycle++;
    }

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
