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

#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <fcntl.h>

#include "memEvent.h"
#include "memBackend.h"
#include "bus.h"

#define NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;



/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id){
    int debugLevel = params.find_integer("debug_level", 0);
    if(debugLevel < 0 || debugLevel > 10)
        _abort(MemController, "Debugging level must be betwee 0 and 10. \n");
    
    dbg.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    dbg.debug(_L10_,"---");
    
    statsOutputTarget_      = (Output::output_location_t)params.find_integer("statistics", 0);
    unsigned int ramSize    = (unsigned int)params.find_integer("mem_size", 0);
    memSize_                = ramSize * (1024*1024ul);
	rangeStart_             = (Addr)params.find_integer("range_start", 0);
	interleaveSize_         = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize_         *= 1024;
	interleaveStep_         = (Addr)params.find_integer("interleave_step", 0);
    interleaveStep_         *= 1024;

    if(0 == memSize_)       _abort(MemController, "Memory size must be bigger than zero and specified in MB\n");

	string memoryFile       = params.find_string("memory_file", NO_STRING_DEFINED);
	string clock_freq       = params.find_string("clock", "");
    cacheLineSize_          = params.find_integer("request_width", 64);
    divertDCLookups_        = params.find_integer("divert_DC_lookups", 0);
    string backendName      = params.find_string("backend", "memHierarchy.simpleMem");
    string protocolStr      = params.find_string("coherence_protocol");
    string link_lat         = params.find_string("direct_link_latency", "100 ns");
    
    requestWidth_           = cacheLineSize_;
    requestSize_            = cacheLineSize_;
    numPages_               = (interleaveStep_ > 0 && interleaveSize_ > 0) ? memSize_ / interleaveSize_ : 0;
    protocol_               = (protocolStr == "mesi" || protocolStr == "MESI") ? 1 : 0;

    int mmap_flags          = setBackingFile(memoryFile);
    backend_                = dynamic_cast<MemBackend*>(loadModuleWithComponent(backendName, this, params));
    lowNetworkLink_         = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));
	memBuffer_              = (uint8_t*)mmap(NULL, memSize_, PROT_READ|PROT_WRITE, mmap_flags, backingFd_, 0);

	if(!memBuffer_)         _abort(MemController, "Unable to MMAP backing store for Memory\n");
    if (!backend_)          _abort(MemController, "Unable to load Module %s as backend\n", backendName.c_str());

    GetSReqReceived_        = 0;
    GetXReqReceived_        = 0;
    PutMReqReceived_        = 0;
    GetSExReqReceived_      = 0;
    numReqOutstanding_      = 0;
    numCycles_              = 0;
    
    assert(!protocolStr.empty());

    /* Clock Handler */
    registerClock(clock_freq, new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);
}



void MemController::handleEvent(SST::Event* _event){
	MemEvent *ev = static_cast<MemEvent*>(_event);
    dbg.debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_,"Memory Controller - Event Received. Cmd = %s\n", CommandString[ev->getCmd()]);
    Command cmd = ev->getCmd();
    
    if(cmd == GetS || cmd == GetX || cmd == GetSEx || cmd == PutM){
        if(cmd == GetS)         GetSReqReceived_++;
        else if(cmd == GetX)    GetXReqReceived_++;
        else if(cmd == GetSEx)  GetSExReqReceived_++;
        else if(cmd == PutM)    PutMReqReceived_++;
    
        addRequest(ev);
    }
    else if(cmd == PutS || cmd == PutE) return;
    else _abort(MemController, "MemController:  Command not supported, Cmd = %s", CommandString[cmd]);
    
    delete _event;
}



void MemController::addRequest(MemEvent* _ev){
    assert(isRequestAddressValid(_ev));
    Command cmd   = _ev->getCmd();
    DRAMReq* req = new DRAMReq(_ev, requestWidth_, cacheLineSize_);
    
    requests_.push_back(req);
    requestQueue_.push_back(req);
    
    dbg.debug(_L10_,"Creating DRAM Request. BsAddr = %"PRIx64", Size: %zu, %s\n", req->baseAddr_, req->size_, CommandString[cmd]);
}



bool MemController::clock(Cycle_t _cycle){
    backend_->clock();

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
    bool uncached = _req->reqEvent_->queryFlag(MemEvent::F_UNCACHED);
    Addr localBaseAddr = convertAddressToLocalAddress(_req->baseAddr_);
    Addr localAddr;

    if(_req->cmd_ == PutM){  /* Write request to memory */
        dbg.debug(_L10_,"WRITE.  Addr = %"PRIx64", Request size = %i , Uncached Req = %s\n",localBaseAddr, _req->reqEvent_->getSize(), uncached ? "true" : "false");
		for ( size_t i = 0 ; i < _req->reqEvent_->getSize() ; i++)
            memBuffer_[localBaseAddr + i] = _req->reqEvent_->getPayload()[i];
	}
    else{
        Addr localAbsAddr = convertAddressToLocalAddress(_req->reqEvent_->getAddr());
        
        if(uncached && _req->cmd_ == GetX) {
            dbg.debug(_L10_,"WRITE. Uncached request, Addr = %"PRIx64", Request size = %i\n", localAbsAddr, _req->reqEvent_->getSize());
            for ( size_t i = 0 ; i < _req->reqEvent_->getSize() ; i++)
                memBuffer_[localAbsAddr + i] = _req->reqEvent_->getPayload()[i];
        }

        if(uncached) localAddr = localAbsAddr;
        else         localAddr = localBaseAddr;
        
    	_req->respEvent_ = _req->reqEvent_->makeResponse();
        assert(_req->respEvent_->getSize() == _req->reqEvent_->getSize());
    
        dbg.debug(_L10_, "READ.  Addr = %"PRIx64", Request size = %i\n", localAddr, _req->reqEvent_->getSize());
		for ( size_t i = 0 ; i < _req->respEvent_->getSize() ; i++)
            _req->respEvent_->getPayload()[i] = memBuffer_[localAddr + i];

        if(_req->reqEvent_->getCmd() == GetX) _req->respEvent_->setGrantedState(M);
        else{
            if(protocol_) _req->respEvent_->setGrantedState(E);
            else _req->respEvent_->setGrantedState(S);
        }
	}
}


void MemController::sendResponse(DRAMReq* _req){
    if(_req->reqEvent_->getCmd() != PutM)
        lowNetworkLink_->send(_req->respEvent_);
    _req->status_ = DRAMReq::DONE;
}



void MemController::printMemory(DRAMReq* _req, Addr localAddr){
    dbg.debug(_L10_,"Resp. Data: ");
    for(unsigned int i = 0; i < cacheLineSize_; i++) dbg.debug(_L10_,"%d",(int)memBuffer_[localAddr + i]);
}



void MemController::handleMemResponse(DRAMReq* _req){
    _req->amtProcessed_ += requestSize_;
    if (_req->amtProcessed_ >= _req->size_) _req->status_ = DRAMReq::RETURNED;

    dbg.debug(_L10_, "Finishing processing.  BaseAddr = %"PRIx64" %s\n", _req->baseAddr_, _req->status_ == DRAMReq::RETURNED ? "RETURNED" : "");

    if(DRAMReq::RETURNED == _req->status_) sendResponse(_req);
}



MemController::~MemController(){
#ifdef HAVE_LIBZ
   if(traceFP) gzclose(traceFP);
#else
    if(traceFP) fclose(traceFP);
#endif
    while ( requests_.size()) {
        DRAMReq *req = requests_.front();
        requests_.pop_front();
        delete req;
    }
}


void MemController::init(unsigned int _phase){
	SST::Event *ev = NULL;
    while ( NULL != (ev = lowNetworkLink_->recvInitData())) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if(!me){
            delete ev;
            return;
        }
        /* Push data to memory */
        if (GetX == me->getCmd()) {
            if(isRequestAddressValid(me)) {
                Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                for ( size_t i = 0 ; i < me->getSize() ; i++)
                    memBuffer_[localAddr + i] = me->getPayload()[i];
            }
        }
        else {
            Output out("", 0, 0, Output::STDERR);
            out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
        }
        delete ev;
    }

}



void MemController::setup(void){
    backend_->setup();
}



void MemController::finish(void){
	munmap(memBuffer_, memSize_);
	if(-1 != backingFd_) close(backingFd_);

    backend_->finish();

    Output out("", 0, 0, statsOutputTarget_);
    out.output("\n--------------------------------------------------------------------\n");
    out.output("--- Main Memory Stats\n");
    out.output("--- Name: %s\n", getName().c_str());
    out.output("--------------------------------------------------------------------\n");
    out.output("- GetS received (read):  %"PRIu64"\n", GetSReqReceived_);
    out.output("- GetX received (read):  %"PRIu64"\n", GetXReqReceived_);
    out.output("- GetSEx received (read):  %"PRIu64"\n", GetSExReqReceived_);
    out.output("- PutM received (write):  %"PRIu64"\n", PutMReqReceived_);
    out.output("- Avg. Requests out: %.3f\n",float(numReqOutstanding_)/float(numCycles_));

}



int MemController::setBackingFile(string memoryFile){
	int mmap_flags = MAP_PRIVATE;
	if(NO_STRING_DEFINED != memoryFile) {
		backingFd_ = open(memoryFile.c_str(), O_RDWR);
		if(backingFd_ < 0) _abort(MemController, "Unable to open backing file!\n");
	} else {
		backingFd_  = -1;
		mmap_flags  |= MAP_ANON;
	}
    
    return mmap_flags;

}

