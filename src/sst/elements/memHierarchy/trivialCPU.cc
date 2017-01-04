// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include "trivialCPU.h"

#include <assert.h>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/simpleMem.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Statistics;

trivialCPU::trivialCPU(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    requestsPendingCycle = new Histogram<uint64_t, uint64_t>("Requests Pending Per Cycle", 2);

    // Restart the RNG to ensure completely consistent results 
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", 0, 0, Output::STDOUT);

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
    
    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "TrivialCPU cannot issue less than one request at a time...fix your input deck\n");
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    memory = dynamic_cast<Interfaces::SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, params));
    if ( !memory ) {
        out.fatal(CALL_INFO, -1, "Unable to load Module as memory\n");
    }
    memory->initialize("mem_link",
			new Interfaces::SimpleMem::Handler<trivialCPU>(this, &trivialCPU::handleEvent) );

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<trivialCPU>(this, &trivialCPU::clockTic);
    clockTC = registerClock( clockFreq, clockHandler );
    
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
	if ( !phase ) {
		memory->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
	}
}

// incoming events are scanned and deleted
void trivialCPU::handleEvent(Interfaces::SimpleMem::Request *req)
{
    std::map<uint64_t, SimTime_t>::iterator i = requests.find(req->id);
    if ( requests.end() == i ) {
        out.fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", req->id);
    } else {
        SimTime_t et = getCurrentSimTime() - i->second;
        requests.erase(i);
        out.output("%s: Received Request with command %d (addr 0x%" PRIx64 ") [Time: %" PRIu64 "] [%zu outstanding requests]\n",
                getName().c_str(),
                req->cmd, req->addr, et,
                requests.size());
        num_reads_returned++;
    }

    delete req;
}


bool trivialCPU::clockTic( Cycle_t )
{
    ++clock_ticks;

    // Histogram bin the requests pending per cycle
    requestsPendingCycle->add((uint64_t) requests.size());

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

                Interfaces::SimpleMem::Addr addr = ((((Interfaces::SimpleMem::Addr) rng.generateNextUInt64()) % maxAddr)>>2) << 2;
                
                Interfaces::SimpleMem::Request::Command cmd = Interfaces::SimpleMem::Request::Read;

                uint32_t instNum = rng.generateNextUInt32() % 20;
                uint64_t size = 4;
                std::string cmdString = "Read";
                if (do_write && 0 == instNum || 1 == instNum) {
                    cmd = Interfaces::SimpleMem::Request::Write;
                    cmdString = "Write";
                } else if (do_flush && 2 == instNum) {
                    cmd = Interfaces::SimpleMem::Request::FlushLine;
                    size = lineSize;
                    addr = addr - (addr % lineSize);
                    cmdString = "FlushLine";
                } else if (do_flush && 3 == instNum) {
                    cmd = Interfaces::SimpleMem::Request::FlushLineInv;
                    size = lineSize;
                    addr = addr - (addr % lineSize);
                    cmdString = "FlushLineInv";
                }

                Interfaces::SimpleMem::Request *req = new Interfaces::SimpleMem::Request(cmd, addr, 4 /*4 bytes*/);
		if ( cmd == Interfaces::SimpleMem::Request::Write ) {
		    req->data.resize(4);
                    req->data[0] = (addr >> 24) & 0xff;
                    req->data[1] = (addr >> 16) & 0xff;
                    req->data[2] = (addr >>  8) & 0xff;
                    req->data[3] = (addr >>  0) & 0xff;
	        }
            
                bool noncacheable = ( addr >= noncacheableRangeStart && addr < noncacheableRangeEnd );
                if ( noncacheable ) {
                    req->flags |= Interfaces::SimpleMem::Request::F_NONCACHEABLE;
                    if ( cmd == Interfaces::SimpleMem::Request::Write ) { ++noncacheableWrites; } 
                    else if (cmd == Interfaces::SimpleMem::Request::Read ) { ++noncacheableReads; }
                }

		memory->sendRequest(req);
		requests[req->id] =  getCurrentSimTime();

		out.output("%s: %d Issued %s%s for address 0x%" PRIx64 "\n",
                        getName().c_str(), numLS, noncacheable ? "Noncacheable " : "" , cmdString.c_str(), addr);
		num_reads_issued++;

                numLS--;
	    }
        }
    }

    // Check whether to end the simulation
    if ( 0 == numLS && requests.empty() ) {
        out.output("TrivialCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}


