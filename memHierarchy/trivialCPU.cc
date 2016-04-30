// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
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

    // Restart the RNG to ensure completely consistent results (XML->Python causes
    // changes in the ComponentId_t ordering which fails to pass tests correctly.
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", 0, 0, Output::STDOUT);

    commFreq = params.find<int>("commFreq", -1);
    if (commFreq < 0) {
        out.fatal(CALL_INFO, -1,"couldn't find communication frequency\n");
    }
    
    maxAddr = params.find<uint64_t>("memSize", -1) -1;
    
    if ( !maxAddr ) {
        out.fatal(CALL_INFO, -1, "Must set memSize\n");
    }

    do_write = params.find<bool>("do_write", 1);

    numLS = params.find<int>("num_loadstore", -1);

    noncacheableRangeStart = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheableRangeEnd = params.find<uint64_t>("noncacheableRangeEnd", 0);


    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    memory = dynamic_cast<Interfaces::SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, params));
    if ( !memory ) {
        out.fatal(CALL_INFO, -1, "Unable to load Module as memory\n");
    }
    memory->initialize("mem_link",
			new Interfaces::SimpleMem::Handler<trivialCPU>(this, &trivialCPU::handleEvent) );

	registerTimeBase("1 ns", true);
	//set our clock
    clockHandler = new Clock::Handler<trivialCPU>(this, &trivialCPU::clockTic);
	clockTC = registerClock( "1GHz", clockHandler );
	num_reads_issued = num_reads_returned = 0;
    clock_ticks = 0;
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
        out.output("%s: Received Request with command %d (response to %" PRIu64 ", addr 0x%" PRIx64 ") [Time: %" PRIu64 "] [%zu outstanding requests]\n",
                getName().c_str(),
                req->cmd, req->id, req->addr, et,
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
		if ( requests.size() > 10 ) {
			out.output("%s: Not issuing read.  Too many outstanding requests.\n",
					getName().c_str());
		} else {

			// yes, communicate
			// create event
			// x4 to prevent splitting blocks
            Interfaces::SimpleMem::Addr addr = ((((Interfaces::SimpleMem::Addr) rng.generateNextUInt64()) % maxAddr)>>2) << 2;

			bool doWrite = do_write && ((0 == (rng.generateNextUInt32() % 10)));

            Interfaces::SimpleMem::Request *req = new Interfaces::SimpleMem::Request((doWrite ? Interfaces::SimpleMem::Request::Write : Interfaces::SimpleMem::Request::Read), addr, 4 /*4 bytes*/);
			if ( doWrite ) {
				req->data.resize(4);
                req->data[0] = (addr >> 24) & 0xff;
                req->data[1] = (addr >> 16) & 0xff;
                req->data[2] = (addr >>  8) & 0xff;
                req->data[3] = (addr >>  0) & 0xff;
			}
            
            bool noncacheable = ( addr >= noncacheableRangeStart && addr < noncacheableRangeEnd );
            if ( noncacheable ) {
                req->flags |= Interfaces::SimpleMem::Request::F_NONCACHEABLE;
                if ( doWrite ) { ++noncacheableWrites; } else { ++noncacheableReads; }
            }

			memory->sendRequest(req);
			requests[req->id] =  getCurrentSimTime();

			out.output("%s: %d Issued %s%s (%" PRIu64 ") for address 0x%" PRIx64 "\n",
					getName().c_str(), numLS, noncacheable ? "Noncacheable " : "" , doWrite ? "Write" : "Read", req->id, addr);
			num_reads_issued++;

            numLS--;
		}

	}

    if ( 0 == numLS && requests.empty() ) {
        out.output("TrivialCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;
    }

	// return false so we keep going
	return false;
}


