// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include "testcpu/streamCPU.h"

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include "memEvent.h"


using namespace SST;
using namespace SST::MemHierarchy;


streamCPU::streamCPU(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    uint32_t outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("StreamCPU:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // get parameters
    commFreq = params.find<int>("commFreq", -1);
    if (commFreq < 0) {
	out.fatal(CALL_INFO, -1,"couldn't find communication frequency\n");
    }
    
    maxAddr = params.find<uint32_t>("memSize", -1) -1;
    if ( !maxAddr ) {
        out.fatal(CALL_INFO, -1, "Must set memSize\n");
    }

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    do_write = params.find<bool>("do_write", 1);

    numLS = params.find<int>("num_loadstore", -1);

    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "Cannot issue less than one request per cycle...fix your input deck\n");
    }

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    memory = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");
    }
    memory->initialize("mem_link", new Interfaces::SimpleMem::Handler<streamCPU> (this, &streamCPU::handleEvent));

    addrOffset = params.find<uint64_t>("addressoffset", 0);

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<streamCPU>(this, &streamCPU::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);
    num_reads_issued = num_reads_returned = 0;

    // Start the next address from the offset
    nextAddr = addrOffset;
}

streamCPU::streamCPU() :
	Component(-1)
{
	// for serialization only
}


void streamCPU::init(unsigned int phase)
{
    memory->init(phase);
}

// incoming events are scanned and deleted
void streamCPU::handleEvent(Interfaces::SimpleMem::Request * req)
{
	//out.output("recv\n");
    std::map<uint64_t, SimTime_t>::iterator i = requests.find(req->id);
    if (i == requests.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->id);
    } else {
        SimTime_t et = getCurrentSimTime() - i->second;
        requests.erase(i);

	out.verbose(CALL_INFO, 1, 0, "Received MemEvent (response to: %10" PRIu64 ", Addr=%15" PRIu64 ", Took: %7" PRIu64 "ns, %6zu pending requests).\n",
                req->id, req->addr, et, requests.size());
        num_reads_returned++;
    }
    delete req;
}

bool streamCPU::clockTic( Cycle_t )
{
    // communicate?
    if ((numLS != 0) && ((rng.generateNextUInt32() % commFreq) == 0) && requests.size() <= maxOutstanding) {
	// yes, communicate
	// create event
	// x8 to prevent splitting blocks
        uint32_t reqsToSend = 1;
        if (maxReqsPerIssue > 1) reqsToSend += rng.generateNextUInt32() % maxReqsPerIssue;
        if (reqsToSend > (maxOutstanding - requests.size())) reqsToSend = maxOutstanding - requests.size();
        if (reqsToSend > numLS) reqsToSend = numLS;

        for (int i = 0; i < reqsToSend; i++) {

    	    bool doWrite = do_write && (((rng.generateNextUInt32() % 10) == 0));
            Interfaces::SimpleMem::Request::Command cmd = doWrite ? Interfaces::SimpleMem::Request::Write : Interfaces::SimpleMem::Request::Read;

            Interfaces::SimpleMem::Request *req = new Interfaces::SimpleMem::Request(cmd, nextAddr, 4/* 4 bytes*/);
	    
	    if ( doWrite ) {
	        req->data.resize(4);
                req->data[0] = (nextAddr >> 24) & 0xff;
                req->data[1] = (nextAddr >> 16) & 0xff;
                req->data[2] = (nextAddr >>  8) & 0xff;
                req->data[3] = (nextAddr >>  0) & 0xff;
	    }
	    
            memory->sendRequest(req);
            requests.insert(std::make_pair(req->id, getCurrentSimTime()));

	    out.verbose(CALL_INFO, 1, 0, "Issued request %10d: %5s for address %20d.\n", numLS, (doWrite ? "write" : "read"), nextAddr);

	    num_reads_issued++;
            nextAddr = (nextAddr + 8);

            if (nextAddr > (maxAddr - 4)) {
		nextAddr = addrOffset;
	    }

	    numLS--;
	}
    }

    if ( numLS == 0 && requests.size() == 0 ) {
        primaryComponentOKToEndSim();
        return true;
    }

    // return false so we keep going
    return false;
}


