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

/* 
 * File:   memController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memoryController.h"
#include "util.h"

#include <sstream>
#include <stdio.h>
#include <fcntl.h>

#include "memEvent.h"
#include "membackend/memBackend.h"
#include "bus.h"
#include "cacheListener.h"
#include "memNIC.h"

#define NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;



/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id) {
    int debugLevel = params.find_integer("debug_level", 0);
    
    dbg.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg.debug(_L10_,"---");
    
    
    int stats               = params.find_integer("statistics", 0);
    if (stats != 0) {
        Output out("", 0, 0, Output::STDOUT);
        out.output("%s, **WARNING** The 'statistics' parameter is deprecated: memHierarchy statistics have been moved to the Statistics API. Please see sstinfo to view available statistics and update your configuration accordingly.\nNO statistics will be printed otherwise!\n", getName().c_str());
    }

    
    rangeStart_             = (Addr)params.find_integer("range_start", 0);
    interleaveSize_         = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize_         *= 1024;
    interleaveStep_         = (Addr)params.find_integer("interleave_step", 0);
    interleaveStep_         *= 1024;

    string memoryFile       = params.find_string("memory_file", NO_STRING_DEFINED);
    string clock_freq       = params.find_string("clock", "");
    cacheLineSize_          = params.find_integer("request_width", 64);
    //divertDCLookups_        = params.find_integer("divert_DC_lookups", 0);
    string backendName      = params.find_string("backend", "memHierarchy.simpleMem");
    string protocolStr      = params.find_string("coherence_protocol");
    string link_lat         = params.find_string("direct_link_latency", "100 ns");
    doNotBack_              = (params.find_integer("do_not_back",0) == 1);

    int addr = params.find_integer("network_address");
    std::string net_bw = params.find_string("network_bw");
    
    const uint32_t listenerCount  = (uint32_t) params.find_integer("listenercount", 0);
    char* nextListenerName   = (char*) malloc(sizeof(char) * 64);
    char* nextListenerParams = (char*) malloc(sizeof(char) * 64);

    for (uint32_t i = 0; i < listenerCount; ++i) {
	    sprintf(nextListenerName, "listener%" PRIu32, i);
	    string listenerMod     = params.find_string(nextListenerName, "");

            if (listenerMod != "") {
		sprintf(nextListenerParams, "listener%" PRIu32 ".", i);
		Params listenerParams = params.find_prefix_params(nextListenerParams);

		CacheListener* loadedListener = dynamic_cast<CacheListener*>(loadSubComponent(listenerMod, this, listenerParams));
		listeners_.push_back(loadedListener);
	    }
    }

    free(nextListenerName);
    free(nextListenerParams);

    string traceFileLoc     = params.find_string("trace_file", "");
    if ("" != traceFileLoc) {
	traceFP = fopen(traceFileLoc.c_str(), "wt");
    } else {
	traceFP = NULL;
    }

    const uint64_t backendRamSizeMB = params.find_integer("backend.mem_size", 0);

    if (params.find("mem_size") != params.end()) {
	dbg.fatal(CALL_INFO, -1, "%s, Error - you specified memory size by the \"mem_size\" parameter, this must now be backend.mem_size, change the parameter name in your input deck.\n", getName().c_str());
    }

    if (0 == backendRamSizeMB) {
	dbg.fatal(CALL_INFO, -1, "%s, Error - you specified 0MBs for backend.mem_size, the memory must have a non-zero size!\n", getName().c_str());
    }

    // Convert into MBs
    memSize_ = backendRamSizeMB * (1024*1024ul);

    requestWidth_           = cacheLineSize_;
    requestSize_            = cacheLineSize_;
    numPages_               = (interleaveStep_ > 0 && interleaveSize_ > 0) ? memSize_ / interleaveSize_ : 0;
    protocol_               = (protocolStr == "mesi" || protocolStr == "MESI") ? 1 : 0;


    // Ensure we can extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");
    backend_                = dynamic_cast<MemBackend*>(loadSubComponent(backendName, this, backendParams));

    if (isPortConnected("direct_link")) {
        cacheLink_   = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));
        networkLink_ = NULL;
    } else {
        if (!isPortConnected("network")) {
            dbg.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port        = "network";
        myInfo.link_bandwidth   = net_bw;
        myInfo.num_vcs          = 1;
        if (params.find_integer("network_num_vc", 1) != 1) {
            dbg.debug(_WARNING_, "%s, WARNING Deprecated parameter: network_num_vc. memHierarchy only uses one virtual channel.\n", getName().c_str());
        }
        myInfo.name             = getName();
        myInfo.network_addr     = addr;
        myInfo.type             = MemNIC::TypeMemory;
        myInfo.link_inbuf_size  = params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = params.find_string("network_output_buffer_size", "1KB");
        networkLink_ = new MemNIC(this, &dbg, -1, myInfo, new Event::Handler<MemController>(this, &MemController::handleEvent));

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.rangeStart       = rangeStart_;
        typeInfo.rangeEnd         = memSize_;
        typeInfo.interleaveSize   = interleaveSize_;
        typeInfo.interleaveStep   = interleaveStep_;
        networkLink_->addTypeInfo(typeInfo);
        cacheLink_ = NULL;
    }

    // Set up backing store if needed
    if (doNotBack_ && memoryFile != NO_STRING_DEFINED) {
        dbg.fatal(CALL_INFO, -1, "%s, Error - conflicting parameters. 'do_not_back_memory' cannot be true if 'memory_file' is specified. You specified: do_not_back_memory = %d, memory_file = %s\n",
                getName().c_str(), doNotBack_, memoryFile.c_str());
    }
    if (!doNotBack_) {
        int mmap_flags          = setBackingFile(memoryFile);
        memBuffer_              = (uint8_t*)mmap(NULL, memSize_, PROT_READ|PROT_WRITE, mmap_flags, backingFd_, 0);
        if (memBuffer_ == MAP_FAILED) {
            int err = errno;
            dbg.fatal(CALL_INFO,-1,"Failed to MMAP backing store for memory: %s, errno = %d\n", strerror(err), err);
        }
    }

    if (!backend_)          dbg.fatal(CALL_INFO,-1,"Unable to load Module %s as backend\n", backendName.c_str());

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSExReqReceived  = registerStatistic<uint64_t>("requests_received_GetSEx");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");


    if (protocolStr.empty()) {
	dbg.fatal(CALL_INFO, -1, "Coherency protocol not specified, please specify MESI or MSI\n");
    }

    cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    cyclesAttemptIssueButRejected = registerStatistic<uint64_t>(
	"cycles_attempted_issue_but_rejected" );
    totalCycles     = registerStatistic<uint64_t>( "total_cycles" );

    /* Clock Handler */
    registerClock(clock_freq, new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);
}



void MemController::handleEvent(SST::Event* event) {
    MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_,"Memory Controller: %s - Event Received. Cmd = %s\n", getName().c_str(), CommandString[ev->getCmd()]);
    dbg.debug(_L10_,"Event info: Addr: 0x%" PRIx64 ", dst = %s, src = %s, rqstr = %s, size = %d, prefetch = %d, vAddr = 0x%" PRIx64 ", instPtr = %" PRIx64 "\n",
        ev->getBaseAddr(), ev->getDst().c_str(), ev->getSrc().c_str(), ev->getRqstr().c_str(), ev->getSize(), ev->isPrefetch(), ev->getVirtualAddress(), ev->getInstructionPointer());
    Command cmd = ev->getCmd();

    // Notify our listeners that we have received an event
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
        case PutM:
            // Notify listeners that we have equiv. of a read
            if ( ! listeners_.empty()) {
		CacheListenerNotification notify(ev->getAddr(),	ev->getVirtualAddress(),
			ev->getInstructionPointer(), ev->getSize(), READ, HIT);

		for (unsigned long int i = 0; i < listeners_.size(); ++i) {
	        	listeners_[i]->notifyAccess(notify);
	    	}
	    }
            
            // Update statistics
            if (cmd == GetS)         stat_GetSReqReceived->addData(1);
            else if (cmd == GetX)    stat_GetXReqReceived->addData(1);
            else if (cmd == GetSEx)  stat_GetSExReqReceived->addData(1);
            else if (cmd == PutM)    stat_PutMReqReceived->addData(1);

            addRequest(ev);
            break;
        case PutS:
        case PutE:
            break;
        default:
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[cmd]);
    }

    delete event;
}



void MemController::addRequest(MemEvent* ev) {
    if ( !isRequestAddressValid(ev) ) {
        dbg.fatal(CALL_INFO, 1, "MemoryController \"%s\" received request from \"%s\" with invalid address.\n"
                "\t\tRequested Address:   0x%" PRIx64 "\n"
                "\t\tMC Range Start:      0x%" PRIx64 "\n"
                "\t\tMC Range End:        0x%" PRIx64 "\n"
                "\t\tMC Interleave Step:  0x%" PRIx64 "\n"
                "\t\tMC Interleave Size:  0x%" PRIx64 "\n",
                getName().c_str(),
                ev->getSrc().c_str(), ev->getAddr(),
                rangeStart_, (rangeStart_ + memSize_),
                interleaveStep_, interleaveSize_);
    }
    Command cmd   = ev->getCmd();
    DRAMReq* req = new DRAMReq(ev, requestWidth_, cacheLineSize_);
    
    requestPool_.insert(req);
    requestQueue_.push_back(req);
    
    dbg.debug(_L10_,"Creating DRAM Request. BsAddr = %" PRIx64 ", Size: %" PRIu64 ", %s\n", req->baseAddr_, req->size_, CommandString[cmd]);
}



bool MemController::clock(Cycle_t cycle) {
    totalCycles->addData(1);
    backend_->clock();
    if (networkLink_) networkLink_->clock();

    while ( !requestQueue_.empty()) {
        DRAMReq *req = requestQueue_.front();
        req->status_ = DRAMReq::PROCESSING;

        bool issued = backend_->issueRequest(req);
        if (issued) {
		cyclesWithIssue->addData(1);
	} else {
		cyclesAttemptIssueButRejected->addData(1);
		break;
	}

        req->amtInProcess_ += requestSize_;

        if (req->amtInProcess_ >= req->size_) {
            dbg.debug(_L10_, "Completed issue of request\n");
            performRequest(req);
            requestQueue_.pop_front();
        }
    }

    stat_outstandingReqs->addData(requestPool_.size());

    return false;
}



void MemController::performRequest(DRAMReq* req) {
    bool noncacheable  = req->reqEvent_->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localBaseAddr = convertAddressToLocalAddress(req->baseAddr_);
    Addr localAddr;

    if (req->cmd_ == PutM) {  /* Write request to memory */
        dbg.debug(_L10_,"WRITE.  Addr = %" PRIx64 ", Request size = %i , Noncacheable Req = %s\n",localBaseAddr, req->reqEvent_->getSize(), noncacheable ? "true" : "false");
	
        if (doNotBack_) return;
        
        for ( size_t i = 0 ; i < req->reqEvent_->getSize() ; i++) 
            memBuffer_[localBaseAddr + i] = req->reqEvent_->getPayload()[i];
        
    } else {
        Addr localAbsAddr = convertAddressToLocalAddress(req->reqEvent_->getAddr());
        
        if (noncacheable && req->cmd_ == GetX) {
            dbg.debug(_L10_,"WRITE. Noncacheable request, Addr = %" PRIx64 ", Request size = %i\n", localAbsAddr, req->reqEvent_->getSize());
            
            if (!doNotBack_) {
                for ( size_t i = 0 ; i < req->reqEvent_->getSize() ; i++)
                    memBuffer_[localAbsAddr + i] = req->reqEvent_->getPayload()[i];
            }
        }
        
        if (noncacheable)   localAddr = localAbsAddr;
        else                localAddr = localBaseAddr;
        
    	req->respEvent_ = req->reqEvent_->makeResponse();

	if (req->respEvent_->getSize() != req->reqEvent_->getSize()) {
		dbg.fatal(CALL_INFO, -1, "Request and response sizes do not match: %" PRIu32 " != %" PRIu32 "\n",
			(uint32_t) req->respEvent_->getSize(), (uint32_t) req->reqEvent_->getSize());
        }

        dbg.debug(_L10_, "READ.  Addr = %" PRIx64 ", Request size = %i\n", localAddr, req->reqEvent_->getSize());
    
        for ( size_t i = 0 ; i < req->respEvent_->getSize() ; i++) 
            req->respEvent_->getPayload()[i] = doNotBack_ ? 0 : memBuffer_[localAddr + i];

        if (noncacheable) req->respEvent_->setFlag(MemEvent::F_NONCACHEABLE);
        
        if (req->reqEvent_->getCmd() == GetX) req->respEvent_->setGrantedState(M);
        else {
            if (protocol_) req->respEvent_->setGrantedState(E); // Directory controller supersedes this; only used if DirCtrl does not exist
            else req->respEvent_->setGrantedState(S);
        }
	}
}


void MemController::sendResponse(DRAMReq* req) {
    if (req->reqEvent_->getCmd() != PutM) {
        if (networkLink_) networkLink_->send(req->respEvent_);
        else cacheLink_->send(req->respEvent_);
    }
    req->status_ = DRAMReq::DONE;
    
    requestPool_.erase(req);
    delete req;
}



void MemController::printMemory(DRAMReq* req, Addr localAddr) {
    dbg.debug(_L10_,"Resp. Data: ");
    for (unsigned int i = 0; i < cacheLineSize_; i++) dbg.debug(_L10_,"%d",(int)memBuffer_[localAddr + i]);
}



void MemController::handleMemResponse(DRAMReq* req) {
    req->amtProcessed_ += requestSize_;
    if (req->amtProcessed_ >= req->size_) req->status_ = DRAMReq::RETURNED;

    dbg.debug(_L10_, "Finishing processing.  BaseAddr = %" PRIx64 " %s\n", req->baseAddr_, req->status_ == DRAMReq::RETURNED ? "RETURNED" : "");

    if (DRAMReq::RETURNED == req->status_) sendResponse(req);
}



MemController::~MemController() {
    while ( requestPool_.size()) {
        DRAMReq *req = *(requestPool_.begin());
        requestPool_.erase(req);
        delete req;
    }
}


void MemController::init(unsigned int phase) {
    if (!networkLink_) {
        SST::Event *ev = NULL;
        while (NULL != (ev = cacheLink_->recvInitData())) {
            MemEvent *me = dynamic_cast<MemEvent*>(ev);
            if (!me) {
                delete ev;
                return;
            }
            /* Push data to memory */
            if (GetX == me->getCmd()) {
                dbg.debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), me->getAddr());
                if (isRequestAddressValid(me) && !doNotBack_) {
                    Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                    for ( size_t i = 0 ; i < me->getSize() ; i++) {
                        memBuffer_[localAddr + i] = me->getPayload()[i];
                    }
                }
            } else {
                Output out("", 0, 0, Output::STDERR);
                out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
            }
            delete ev;
        }
    } else {
        networkLink_->init(phase);

        while (MemEvent *ev = networkLink_->recvInitData()) {
            /* Push data to memory */
            if (ev->getDst() == getName()) {
                if (GetX == ev->getCmd()) {
                    dbg.debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), ev->getAddr());
                    if (isRequestAddressValid(ev) && !doNotBack_) {
                        Addr localAddr = convertAddressToLocalAddress(ev->getAddr());
                        for ( size_t i = 0 ; i < ev->getSize() ; i++) {
                            memBuffer_[localAddr + i] = ev->getPayload()[i];
                        }
                    }
                } else {
                    Output out("", 0, 0, Output::STDERR);
                    out.debug(_L10_,"Memory received unexpected Init Command: %d\n", ev->getCmd());
                }
            }
            delete ev; 
        }
    }
}



void MemController::setup(void) {
    backend_->setup();
    if (networkLink_) networkLink_->setup();
}



void MemController::finish(void) {
	if (!doNotBack_) munmap(memBuffer_, memSize_);
	if (-1 != backingFd_) close(backingFd_);

    // Close the trace file IF it is opened
    if (NULL != traceFP) {
	fclose(traceFP);
    }

    backend_->finish();
    if (networkLink_) networkLink_->finish();
}



int MemController::setBackingFile(string memoryFile) {
	int mmap_flags = MAP_PRIVATE;
	if (NO_STRING_DEFINED != memoryFile) {
		backingFd_ = open(memoryFile.c_str(), O_RDWR);
		if (backingFd_ < 0) dbg.fatal(CALL_INFO,-1,"Failed to open backing file\n");
	} else {
		backingFd_  = -1;
		mmap_flags  |= MAP_ANON;
	}
    
    return mmap_flags;

}

