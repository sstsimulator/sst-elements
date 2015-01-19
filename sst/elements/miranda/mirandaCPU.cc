// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/simulation.h>
#include <sst/core/unitAlgebra.h>

#include "mirandaGenerator.h"
#include "mirandaCPU.h"

using namespace SST::Miranda;

RequestGenCPU::RequestGenCPU(SST::ComponentId_t id, SST::Params& params) :
	Component(id) {

	const int verbose = (int) params.find_integer("verbose", 0);
	out = new Output("RequestGenCPU[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

	maxRequestsPending = (uint32_t) params.find_integer("maxmemreqpending", 16);
	requestsPending = 0;

	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum requests to be memory to be outstanding.\n",
		maxRequestsPending);

	std::string interfaceName = params.find_string("memoryinterface", "memHierarchy.memInterface");
	out->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

	Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
	cache_link = dynamic_cast<SimpleMem*>( loadModuleWithComponent(interfaceName,
		this, interfaceParams) );

	if(NULL == cache_link) {
		out->fatal(CALL_INFO, -1, "Error loading memory interface module.\n");
	} else {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory interface successfully.\n");
	}

	out->verbose(CALL_INFO, 1, 0, "Initializing memory interface...\n");

	bool init_success = cache_link->initialize("cache_link", new SimpleMem::Handler<RequestGenCPU>(this, &RequestGenCPU::handleEvent) );

	if(init_success) {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory initialize routine returned successfully.\n");
	} else {
		out->fatal(CALL_INFO, -1, "Failed to initialize interface: %s\n", interfaceName.c_str());
	}

	if(NULL == cache_link) {
		out->fatal(CALL_INFO, -1, "Failed to load interface: %s\n", interfaceName.c_str());
	} else {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory interface successfully.\n");
	}

	std::string reqGenModName = params.find_string("generator", "");
	out->verbose(CALL_INFO, 1, 0, "Request generator to be loaded is: %s\n", reqGenModName.c_str());
	Params genParams = params.find_prefix_params("generatorParams.");
	reqGen = dynamic_cast<RequestGenerator*>( loadModuleWithComponent(reqGenModName, this, genParams) );

	if(NULL == reqGen) {
		out->fatal(CALL_INFO, -1, "Failed to load generator: %s\n", reqGenModName.c_str());
	} else {
		out->verbose(CALL_INFO, 1, 0, "Generator loaded successfully.\n");
	}

	std::string cpuClock = params.find_string("clock", "2GHz");
	registerClock(cpuClock, new Clock::Handler<RequestGenCPU>(this, &RequestGenCPU::clockTick));

	out->verbose(CALL_INFO, 1, 0, "CPU clock configured for %s\n", cpuClock.c_str());
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	cacheLine = (uint64_t) params.find_integer("cache_line_size", 64);

	statReadReqs   		= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "read_reqs") );
	statWriteReqs  		= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "write_reqs") );
	statSplitReadReqs 	= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "split_read_reqs") );
	statSplitWriteReqs 	= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "split_write_reqs") );
	statCyclesWithIssue 	= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "cycles_with_issue") );
	statCyclesWithoutIssue 	= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "cycles_no_issue") );
	statBytesRead 		= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "total_bytes_read") );
	statBytesWritten 	= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "total_bytes_write") );
	statReqLatency 		= registerStatistic( new AccumulatorStatistic<uint64_t>(this, "total_req_latency") );

	reqMaxPerCycle = 2;
	out->verbose(CALL_INFO, 1, 0, "Configuration completed.\n");
}

RequestGenCPU::~RequestGenCPU() {
	delete cache_link;
	delete reqGen;
	delete out;

	delete statReadReqs;
	delete statWriteReqs;
	delete statSplitReadReqs;
	delete statSplitWriteReqs;
	delete statCyclesWithIssue;
	delete statCyclesWithoutIssue;
	delete statBytesRead;
	delete statBytesWritten;
	delete statReqLatency;
}

void RequestGenCPU::finish() {
	// Tell the generator we are completed.
	reqGen->completed();
}

void RequestGenCPU::init(unsigned int phase) {

}

void RequestGenCPU::handleEvent( Interfaces::SimpleMem::Request* ev) {
	out->verbose(CALL_INFO, 2, 0, "Recv event for processing from interface\n");

	SimpleMem::Request::id_t reqID = ev->id;
	std::map<SimpleMem::Request::id_t, uint64_t>::iterator reqFind = requestsInFlight.find(reqID);

	if(reqFind == requestsInFlight.end()) {
		out->fatal(CALL_INFO, -1, "Unable to find request %" PRIu64 " in request map.\n", reqID);
	} else{
		statReqLatency->addData((getCurrentSimTimeNano() - reqFind->second));
		requestsInFlight.erase(reqID);

		// Decrement pending requests, we have recv'd a response
		requestsPending--;

		delete ev;
	}
}

void RequestGenCPU::issueRequest(RequestGeneratorRequest* req) {
	const uint64_t reqAddress = req->getAddress();
	const uint64_t reqLength  = req->getLength();
	bool isRead               = req->isRead();
	const uint64_t lineOffset = reqAddress % cacheLine;

	out->verbose(CALL_INFO, 4, 0, "Issue request: address=%" PRIu64 ", length=%" PRIu64 ", operation=%s, cache line offset=%" PRIu64 "\n",
		reqAddress, reqLength, (isRead ? "READ" : "WRITE"), lineOffset);

	if(isRead) {
		statBytesRead->addData(reqLength);
	} else {
		statBytesWritten->addData(reqLength);
	}

	if(lineOffset + reqLength > cacheLine) {
		// Request is for a split operation (i.e. split over cache lines)
		const uint64_t lowerLength = cacheLine - lineOffset;
		const uint64_t upperLength = reqLength - lowerLength;

		// Ensure that lengths are calculated correctly.
		assert(lowerLength + upperLength == reqLength);

		const uint64_t lowerAddress = reqAddress;
		const uint64_t upperAddress = (lowerAddress - lowerAddress % cacheLine) +
						cacheLine;

		out->verbose(CALL_INFO, 4, 0, "Issuing a split cache line operation:\n");
		out->verbose(CALL_INFO, 4, 0, "L -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
			lowerAddress, lowerLength);
		out->verbose(CALL_INFO, 4, 0, "U -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
			upperAddress, upperLength);

		SimpleMem::Request* reqLower = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			lowerAddress, lowerLength);

		SimpleMem::Request* reqUpper = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			upperAddress, upperLength);

		requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(reqLower->id, getCurrentSimTimeNano()) );
		requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(reqUpper->id, getCurrentSimTimeNano()) );

		out->verbose(CALL_INFO, 4, 0, "Issuing requesting into cache link...\n");
		cache_link->sendRequest(reqLower);
		cache_link->sendRequest(reqUpper);
		out->verbose(CALL_INFO, 4, 0, "Completed issue.\n");

		requestsPending += 2;

		// Keep track of split requests
		if(isRead) {
			statSplitReadReqs->addData(1);
		} else {
			statSplitWriteReqs->addData(1);
		}
	} else {
		// This is not a split laod, i.e. issue in a single transaction
		SimpleMem::Request* request = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			reqAddress, reqLength);

		requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(request->id, getCurrentSimTimeNano()) );
		cache_link->sendRequest(request);

		requestsPending++;

		if(isRead) {
			statReadReqs->addData(1);
		} else {
			statWriteReqs->addData(1);
		}
	}

	// Mark request as issued
	req->markIssued();
}

bool RequestGenCPU::clockTick(SST::Cycle_t cycle) {
	if(reqGen->isFinished()) {
		if( (pendingRequests.size() == 0) && (0 == requestsPending) ) {
			out->verbose(CALL_INFO, 4, 0, "Request generator complete and no requests pending, simulation can halt.\n");

			// Deregister here
			primaryComponentOKToEndSim();
			return true;
		}
	}

	// Process the request which may require splitting into multiple
	// requests (if breaks over a cache line)
	out->verbose(CALL_INFO, 2, 0, "Requests pending %" PRIu32 ", maximum permitted %" PRIu32 ".\n",
		requestsPending, maxRequestsPending);

	if(requestsPending < maxRequestsPending) {
		statCyclesWithIssue->addData(1);

		for(uint32_t reqThisCycle = 0; reqThisCycle < reqMaxPerCycle; ++reqThisCycle) {
			if(requestsPending < maxRequestsPending) {
				out->verbose(CALL_INFO, 2, 0, "Will attempt to issue as free slots in load/store unit.\n");

				if(pendingRequests.size() == 0) {
					if(! reqGen->isFinished()) {
						reqGen->generate(&pendingRequests);
					}

					if(pendingRequests.size() == 0) {
						break;
					}
				} else {
					out->verbose(CALL_INFO, 4, 0, "Issuing request cycle %" PRIu64 ", pending requests: %" PRIu64 ".\n",
						(uint64_t) cycle, (uint64_t) pendingRequests.size());

					RequestGeneratorRequest* nxtRq = pendingRequests.front();
					pendingRequests.pop();

					issueRequest(nxtRq);

					delete nxtRq;
				}
			} else {
				out->verbose(CALL_INFO, 2, 0, "Unable to issue as all resources are now full, continue next cycle.\n");
				break;
			}
		}
	} else {
		out->verbose(CALL_INFO, 4, 0, "Will not issue, not free slots in load/store unit.\n");
		statCyclesWithoutIssue->addData(1);
	}

	return false;
}
