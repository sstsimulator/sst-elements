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
#include "balarMMIO.h"
#include "util.h"


using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
 
/* Example MMIO device */
BalarMMIO::BalarMMIO(ComponentId_t id, Params &params) : SST::Component(id) {

    // Output for warnings
    out.init("BalarMMIOComponent[@f:@l:@p] ", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Configurations for GPU
    cpu_core_count = (uint32_t) params.find<uint32_t>("cpu_cores", 1);
    gpu_core_count = (uint32_t) params.find<uint32_t>("gpu_cores", 1);
    latency = (uint32_t) params.find<uint32_t>("latency", 1);
    maxPendingTransCore = (uint32_t) params.find<uint32_t>("maxtranscore", 16);
    maxPendingCacheTrans = (uint32_t) params.find<uint32_t>("maxcachetrans", 512);

    // Ensure that GPGP-sim has the same as SST gpu cores
    SST_gpgpusim_numcores_equal_check(gpu_core_count);
    pending_transactions_count = 0;
    remainingTransfer = 0;
    totalTransfer = 0;
    ackTransfer = 0;
    transferNumber = 0;

    bool found = false;

    // Initialize registers
    squared = 0;

    // Memory address
    mmio_addr = params.find<uint64_t>("base_addr", 0);

    std::string clockfreq = params.find<std::string>("clock", "1GHz");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: clock. Must have units of Hz or s and be > 0. "
                "(SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }
    TimeConverter* tc = getTimeConverter(clockfreq);

    // Bind tick function
    registerClock(tc, new Clock::Handler<BalarMMIO>(this, &BalarMMIO::clockTic));

    // Link names
    char* link_buffer = (char*) malloc(sizeof(char) * 256);
    char* link_buffer_mem = (char*) malloc(sizeof(char) * 256);
    char* link_cache_buffer = (char*) malloc(sizeof(char) * 256);

    // Buffer to keep track of pending transactions
    pendingTransactions = new std::unordered_map<StandardMem::Request::id_t, StandardMem::Request*>();
    gpuCachePendingTransactions = new std::unordered_map<StandardMem::Request::id_t, cache_req_params>();

    // Interface to CPU
    iface = loadUserSubComponent<SST::Interfaces::StandardMem>("iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleEvent));
    
    if (!iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    iface->setMemoryMappedAddressRegion(mmio_addr, 1);

    // TODO GPU Cache interface?

    handlers = new mmioHandlers(this, &out);
}

void BalarMMIO::init(unsigned int phase) {
    iface->init(phase);
}

void BalarMMIO::setup() {
    iface->setup();
}

bool BalarMMIO::clockTic(Cycle_t cycle) {
    bool done = SST_gpu_core_cycle();
    return done;
}

void BalarMMIO::handleEvent(StandardMem::Request* req) {
    // incoming CPU request, handle using mmioHandlers
    req->handle(handlers);
}

/* Handler for incoming Write requests, passing the function arguments */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Write* write) {

    // Convert 8 bytes of the payload into an int
    std::vector<uint8_t> buff = write->data;
    balarCudaCallPacket_t *pack_ptr = decode_balar_packet(&buff);
    // TODO Write converter for all function calls?
    mmio->cuda_ret = (cudaError_t) pack_ptr->cuda_call_id; // For testing purpose
    out->verbose(_INFO_, "Handle Write. Enum is %s\n", gpu_api_to_string(pack_ptr->cuda_call_id)->c_str());

    /* Send response (ack) if needed */
    // TODO Use this to send cuda call ret val?
    if (!(write->posted)) {
        mmio->iface->send(write->makeResponse());
    }
    delete write;
}

/* Handler for incoming Read requests */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "Handle Read. Returning %d\n", mmio->cuda_ret);

    vector<uint8_t> payload;
    cudaError_t value = mmio->cuda_ret;
    intToData(value, &payload);

    payload.resize(read->size, 0); // A bit silly if size != sizeof(int) but that's the CPU's problem

    // Make a response. Must fill in payload.
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());
    resp->data = payload;
    mmio->iface->send(resp);
    delete read;
}

/* Handler for incoming Read responses - should be a response to a Read we issued */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::ReadResp* resp) {
    // TODO Do nothing rn
    // std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    // if ( mmio->requests.end() == i ) {
    //     out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    // } else {
    //     SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
    //     mmio->statReadLatency->addData(et);
    //     mmio->requests.erase(i);
    // }
    // delete resp;
    // if (mmio->mem_access == 0 && mmio->requests.empty())
    //     mmio->primaryComponentOKToEndSim();
}

/* Handler for incoming Write responses - should be a response to a Write we issued */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::WriteResp* resp) {
    // TODO Do nothing rn
    // std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    // if (mmio->requests.end() == i) {
    //     out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    // } else {
    //     SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
    //     mmio->statWriteLatency->addData(et);
    //     mmio->requests.erase(i);
    // }
    // delete resp;
    // if (mmio->mem_access == 0 && mmio->requests.empty())
    //     mmio->primaryComponentOKToEndSim();
}

void BalarMMIO::mmioHandlers::intToData(int32_t num, vector<uint8_t>* data) {
    data->clear();
    for (int i = 0; i < sizeof(int); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

int32_t BalarMMIO::mmioHandlers::dataToInt(vector<uint8_t>* data) {
    int32_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

uint64_t BalarMMIO::mmioHandlers::dataToUInt64(vector<uint8_t>* data) {
    uint64_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void BalarMMIO::printStatus(Output &statusOut) {
    statusOut.output("Balar::BalarMMIO %s\n", getName().c_str());
    statusOut.output("    Register: %d\n", squared);
    iface->printStatus(statusOut);
    statusOut.output("End Balar::BalarMMIO\n\n");
}
