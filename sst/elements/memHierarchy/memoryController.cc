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
    if(debugLevel < 0 || debugLevel > 10)
        _abort(MemController, "Debugging level must be between 0 and 10. \n");
    
    dbg.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    dbg.debug(_L10_,"---");
    
    statsOutputTarget_      = (Output::output_location_t)params.find_integer("statistics", 0);
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
    int  directLink         = params.find_integer("direct_link",1);

    isNetworkConnected_     = directLink ? false : true;
    int addr = params.find_integer("network_address");
    std::string net_bw = params.find_string("network_bw");
    
    const uint32_t listenerCount  = (uint32_t) params.find_integer("listenercount", 0);
    char* nextListenerName   = (char*) malloc(sizeof(char) * 64);
    char* nextListenerParams = (char*) malloc(sizeof(char) * 64);

    for(uint32_t i = 0; i < listenerCount; ++i) {
	    sprintf(nextListenerName, "listener%" PRIu32, i);
	    string listenerMod     = params.find_string(nextListenerName, "");

            if(listenerMod != "") {
		sprintf(nextListenerParams, "listener%" PRIu32 ".", i);
		Params listenerParams = params.find_prefix_params(nextListenerParams);

		CacheListener* loadedListener = dynamic_cast<CacheListener*>(loadModule(listenerMod, listenerParams));
		listeners_.push_back(loadedListener);
	    }
    }

    free(nextListenerName);
    free(nextListenerParams);

    string traceFileLoc     = params.find_string("trace_file", "");
    if("" != traceFileLoc) {
	traceFP = fopen(traceFileLoc.c_str(), "wt");
    } else {
	traceFP = NULL;
    }

    const uint64_t backendRamSizeMB = params.find_integer("backend.mem_size", 0);

    if(params.find("mem_size") != params.end()) {
	_abort(MemController, "Error - you specified memory size by the \"mem_size\" parameter, this must now be backend.mem_size, change the parameter name in your input deck.\n");
    }

    if(0 == backendRamSizeMB) {
	_abort(MemController, "Error - you specified 0MBs for backend.mem_size, the memory must have a non-zero size!\n");
    }

    // Convert into MBs
    memSize_ = backendRamSizeMB * (1024*1024ul);

    requestWidth_           = cacheLineSize_;
    requestSize_            = cacheLineSize_;
    numPages_               = (interleaveStep_ > 0 && interleaveSize_ > 0) ? memSize_ / interleaveSize_ : 0;
    protocol_               = (protocolStr == "mesi" || protocolStr == "MESI") ? 1 : 0;

    int mmap_flags          = setBackingFile(memoryFile);

    // Ensure we can extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");
    backend_                = dynamic_cast<MemBackend*>(loadModuleWithComponent(backendName, this, backendParams));

    if (!isNetworkConnected_) {
    lowNetworkLink_         = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));
    } else {
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port        = "network";
        myInfo.link_bandwidth   = net_bw;
        myInfo.num_vcs          = params.find_integer("network_num_vc", 3);
        myInfo.name             = getName();
        myInfo.network_addr     = addr;
        myInfo.type             = MemNIC::TypeMemory;
        myInfo.link_inbuf_size  = params.find_string("network_input_buffer_size", "1KB");
        myInfo.link_outbuf_size = params.find_string("network_output_buffer_size", "1KB");
        networkLink_ = new MemNIC(this, myInfo, new Event::Handler<MemController>(this, &MemController::handleEvent));

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.rangeStart       = rangeStart_;
        typeInfo.rangeEnd         = memSize_;
        typeInfo.interleaveSize   = interleaveSize_;
        typeInfo.interleaveStep   = interleaveStep_;
        networkLink_->addTypeInfo(typeInfo);
    }
    memBuffer_              = (uint8_t*)mmap(NULL, memSize_, PROT_READ|PROT_WRITE, mmap_flags, backingFd_, 0);

    if(!memBuffer_)         dbg.fatal(CALL_INFO,-1,"Failed to MMAP backing store for memory\n");
    if (!backend_)          dbg.fatal(CALL_INFO,-1,"Unable to load Module %s as backend\n", backendName.c_str());

    GetSReqReceived_        = 0;
    GetXReqReceived_        = 0;
    PutMReqReceived_        = 0;
    GetSExReqReceived_      = 0;
    numReqOutstanding_      = 0;
    numCycles_              = 0;

    if(protocolStr.empty()) {
	dbg.fatal(CALL_INFO, -1, "Coherency protocol not specified, please specify MESI or MSI\n");
    }

    /* Clock Handler */
    registerClock(clock_freq, new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);
}



void MemController::handleEvent(SST::Event* _event){
    MemEvent *ev = static_cast<MemEvent*>(_event);
    dbg.debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_,"Memory Controller: %s - Event Received. Cmd = %s\n", getName().c_str(), CommandString[ev->getCmd()]);
    Command cmd = ev->getCmd();

    // Notify our listeners that we have received an event
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
        case PutM:
            // Notify listeners that we have equiv. of a read
	    for(unsigned long int i = 0; i < listeners_.size(); ++i) {
	        listeners_[i]->notifyAccess(CacheListener::READ, CacheListener::HIT, ev->getAddr(), ev->getSize());
	    }

            if(cmd == GetS)         GetSReqReceived_++;
            else if(cmd == GetX)    GetXReqReceived_++;
            else if(cmd == GetSEx)  GetSExReqReceived_++;
            else if(cmd == PutM)    PutMReqReceived_++;

            addRequest(ev);
            break;
        case PutS:
        case PutE:
            break;
        default:
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[cmd]);
    }

    delete _event;
}



void MemController::addRequest(MemEvent* _ev){
    if ( !isRequestAddressValid(_ev) ) {
        dbg.fatal(CALL_INFO, 1, "MemoryController \"%s\" received request from \"%s\" with invalid address.\n"
                "\t\tRequested Address:   0x%" PRIx64 "\n"
                "\t\tMC Range Start:      0x%" PRIx64 "\n"
                "\t\tMC Range End:        0x%" PRIx64 "\n"
                "\t\tMC Interleave Step:  0x%" PRIx64 "\n"
                "\t\tMC Interleave Size:  0x%" PRIx64 "\n",
                getName().c_str(),
                _ev->getSrc().c_str(), _ev->getAddr(),
                rangeStart_, (rangeStart_ + memSize_),
                interleaveStep_, interleaveSize_);
    }
    Command cmd   = _ev->getCmd();
    DRAMReq* req = new DRAMReq(_ev, requestWidth_, cacheLineSize_);
    
    requests_.push_back(req);
    requestQueue_.push_back(req);
    
    dbg.debug(_L10_,"Creating DRAM Request. BsAddr = %" PRIx64 ", Size: %zu, %s\n", req->baseAddr_, req->size_, CommandString[cmd]);
}



bool MemController::clock(Cycle_t _cycle){
    backend_->clock();
    if (isNetworkConnected_) networkLink_->clock();

    while ( !requestQueue_.empty()) {
        DRAMReq *req = requestQueue_.front();
        req->status_ = DRAMReq::PROCESSING;

        bool issued = backend_->issueRequest(req);
        if (!issued) break;

        req->amtInProcess_ += requestSize_;

        if(req->amtInProcess_ >= req->size_) {
            dbg.debug(_L10_, "Completed issue of request\n");
            performRequest(req);
            requestQueue_.pop_front();
        }
    }

    /* Clean out old requests */
    while (!requests_.empty()) {
        DRAMReq *req = requests_.front();
        if(DRAMReq::DONE == req->status_) {
            requests_.pop_front();
            delete req;
        }
        else break;
    }

    numReqOutstanding_ += requests_.size();
    numCycles_++;

	return false;
}



void MemController::performRequest(DRAMReq* _req){
    bool noncacheable  = _req->reqEvent_->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localBaseAddr = convertAddressToLocalAddress(_req->baseAddr_);
    Addr localAddr;

    if(_req->cmd_ == PutM){  /* Write request to memory */
        dbg.debug(_L10_,"WRITE.  Addr = %" PRIx64 ", Request size = %i , Noncacheable Req = %s\n",localBaseAddr, _req->reqEvent_->getSize(), noncacheable ? "true" : "false");
	for ( size_t i = 0 ; i < _req->reqEvent_->getSize() ; i++) 
            memBuffer_[localBaseAddr + i] = _req->reqEvent_->getPayload()[i];
        
	}
    else {
        Addr localAbsAddr = convertAddressToLocalAddress(_req->reqEvent_->getAddr());
        
        if(noncacheable && _req->cmd_ == GetX) {
            dbg.debug(_L10_,"WRITE. Noncacheable request, Addr = %" PRIx64 ", Request size = %i\n", localAbsAddr, _req->reqEvent_->getSize());
            for ( size_t i = 0 ; i < _req->reqEvent_->getSize() ; i++)
                memBuffer_[localAbsAddr + i] = _req->reqEvent_->getPayload()[i];
        }
        
        if(noncacheable) localAddr = localAbsAddr;
        else             localAddr = localBaseAddr;
        
    	_req->respEvent_ = _req->reqEvent_->makeResponse();

	if(_req->respEvent_->getSize() != _req->reqEvent_->getSize()) {
		dbg.fatal(CALL_INFO, -1, "Request and response sizes do not match: %" PRIu32 " != %" PRIu32 "\n",
			(uint32_t) _req->respEvent_->getSize(), (uint32_t) _req->reqEvent_->getSize());
        }

        dbg.debug(_L10_, "READ.  Addr = %" PRIx64 ", Request size = %i\n", localAddr, _req->reqEvent_->getSize());

        for ( size_t i = 0 ; i < _req->respEvent_->getSize() ; i++) 
            _req->respEvent_->getPayload()[i] = memBuffer_[localAddr + i];
        
        if (noncacheable) _req->respEvent_->setFlag(MemEvent::F_NONCACHEABLE);
        
        if(_req->reqEvent_->getCmd() == GetX) _req->respEvent_->setGrantedState(M);
        else {
            if(protocol_) _req->respEvent_->setGrantedState(E); // Directory controller supersedes this; only used if DirCtrl does not exist
            else _req->respEvent_->setGrantedState(S);
        }
	}
}


void MemController::sendResponse(DRAMReq* _req){
    if(_req->reqEvent_->getCmd() != PutM) {
        if (isNetworkConnected_) networkLink_->send(_req->respEvent_);
        else lowNetworkLink_->send(_req->respEvent_);
    }
    _req->status_ = DRAMReq::DONE;
}



void MemController::printMemory(DRAMReq* _req, Addr localAddr){
    dbg.debug(_L10_,"Resp. Data: ");
    for(unsigned int i = 0; i < cacheLineSize_; i++) dbg.debug(_L10_,"%d",(int)memBuffer_[localAddr + i]);
}



void MemController::handleMemResponse(DRAMReq* _req){
    _req->amtProcessed_ += requestSize_;
    if (_req->amtProcessed_ >= _req->size_) _req->status_ = DRAMReq::RETURNED;

    dbg.debug(_L10_, "Finishing processing.  BaseAddr = %" PRIx64 " %s\n", _req->baseAddr_, _req->status_ == DRAMReq::RETURNED ? "RETURNED" : "");

    if(DRAMReq::RETURNED == _req->status_) sendResponse(_req);
}



MemController::~MemController(){
    while ( requests_.size()) {
        DRAMReq *req = requests_.front();
        requests_.pop_front();
        delete req;
    }
}


void MemController::init(unsigned int _phase){
    if (!isNetworkConnected_) {
        SST::Event *ev = NULL;
        while (NULL != (ev = lowNetworkLink_->recvInitData())) {
            MemEvent *me = dynamic_cast<MemEvent*>(ev);
            if (!me) {
                delete ev;
                return;
            }
            /* Push data to memory */
            if (GetX == me->getCmd()) {
                dbg.debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), me->getAddr());
                if(isRequestAddressValid(me)) {
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
        networkLink_->init(_phase);

        while (MemEvent *ev = networkLink_->recvInitData()) {
            /* Push data to memory */
            if (ev->getDst() == getName()) {
                if (GetX == ev->getCmd()) {
                    dbg.debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), ev->getAddr());
                    if(isRequestAddressValid(ev)) {
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



void MemController::setup(void){
    backend_->setup();
    if (isNetworkConnected_) networkLink_->setup();
}



void MemController::finish(void){
	munmap(memBuffer_, memSize_);
	if(-1 != backingFd_) close(backingFd_);

    // Close the trace file IF it is opened
    if(NULL != traceFP) {
	fclose(traceFP);
    }

    backend_->finish();
    if (isNetworkConnected_) networkLink_->finish();

    Output out("", 0, 0, statsOutputTarget_);
    out.output("\n--------------------------------------------------------------------\n");
    out.output("--- Main Memory Stats\n");
    out.output("--- Name: %s\n", getName().c_str());
    out.output("--------------------------------------------------------------------\n");
    out.output("- GetS received (read):  %" PRIu64 "\n", GetSReqReceived_);
    out.output("- GetX received (read):  %" PRIu64 "\n", GetXReqReceived_);
    out.output("- GetSEx received (read):  %" PRIu64 "\n", GetSExReqReceived_);
    out.output("- PutM received (write):  %" PRIu64 "\n", PutMReqReceived_);
    out.output("- Avg. Requests out: %.3f\n",float(numReqOutstanding_)/float(numCycles_));

}



int MemController::setBackingFile(string memoryFile){
	int mmap_flags = MAP_PRIVATE;
	if(NO_STRING_DEFINED != memoryFile) {
		backingFd_ = open(memoryFile.c_str(), O_RDWR);
		if(backingFd_ < 0) dbg.fatal(CALL_INFO,-1,"Failed to open backing file\n");
	} else {
		backingFd_  = -1;
		mmap_flags  |= MAP_ANON;
	}
    
    return mmap_flags;

}

