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
#include "testcpu/standardMMIO.h"
#include "util.h"


using namespace SST;
using namespace SST::Experimental::Interfaces;
using namespace SST::MemHierarchy;
 
/* Example MMIO device */
StandardMMIO::StandardMMIO(ComponentId_t id, Params &params) : SST::Component(id) {

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

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

    iface = loadUserSubComponent<SST::Experimental::Interfaces::StandardMem>("iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<StandardMMIO>(this, &StandardMMIO::handleEvent));
    
    if (!iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    iface->setMemoryMappedAddressRegion(mmio_addr, 1);

    handlers = new mmioHandlers(this, &out);

    // Configure access types
    // Default is just accepting reads/writes
    // But this component can also issue memory accesses
    mem_access = params.find<uint32_t>("mem_accesses", false);
    if (mem_access > 0) {
        // Need an RNG so we can generate memory addresses
        rng = *(new SST::RNG::MarsagliaRNG(21, 101));

        // Need a clock so that we can decide whether to issue requests on each clock
        registerClock(tc, new Clock::Handler<StandardMMIO>(this, &StandardMMIO::clockTic));

        // Don't end simulation until we've finished sending requests & receiving responses
        primaryComponentDoNotEndSim();

        // Figure out max address we can give a memory access
        bool found;
        max_addr = params.find<Addr>("max_addr", 0, found);
        if (!found) {
            out.fatal(CALL_INFO, 0-1, "%s, Error: Invalid param, 'max_addr' must be specified if mem_accesses > 0\n");
        }

        // Register related statistics
        statReadLatency = registerStatistic<uint64_t>("read_latency");
        statWriteLatency = registerStatistic<uint64_t>("write_latency");
    }
}

void StandardMMIO::init(unsigned int phase) {
    iface->init(phase);
}

void StandardMMIO::setup() {
    iface->setup();
}

bool StandardMMIO::clockTic(Cycle_t cycle) {
    // Decide whether to send a request
    if (mem_access > 0) {
        uint32_t req = rng.generateNextUInt32() % 10;
        if (req < 2) {
            // Issue read
            Addr addr = ((rng.generateNextUInt64() % max_addr)>>2) << 2;
            StandardMem::Request* req = new Experimental::Interfaces::StandardMem::Read(addr, 4);
            out.verbose(CALL_INFO, 2, 0, "%s: %d Issued Read for address 0x%" PRIx64 "\n", getName().c_str(), mem_access, addr);
        
            requests.insert(std::make_pair(req->getID(), std::make_pair(getCurrentSimTime(), "Read")));
            iface->send(req);
            mem_access--;

        } else if (req < 4) {
            // Issue write
            Addr addr = (rng.generateNextUInt64() % max_addr);
            addr = (addr / iface->getLineSize()) * iface->getLineSize();
            std::vector<uint8_t> payload;
            payload.resize(iface->getLineSize(), 0);
            StandardMem::Request* req = new Experimental::Interfaces::StandardMem::Write(addr, iface->getLineSize(), payload);
            out.verbose(CALL_INFO, 2, 0, "%s: %d Issued Write for address 0x%" PRIx64 "\n", getName().c_str(), mem_access, addr);
            mem_access--;
        }
    }
    if (mem_access == 0) { 
        return true; // Stop clock
    }
    return false; // Do not stop clock
}

void StandardMMIO::handleEvent(StandardMem::Request* req) {
    req->handle(handlers);
}

/* Handler for incoming Write requests */
void StandardMMIO::mmioHandlers::handle(SST::Experimental::Interfaces::StandardMem::Write* write) {

    // Convert 8 bytes of the payload into an int
    std::vector<uint8_t> buff = write->data;
    int32_t value = dataToInt(&buff);
    mmio->squared = value*value;
    out->verbose(_INFO_, "Handle Write. Squared is %d\n", mmio->squared);

    /* Send response (ack) if needed */
    if (!(write->posted)) {
        mmio->iface->send(write->makeResponse());
    }
    delete write;
}

/* Handler for incoming Read requests */
void StandardMMIO::mmioHandlers::handle(SST::Experimental::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "Handle Read. Returning %d\n", mmio->squared);

    vector<uint8_t> payload;
    int32_t value = mmio->squared;
    intToData(value, &payload);

    payload.resize(read->size, 0); // A bit silly if size != sizeof(int) but that's the CPU's problem

    // Make a response. Must fill in payload.
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());
    resp->data = payload;
    mmio->iface->send(resp);
    delete read;
}

/* Handler for incoming Read responses - should be a response to a Read we issued */
void StandardMMIO::mmioHandlers::handle(SST::Experimental::Interfaces::StandardMem::ReadResp* resp) {
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    if ( mmio->requests.end() == i ) {
        out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
        mmio->statReadLatency->addData(et);
        mmio->requests.erase(i);
    }
    delete resp;
    if (mmio->mem_access == 0 && mmio->requests.empty())
        mmio->primaryComponentOKToEndSim();
}

/* Handler for incoming Write responses - should be a response to a Write we issued */
void StandardMMIO::mmioHandlers::handle(SST::Experimental::Interfaces::StandardMem::WriteResp* resp) {
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    if (mmio->requests.end() == i) {
        out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
        mmio->statWriteLatency->addData(et);
        mmio->requests.erase(i);
    }
    delete resp;
    if (mmio->mem_access == 0 && mmio->requests.empty())
        mmio->primaryComponentOKToEndSim();
}

void StandardMMIO::mmioHandlers::intToData(int32_t num, vector<uint8_t>* data) {
    data->clear();
    for (int i = 0; i < sizeof(int); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

int32_t StandardMMIO::mmioHandlers::dataToInt(vector<uint8_t>* data) {
    int32_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void StandardMMIO::printStatus(Output &statusOut) {
    statusOut.output("MemHierarchy::StandardMMIO %s\n", getName().c_str());
    statusOut.output("    Register: %d\n", squared);
    iface->printStatus(statusOut);
    statusOut.output("End MemHierarchy::StandardMMIO\n\n");
}
