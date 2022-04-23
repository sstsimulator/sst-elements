// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
#include "testcpu/trivialCPU.h"

#include <sst/core/params.h>
#include <sst/core/interfaces/stringEvent.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Statistics;

trivialCPU::trivialCPU(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    requestsPendingCycle = registerStatistic<uint64_t>("pendCycle");

    // Restart the RNG to ensure completely consistent results
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);

    commFreq = params.find<int>("commFreq", -1);
    if (commFreq < 0) {
        out.fatal(CALL_INFO, -1,"couldn't find communication frequency\n");
    }

    maxAddr = params.find<uint64_t>("memSize", -1) -1;

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    if ( !maxAddr ) {
        out.fatal(CALL_INFO, -1, "Must set memSize\n");
    }

    lineSize = params.find<uint64_t>("lineSize", 64);

    do_flush = params.find<bool>("do_flush", 0);

    do_write = params.find<bool>("do_write", 1);

    numLS = params.find<int>("num_loadstore", -1);

    noncacheableRangeStart = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheableRangeEnd = params.find<uint64_t>("noncacheableRangeEnd", 0);
    noncacheableSize = noncacheableRangeEnd - noncacheableRangeStart;

    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "TrivialCPU cannot issue less than one request at a time...fix your input deck\n");
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<trivialCPU>(this, &trivialCPU::clockTic);
    clockTC = registerClock( clockFreq, clockHandler );


    memory = loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new Interfaces::StandardMem::Handler<trivialCPU>(this, &trivialCPU::handleEvent));

    if (!memory) {
        Params interfaceParams;
        interfaceParams.insert("port", "mem_link");
        memory = loadAnonymousSubComponent<Interfaces::StandardMem>("memHierarchy.standardInterface", "memory", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
                interfaceParams, clockTC, new Interfaces::StandardMem::Handler<trivialCPU>(this, &trivialCPU::handleEvent));
        //out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent\n");
    }

    clock_ticks = 0;
    num_reads_issued = num_reads_returned = 0;
    noncacheableReads = noncacheableWrites = 0;
}

trivialCPU::trivialCPU() :
	Component(-1)
{
	// for serialization only
}


void trivialCPU::init(unsigned int phase)
{
    memory->init(phase);
}

// incoming events are scanned and deleted
void trivialCPU::handleEvent(Interfaces::StandardMem::Request *req)
{
    std::map<uint64_t, SimTime_t>::iterator i = requests.find(req->getID());
    if ( requests.end() == i ) {
        out.fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", req->getID());
    } else {
        SimTime_t et = getCurrentSimTime() - i->second;
        requests.erase(i);
        out.verbose(CALL_INFO, 2, 0, "%s: Received Request: %s [Time: %" PRIu64 "] [%zu outstanding requests]\n",
                    getName().c_str(), req->getString().c_str(), et, requests.size());
        num_reads_returned++;
    }

    delete req;
}


bool trivialCPU::clockTic( Cycle_t )
{
    ++clock_ticks;

    // Histogram bin the requests pending per cycle
    requestsPendingCycle->addData((uint64_t) requests.size());

    // communicate?
    if ((0 != numLS) && (0 == (rng.generateNextUInt32() % commFreq))) {
        if ( requests.size() < maxOutstanding ) {
            // yes, communicate
            // create event
            // x4 to prevent splitting blocks
            uint32_t reqsToSend = 1;
            if (maxReqsPerIssue > 1) reqsToSend += rng.generateNextUInt32() % maxReqsPerIssue;
            if (reqsToSend > (maxOutstanding - requests.size())) reqsToSend = maxOutstanding - requests.size();
            if (reqsToSend > numLS) reqsToSend = numLS;
            	    

            for (int i = 0; i < reqsToSend; i++) {

                Interfaces::StandardMem::Addr addr = rng.generateNextUInt64();

                uint32_t instNum = rng.generateNextUInt32() % 20;
                uint64_t size = 4;
                std::string cmdString = "Read";
                Interfaces::StandardMem::Request* req;

                if (do_write && instNum < 2) {
                    cmdString = "Write";
                    addr = ((addr % maxAddr)>>2) << 2;
                    std::vector<uint8_t> data = { 
                        static_cast<uint8_t>((addr >> 24) & 0xff), 
                        static_cast<uint8_t>((addr >> 16) & 0xff), 
                        static_cast<uint8_t>((addr >> 8) & 0xff), 
                        static_cast<uint8_t>(addr & 0xff)
                    };
                    req = new Interfaces::StandardMem::Write(addr, 4 /* 4 bytes */, data);
                } else if (do_flush && 2 == instNum) {
                    size = lineSize;
                    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
                    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
                        addr += noncacheableRangeEnd;
                    addr = addr - (addr % lineSize);
                    req = new Interfaces::StandardMem::FlushAddr(addr, size, false, 10);
                    cmdString = "FlushLine";
                } else if (do_flush && 3 == instNum) {
                    size = lineSize;
                    addr = ((addr % (maxAddr - noncacheableRangeEnd)>>2) << 2) + noncacheableRangeEnd;
                    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
                    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
                        addr += noncacheableRangeEnd;
                    addr = addr - (addr % lineSize);
                    req = new Interfaces::StandardMem::FlushAddr(addr, size, true, 10);
                    cmdString = "FlushLineInv";
                } else {
                    addr = ((addr % maxAddr)>>2) << 2;
                    req = new Interfaces::StandardMem::Read(addr, 4);
                    cmdString = "Read";
                }

                bool noncacheable = ( addr >= noncacheableRangeStart && addr < noncacheableRangeEnd );
                if ( noncacheable ) {
                    req->setNoncacheable();
                    if ( cmdString == "Write" ) { ++noncacheableWrites; }
                    else if (cmdString == "Read") { ++noncacheableReads; }
                }

		memory->send(req);
		requests[req->getID()] =  getCurrentSimTime();

		out.verbose(CALL_INFO, 2, 0, "%s: %d Issued %s%s for address 0x%" PRIx64 "\n",
                            getName().c_str(), numLS, noncacheable ? "Noncacheable " : "" , cmdString.c_str(), addr);

                num_reads_issued++;

                numLS--;
	    }
        }
    }

    // Check whether to end the simulation
    if ( 0 == numLS && requests.empty() ) {
        out.verbose(CALL_INFO, 1, 0, "TrivialCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}


