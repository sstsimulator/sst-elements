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
#include <sstream>
#include <sst/core/simulation.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/timeConverter.h>

#include "mirandaGenerator.h"
#include "mirandaCPU.h"
#include "mirandaEvent.h"

using namespace SST::Miranda;

RequestGenCPU::RequestGenCPU(SST::ComponentId_t id, SST::Params& params) :
	Component(id), srcLink(NULL), reqGen(NULL) {

	const int verbose = params.find<int>("verbose", 0);
	std::stringstream prefix;
	prefix << "@t:" <<getName() << ":RequestGenCPU[@p:@l]: ";
	out = new Output( prefix.str(), verbose, 0, SST::Output::STDOUT);
        
	maxRequestsPending[READ] = params.find<uint32_t>("maxloadmemreqpending", 16);
	maxRequestsPending[WRITE] = params.find<uint32_t>("maxstorememreqpending", 16);
	maxRequestsPending[CUSTOM] = params.find<uint32_t>("maxcustommemreqpending", 16);
	
        requestsPending[READ] = requestsPending[WRITE] = requestsPending[CUSTOM] = 0;

	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum Load requests to be memory to be outstanding.\n",
		maxRequestsPending[READ]);
	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum Store requests to be memory to be outstanding.\n",
		maxRequestsPending[WRITE]);
	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum Custom requests to be memory to be outstanding.\n",
		maxRequestsPending[CUSTOM]);

	std::string interfaceName = params.find<std::string>("memoryinterface", "memHierarchy.memInterface");
	out->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

	Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
	cache_link = dynamic_cast<Interfaces::SimpleMem*>( loadSubComponent(interfaceName,
		this, interfaceParams) );

	maxOpLookup = params.find<uint64_t>("max_reorder_lookups", 16);

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

	std::string cpuClock = params.find<std::string>("clock", "2GHz");
	clockHandler = new Clock::Handler<RequestGenCPU>(this, &RequestGenCPU::clockTick);
	timeConverter = registerClock(cpuClock, clockHandler );

	out->verbose(CALL_INFO, 1, 0, "CPU clock configured for %s\n", cpuClock.c_str());

	std::string reqGenModName = params.find<std::string>("generator", "");
	
	cacheLine = params.find<uint64_t>("cache_line_size", 64);
	const uint64_t pageSize  = params.find<uint64_t>("pagesize", 4096);
	const uint64_t pageCount = params.find<uint64_t>("pagecount", 4194304);
	
	if( pageSize % cacheLine > 0 ) {
		out->fatal(CALL_INFO, -8, "Error: Miranda memory configuration. Page size=%" PRIu64 ", is not a multiple of cache line size: %" PRIu64 "\n",
			pageSize, cacheLine);
	}
	
	MirandaPageMappingPolicy policy = MirandaPageMappingPolicy::LINEAR;
	
	std::string policyStr = params.find<std::string>("pagemap", "linear");
	
	if(policyStr == "RANDOMIZED" || policyStr == "randomized") {
		policy = MirandaPageMappingPolicy::RANDOMIZED;
	} else if (policyStr == "LINEAR" || policyStr == "linear" ) {
		policy = MirandaPageMappingPolicy::LINEAR;
	} else {
		out->fatal(CALL_INFO, -8, "Error: unknown page mapping type: \'%s\'\n", policyStr.c_str());
	}
	
	memMgr = new MirandaMemoryManager(out, pageSize, pageCount, policy);

    if ( NULL != (reqGen = dynamic_cast<RequestGenerator*>(loadNamedSubComponent("generator"))) ) {
        out->verbose(CALL_INFO, 1, 0, "Generator loaded successfully.\n");
	    registerAsPrimaryComponent();
	    primaryComponentDoNotEndSim();

    } else if ( ! reqGenModName.empty() ) {

		out->verbose(CALL_INFO, 1, 0, "Request generator to be loaded is: %s\n", reqGenModName.c_str());
		Params genParams = params.find_prefix_params("generatorParams.");
		reqGen = dynamic_cast<RequestGenerator*>( loadSubComponent(reqGenModName, this, genParams) );
		if(NULL == reqGen) {
			out->fatal(CALL_INFO, -1, "Failed to load generator: %s\n", reqGenModName.c_str());
		} else {
			out->verbose(CALL_INFO, 1, 0, "Generator loaded successfully.\n");
		}

	    registerAsPrimaryComponent();
	    primaryComponentDoNotEndSim();

	} else if ( isPortConnected("src") ) {

		out->verbose(CALL_INFO, 1, 0, "getting generators from a link\n");
		srcLink = configureLink( "src", "50ps", new Event::Handler<RequestGenCPU>(this, &RequestGenCPU::handleSrcEvent));
		if ( NULL == srcLink ) {
			out->fatal(CALL_INFO, -1, "Failed to configure src link\n");
		}

	} else {
		out->fatal(CALL_INFO, -1, "Failed to find a generator or src port\n");
	}


        statReqs[READ]            = registerStatistic<uint64_t>( "read_reqs" );
	statReqs[WRITE]		  = registerStatistic<uint64_t>( "write_reqs" );
        statReqs[CUSTOM]          = registerStatistic<uint64_t>( "custom_reqs" );
	statSplitReqs[READ] 	  = registerStatistic<uint64_t>( "split_read_reqs" );
	statSplitReqs[WRITE]      = registerStatistic<uint64_t>( "split_write_reqs" );
	statSplitReqs[CUSTOM]  	  = registerStatistic<uint64_t>( "split_custom_reqs" );
	statCyclesWithIssue 	  = registerStatistic<uint64_t>( "cycles_with_issue" );
	statCyclesWithoutIssue 	  = registerStatistic<uint64_t>( "cycles_no_issue" );
	statBytes[READ] 	  = registerStatistic<uint64_t>( "total_bytes_read" );
	statBytes[WRITE] 	  = registerStatistic<uint64_t>( "total_bytes_write" );
        statBytes[CUSTOM]         = registerStatistic<uint64_t>( "total_bytes_custom" );
	statReqLatency 		  = registerStatistic<uint64_t>( "req_latency" );
	statTime                  = registerStatistic<uint64_t>( "time" );
	statCyclesHitFence        = registerStatistic<uint64_t>( "cycles_hit_fence" );
	statMaxIssuePerCycle      = registerStatistic<uint64_t>( "cycles_max_issue" );
	statCyclesHitReorderLimit = registerStatistic<uint64_t>( "cycles_max_reorder" );
	statCycles                = registerStatistic<uint64_t>( "cycles" );

	reqMaxPerCycle = params.find<uint32_t>("max_reqs_cycle", 2);



	out->verbose(CALL_INFO, 1, 0, "Miranda CPU Configuration:\n");
	out->verbose(CALL_INFO, 1, 0, "- Max requests per cycle:         %" PRIu32 "\n", reqMaxPerCycle);
	out->verbose(CALL_INFO, 1, 0, "- Max reorder lookups             %" PRIu32 "\n", maxOpLookup);
	out->verbose(CALL_INFO, 1, 0, "- Clock:                          %s\n", cpuClock.c_str());
	out->verbose(CALL_INFO, 1, 0, "- Cache line size:                %" PRIu64 " bytes\n", cacheLine);
	out->verbose(CALL_INFO, 1, 0, "- Max Load requests pending:      %" PRIu32 "\n", maxRequestsPending[READ]);
	out->verbose(CALL_INFO, 1, 0, "- Max Store requests pending:     %" PRIu32 "\n", maxRequestsPending[WRITE]);
	out->verbose(CALL_INFO, 1, 0, "- Max Custom requests pending:     %" PRIu32 "\n", maxRequestsPending[CUSTOM]);
	out->verbose(CALL_INFO, 1, 0, "Configuration completed.\n");
}

RequestGenCPU::~RequestGenCPU() {
	delete cache_link;
	delete out;

//	delete statReadReqs;
//	delete statWriteReqs;
//	delete statSplitReadReqs;
//	delete statSplitWriteReqs;
//	delete statCyclesWithIssue;
//	delete statCyclesWithoutIssue;
//	delete statBytesRead;
//	delete statBytesWritten;
//	delete statReqLatency;
//	delete statCyclesHitFence;
//	delete statCyclesHitReorderLimit;
//	delete statMaxIssuePerCycle;
//	delete statCycles;
}

void RequestGenCPU::finish() {
}

void RequestGenCPU::init(unsigned int phase) {
    cache_link->init(phase);
}

void RequestGenCPU::handleSrcEvent( Event* ev ) {

	MirandaReqEvent* event = static_cast<MirandaReqEvent*>(ev); 

	out->verbose(CALL_INFO, 2, 0, "got %lu generators\n", event->generators.size() );
	loadGenerator( event );

	if ( 0 != timeConverter->convertFromCoreTime( Simulation::getSimulation()->getCurrentSimCycle()) ) {
		clockTick( reregisterClock( timeConverter, clockHandler ) );
	}	

	srcReqEvent = event;
}

void RequestGenCPU::loadGenerator( MirandaReqEvent* event ) {

	std::string& generator = event->generators.front().first;
	SST::Params& params = event->generators.front().second;
	loadGenerator( generator, params );
	event->generators.pop_front();
}

void RequestGenCPU::loadGenerator( const std::string& name, SST::Params& params) {

	out->verbose(CALL_INFO, 1, 0, "generator to be loaded is: %s\n", name.c_str());

	reqGen = dynamic_cast<RequestGenerator*>( loadSubComponent( name, this, params ) );

	if(NULL == reqGen) {
		out->fatal(CALL_INFO, -1, "Failed to load generator: %s\n", name.c_str());
	}
}


void RequestGenCPU::handleEvent( Interfaces::SimpleMem::Request* ev) {
	out->verbose(CALL_INFO, 2, 0, "Recv event for processing from interface\n");

	SimpleMem::Request::id_t reqID = ev->id;
	std::map<SimpleMem::Request::id_t, CPURequest*>::iterator reqFind = requestsInFlight.find(reqID);

	if(reqFind == requestsInFlight.end()) {
		out->fatal(CALL_INFO, -1, "Unable to find request %" PRIu64 " in request map.\n", reqID);
	} else{
		CPURequest* cpuReq = reqFind->second;

		out->verbose(CALL_INFO, 4, 0, "Miranda request located ID=%" PRIu64 ", contains %" PRIu32 " parts, issue time=%" PRIu64 ", time now=%" PRIu64 "\n",
			cpuReq->getOriginalReqID(), cpuReq->countParts(), cpuReq->getIssueTime(), getCurrentSimTimeNano());

		statReqLatency->addData((getCurrentSimTimeNano() - cpuReq->getIssueTime()));
		requestsInFlight.erase(reqFind);

		// Tell the CPU request one more of its parts are satisfied
		cpuReq->decPartCount();

		// Decrement pending requests, we have recv'd a response
			
		if ( ev->cmd == Interfaces::SimpleMem::Request::Command::ReadResp ) { 
			requestsPending[READ]--;
		} else if ( ev->cmd == Interfaces::SimpleMem::Request::Command::WriteResp ) { 
			requestsPending[WRITE]--;
		} else if( ev->cmd == Interfaces::SimpleMem::Request::Command::CustomCmd ){
                        requestsPending[CUSTOM]--;
                }

		// If all the parts of this request are now completed then we will mark it for
		// deletion and update any pending requests which are depending on us
		if(cpuReq->completed()) {
			out->verbose(CALL_INFO, 4, 0, "-> Entry has all parts satisfied, removing ID=%" PRIu64 ", total processing time: %" PRIu64 "ns\n",
				cpuReq->getOriginalReqID(), (getCurrentSimTimeNano() - cpuReq->getIssueTime()));

			// Notify every pending request that there may be a satisfied dependency
			for(uint32_t i = 0; i < pendingRequests.size(); ++i) {
				pendingRequests.at(i)->satisfyDependency(cpuReq->getOriginalReqID());
			}

			delete cpuReq;
		}

		delete ev;
	}
}

void RequestGenCPU::issueRequest(MemoryOpRequest* req) {
    const uint64_t reqAddress = req->getAddress();
    const uint64_t reqLength  = req->getLength();
    bool isRead               = req->isRead();
    bool isCustom             = req->isCustom();
    ReqOperation operation    = req->getOperation();
    const uint64_t lineOffset = reqAddress % cacheLine;

    if( !isCustom ){
        out->verbose(CALL_INFO, 4, 0, "Issue request: address=0x%" PRIx64 ", length=%" PRIu64 ", operation=%s, cache line offset=%" PRIu64 "\n",
                reqAddress, reqLength, (isRead ? "READ" : "WRITE"), lineOffset);
    }else{
        out->verbose(CALL_INFO, 4, 0, "Issue request: address=0x%" PRIx64 ", length=%" PRIu64 ", operation=%s, cache line offset=%" PRIu64 "\n",
                reqAddress, reqLength, "CUSTOM", lineOffset);
    }
        
    if (statBytes[operation] != nullptr)
        statBytes[operation]->addData(reqLength);

    if(lineOffset + reqLength > cacheLine) {
        // Request is for a split operation (i.e. split over cache lines)
    	const uint64_t lowerLength = cacheLine - lineOffset;
        const uint64_t upperLength = reqLength - lowerLength;

    	// Ensure that lengths are calculated correctly.
        assert(lowerLength + upperLength == reqLength);

    	const uint64_t lowerAddress = memMgr->mapAddress(reqAddress);
        const uint64_t upperAddress = memMgr->mapAddress((lowerAddress - lowerAddress % cacheLine) + cacheLine);

	out->verbose(CALL_INFO, 4, 0, "Issuing a split cache line operation:\n");
    	out->verbose(CALL_INFO, 4, 0, "L -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
                lowerAddress, lowerLength);
	out->verbose(CALL_INFO, 4, 0, "U -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
                upperAddress, upperLength);

        SimpleMem::Request *reqLower;
        SimpleMem::Request *reqUpper;
        
        if( isCustom ){
            CustomOpRequest * creq = static_cast<CustomOpRequest*>(req);
            // build a custom request event
            reqLower = new SimpleMem::Request(
                    SimpleMem::Request::CustomCmd,
                    lowerAddress, lowerLength, creq->getOpcode(),0,0);

	    reqUpper = new SimpleMem::Request(
                    SimpleMem::Request::CustomCmd,
                    upperAddress, upperLength, creq->getOpcode(),0,0);
        }else{
            // build a normal event
            reqLower = new SimpleMem::Request(
                    isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
                    lowerAddress, lowerLength);

	    reqUpper = new SimpleMem::Request(
                    isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
                    upperAddress, upperLength);
        }
        
        CPURequest* newCPUReq = new CPURequest(req->getRequestID());
    	newCPUReq->incPartCount();
        newCPUReq->incPartCount();
    	newCPUReq->setIssueTime(getCurrentSimTimeNano());

    	requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, CPURequest*>(reqLower->id, newCPUReq) );
        requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, CPURequest*>(reqUpper->id, newCPUReq) );

    	out->verbose(CALL_INFO, 4, 0, "Issuing requesting into cache link...\n");
        cache_link->sendRequest(reqLower);
    	cache_link->sendRequest(reqUpper);
        out->verbose(CALL_INFO, 4, 0, "Completed issue.\n");

        requestsPending[operation] += 2;

    	// Keep track of split requests
        if (statSplitReqs[operation] != nullptr)
            statSplitReqs[operation]->addData(1);
    
    } else {
        // This is not a split load, i.e. issue in a single transaction
        SimpleMem::Request *request;
        if( isCustom ){
            CustomOpRequest* creq = static_cast<CustomOpRequest*>(req);
            // issue custom request
            request = new SimpleMem::Request(
                    SimpleMem::Request::CustomCmd,
                    memMgr->mapAddress(reqAddress), reqLength,
                    creq->getOpcode(),0,0);
        }else{
            // issue standard request
	    request = new SimpleMem::Request(
                    isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
    		    memMgr->mapAddress(reqAddress), reqLength);
        }

        request->setVirtualAddress(memMgr->mapAddress(reqAddress));

        CPURequest* newCPUReq = new CPURequest(req->getRequestID());
        newCPUReq->incPartCount();
        newCPUReq->setIssueTime(getCurrentSimTimeNano());

        requestsInFlight.insert( std::pair<SimpleMem::Request::id_t, CPURequest*>(request->id, newCPUReq) );
        cache_link->sendRequest(request);

        requestsPending[operation]++;
                
        if (statReqs[operation] != nullptr)
            statReqs[operation]->addData(1);
    }
}

bool RequestGenCPU::clockTick(SST::Cycle_t cycle) {

    if ( ! reqGen ) {
        out->verbose(CALL_INFO, 2,0, "unregister\n");
        return true;
    }
    statCycles->addData(1);

    if (reqGen->isFinished()) {
        if ( (pendingRequests.size() == 0) &&
                (0 == requestsPending[READ]) &&
                (0 == requestsPending[WRITE]) &&
                (0 == requestsPending[CUSTOM]) ) {
            out->verbose(CALL_INFO, 4, 0, "Request generator complete and no requests pending, simulation can halt.\n");
            
            // Tell the statistics engine how long we have executed for
            statTime->addData(getCurrentSimTimeNano());
            
            reqGen->completed();
            delete reqGen;
            reqGen = NULL;
            
            if ( NULL == srcLink ) {
                primaryComponentOKToEndSim();
            } else {
                if ( srcReqEvent->generators.empty() ) {
                    MirandaRspEvent* event = new MirandaRspEvent;
                    event->key = static_cast<MirandaReqEvent*>(srcReqEvent)->key;
                    delete srcReqEvent;
                    srcLink->send(0,event);
                    
                    return true;
                } else {
                    loadGenerator( srcReqEvent );
                    return false;
                }
            
            }
	    
            // Deregister here
            return true;
        }
    }
    
    // Process the request which may require splitting into multiple
    // requests (if breaks over a cache line)
    out->verbose(CALL_INFO, 2, 0, "Load Requests pending %" PRIu32 ", maximum permitted %" PRIu32 ".\n",
            requestsPending[READ], maxRequestsPending[READ]);
    out->verbose(CALL_INFO, 2, 0, "Store Requests pending %" PRIu32 ", maximum permitted %" PRIu32 ".\n",
            requestsPending[WRITE], maxRequestsPending[WRITE]);
    out->verbose(CALL_INFO, 2, 0, "Custom Requests pending %" PRIu32 ", maximum permitted %" PRIu32 ".\n",
            requestsPending[CUSTOM], maxRequestsPending[CUSTOM]);

    bool issued = false;
    uint32_t reqsIssuedThisCycle = 0;
    std::vector<uint32_t> delReqs;
    
    // We need to generate at least as many requests as can be looked up in the OoO window
    // otherwise the issue will have starvation.
    for(int i = pendingRequests.size(); i < maxOpLookup; ++i) {
        if( reqGen->isFinished()) {
            break;
    	} else {
            reqGen->generate(&pendingRequests);
    	}
    }
    
    for(uint32_t i = 0; i < pendingRequests.size(); ++i) {
        if(reqsIssuedThisCycle == reqMaxPerCycle) {
            statMaxIssuePerCycle->addData(1);
            break;
    	}

	// Only a certain number of lookups are allowed, if we exceed this then we
        // must exit the issue loop
    	if(i == maxOpLookup) {
            out->verbose(CALL_INFO, 2, 0, "Hit maximum reorder limit this cycle, no further operations will issue.\n");
            statCyclesHitReorderLimit->addData(1);
            break;
    	}

        MemoryOpRequest* memOpReq;
	GeneratorRequest* nxtRq = pendingRequests.at(i);

	if(nxtRq->getOperation() == REQ_FENCE) {
            if(0 == requestsInFlight.size()) {
		out->verbose(CALL_INFO, 4, 0, "Fence operation completed, no pending requests, will be retired.\n");
                
                // Keep record we will delete fence at i
    		delReqs.push_back(i);
		
                // Delete the fence
    		delete nxtRq;
            } else {
                out->verbose(CALL_INFO, 4, 0, "Fence operation in flight (>0 pending requests), stall.\n");
            }
            
            statCyclesHitFence->addData(1);
            
            // Fence operations do now allow anything else to complete in this cycle
            break;

        } else if ( ( memOpReq = dynamic_cast<MemoryOpRequest*>(nxtRq) ) ) {

            if( requestsPending[memOpReq->getOperation()] < maxRequestsPending[memOpReq->getOperation()] ) {
                out->verbose(CALL_INFO, 4, 0, "Will attempt to issue as free slots in the load/store unit.\n");


		if(nxtRq->canIssue()) {
                    issued = true;
                    reqsIssuedThisCycle++;
                    
                    out->verbose(CALL_INFO, 4, 0, "Request %" PRIu64 " encountered, cleared to be issued, %" PRIu32 " issued this cycle.\n",
                            nxtRq->getRequestID(), reqsIssuedThisCycle);

    		    // Keep record we will delete at index i
                    delReqs.push_back(i);
                    
                    //MemoryOpRequest* memOpReq = dynamic_cast<MemoryOpRequest*>(nxtRq);
                    issueRequest(memOpReq);
                    
                    delete nxtRq;
		} else {
                    out->verbose(CALL_INFO, 4, 0, "Request %" PRIu64 " in queue, has dependencies which are not satisfied, wait.\n",
                            nxtRq->getRequestID());
                }
    	    } else {
                out->verbose(CALL_INFO, 4, 0, "All load/store/custom slots occupied, no more issues will be attempted.\n");
    	        break;
            }
	} else {
            out->fatal(CALL_INFO, -1, "Error, invalid operation \n");
        }
    }

    pendingRequests.erase(delReqs);

    if(issued) {
	statCyclesWithIssue->addData(1);
    } else {
	out->verbose(CALL_INFO, 4, 0, "Will not issue, not free slots in load/store unit.\n");
	statCyclesWithoutIssue->addData(1);
    }

    return false;
}
