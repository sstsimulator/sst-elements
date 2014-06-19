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
#include "memoryController.h"
#include "util.h"

#include <assert.h>
#include <fcntl.h>
#include <sstream>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include "memEvent.h"
#include "bus.h"

#define NO_STRING_DEFINED "N/A"

using namespace SST;
using namespace SST::MemHierarchy;


MemBackend::MemBackend(Component *comp, Params &params) : Module(){
    ctrl = dynamic_cast<MemController*>(comp);
    if (!ctrl)
        _abort(MemBackend, "MemBackends expect to be loaded into MemControllers.\n");
}


/*************************** Simple Backend ********************/
SimpleMemory::SimpleMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string access_time = params.find_string("access_time", "100 ns");
    self_link = ctrl->configureSelfLink("Self", access_time,
            new Event::Handler<SimpleMemory>(this, &SimpleMemory::handleSelfEvent));
}

void SimpleMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    MemController::DRAMReq *req = ev->req;
    ctrl->handleMemResponse(req);
    delete event;
}


bool SimpleMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->addr + req->amt_in_process;
    ctrl->dbg.debug(_L10_, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    self_link->send(1, new MemCtrlEvent(req));
    return true;
}



#if defined(HAVE_LIBDRAMSIM)
/*************************** DRAMSim Backend ********************/
DRAMSimMemory::DRAMSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string deviceIniFilename = params.find_string("device_ini", NO_STRING_DEFINED);
    if ( NO_STRING_DEFINED == deviceIniFilename )
        _abort(MemController, "XML must define a 'device_ini' file parameter\n");
    std::string systemIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if ( NO_STRING_DEFINED == systemIniFilename )
        _abort(MemController, "XML must define a 'system_ini' file parameter\n");


    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
    memSystem = DRAMSim::getMemorySystemInstance(
            deviceIniFilename, systemIniFilename, "", "", ramSize);

    DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>
        *readDataCB, *writeDataCB;

    readDataCB = new DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &DRAMSimMemory::dramSimDone);
    writeDataCB = new DRAMSim::Callback<DRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &DRAMSimMemory::dramSimDone);

    memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
}


bool DRAMSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->addr + req->amt_in_process;
    bool ok = memSystem->willAcceptTransaction(addr);
    if ( !ok ) return false;
    ok = memSystem->addTransaction(req->isWrite, addr);
    if ( !ok ) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(_L10_, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}


void DRAMSimMemory::clock(){
    memSystem->update();
}


void DRAMSimMemory::finish(){
    memSystem->printStats(true);
}


void DRAMSimMemory::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    std::deque<MemController::DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(_L10_, "Memory Request for %"PRIx64" Finished [%zu reqs]\n", (Addr)addr, reqs.size());
    assert(reqs.size());
    MemController::DRAMReq *req = reqs.front();
    reqs.pop_front();
    if ( 0 == reqs.size() )
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}
#endif



#if defined(HAVE_LIBHYBRIDSIM)
/*************************** HybridSim Backend ********************/
HybridSimMemory::HybridSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string hybridIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if ( hybridIniFilename == NO_STRING_DEFINED )
        _abort(MemController, "XML must define a 'system_ini' file parameter\n");

    memSystem = HybridSim::getMemorySystemInstance( 1, hybridIniFilename);

    typedef HybridSim::Callback <HybridSimMemory, void, uint, uint64_t, uint64_t> hybridsim_callback_t;
    HybridSim::TransactionCompleteCB *read_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    HybridSim::TransactionCompleteCB *write_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    memSystem->RegisterCallbacks(read_cb, write_cb);
}


bool HybridSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->addr + req->amt_in_process;

    bool ok = memSystem->WillAcceptTransaction();
    if ( !ok ) return false;
    ok = memSystem->addTransaction(req->isWrite, addr);
    if ( !ok ) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(_L10_, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}


void HybridSimMemory::clock(){
    memSystem->update();
}

void HybridSimMemory::finish(){
    memSystem->printLogfile();
}

void HybridSimMemory::hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle){
    std::deque<MemController::DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(_L10_, "Memory Request for %"PRIx64" Finished [%zu reqs]\n", addr, reqs.size());
    assert(reqs.size());
    MemController::DRAMReq *req = reqs.front();
    reqs.pop_front();
    if ( reqs.size() == 0 )
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}

#endif


/*************************** VaultSim Backend ********************/
VaultSimMemory::VaultSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string access_time = params.find_string("access_time", "100 ns");
    cube_link = ctrl->configureLink( "cube_link", access_time,
            new Event::Handler<VaultSimMemory>(this, &VaultSimMemory::handleCubeEvent));
}


bool VaultSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->addr + req->amt_in_process;
    ctrl->dbg.debug(_L10_, "Issued transaction to Cube Chain for address %"PRIx64"\n", (Addr)addr);
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (outToCubes.size() > 255) {
        req->status = MemController::DRAMReq::NEW;
        return false;
    }
    MemEvent::id_type reqID = req->reqEvent->getID();
    assert(outToCubes.find(reqID) == outToCubes.end());
    outToCubes[reqID] = req; // associate the memEvent w/ the DRAMReq
    MemEvent *outgoingEvent = new MemEvent(*req->reqEvent); // we make a copy, because the dramreq keeps to 'original'
    cube_link->send(outgoingEvent); // send the event off
    return true;
}


void VaultSimMemory::handleCubeEvent(SST::Event *event){
  MemEvent *ev = dynamic_cast<MemEvent*>(event);
  if (ev) {
    memEventToDRAMMap_t::iterator ri = outToCubes.find(ev->getResponseToID());
    if (ri != outToCubes.end()) {
      ctrl->handleMemResponse(ri->second);
      outToCubes.erase(ri);
      delete event;
    }
    else _abort(MemController, "Could not match incoming request from cubes\n");
  }
  else _abort(MemController, "Recived wrong event type from cubes\n");

}



/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id){
    int debugLevel = params.find_integer("debug_level", 0);
    if(debugLevel < 0 || debugLevel > 10)
        _abort(MemController, "Debugging level must be betwee 0 and 10. \n");
    
    dbg.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    dbg.debug(_L10_,"---");
    statsOutputTarget = (Output::output_location_t)params.find_integer("statistics", 0);

    bool tfFound = false;
    std::string traceFile = params.find_string("trace_file", "", tfFound);
	if ( tfFound ) {
#ifdef HAVE_LIBZ
		traceFP = gzopen(traceFile.c_str(), "w");
		gzsetparams(traceFP, 9, Z_DEFAULT_STRATEGY);
#else
		traceFP = fopen(traceFile.c_str(), "w+");
#endif
    } else  traceFP = NULL;


    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
	if ( 0 == ramSize ) _abort(MemController, "Must specify RAM size (mem_size) in MB\n");
	
    memSize         = ramSize * (1024*1024ul);
	rangeStart      = (Addr)params.find_integer("range_start", 0);
	interleaveSize  = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize  *= 1024;
	interleaveStep  = (Addr)params.find_integer("interleave_step", 0);
    interleaveStep  *= 1024;
    if ( interleaveStep > 0 && interleaveSize > 0 ) numPages = memSize / interleaveSize;
    else numPages = 0;


	std::string memoryFile  = params.find_string("memory_file", NO_STRING_DEFINED);
	std::string clock_freq  = params.find_string("clock", "");
    cacheLineSize           = params.find_integer("request_width", 64);
    divert_DC_lookups       = params.find_integer("divert_DC_lookups", 0);
    std::string backendName = params.find_string("backend", "memHierarchy.simpleMem");

    requestWidth    = cacheLineSize;
    requestSize     = cacheLineSize;

    registerClock(clock_freq, new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);


    backend = dynamic_cast<MemBackend*>(loadModuleWithComponent(backendName, this, params));
    if (!backend)  _abort(MemController, "Unable to load Module %s as backend\n", backendName.c_str());

	int mmap_flags = MAP_PRIVATE;
	if ( NO_STRING_DEFINED != memoryFile ) {
		backing_fd = open(memoryFile.c_str(), O_RDWR);
		if ( backing_fd < 0 ) _abort(MemController, "Unable to open backing file!\n");
	} else {
		backing_fd  = -1;
		mmap_flags  |= MAP_ANON;
	}

	memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, mmap_flags, backing_fd, 0);
	if ( !memBuffer ) _abort(MemController, "Unable to MMAP backing store for Memory\n");

    std::string link_lat = params.find_string("direct_link_latency", "100 ns");
    upstream_link = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));

    std::string protocolStr = params.find_string("coherence_protocol");
    assert(!protocolStr.empty());
    
    if(protocolStr == "mesi" || protocolStr == "MESI") protocol = 1;
    else protocol = 0;

    respondToInvalidates = false;

    GetSReqReceived     = 0;
    GetXReqReceived     = 0;
    PutMReqReceived     = 0;
    GetSExReqReceived   = 0;
    numReqOutstanding   = 0;
    numCycles           = 0;
}


MemController::~MemController(){
#ifdef HAVE_LIBZ
   if ( traceFP ) gzclose(traceFP);
#else
    if ( traceFP ) fclose(traceFP);
#endif
    while ( requests.size() ) {
        DRAMReq *req = requests.front();
        requests.pop_front();
        delete req;
    }
}


void MemController::init(unsigned int phase){
	SST::Event *ev = NULL;
    while ( NULL != (ev = upstream_link->recvInitData()) ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if(!me){
            delete ev;
            return;
        }
        /* Push data to memory */
        if (GetX == me->getCmd()) {
            if ( isRequestAddressValid(me) ) {
                Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                for ( size_t i = 0 ; i < me->getSize() ; i++ )
                    memBuffer[localAddr + i] = me->getPayload()[i];
            }
        }
        else {
            Output out("", 0, 0, Output::STDERR);
            out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd() );
        }
        delete ev;
    }

}

void MemController::setup(void){
    backend->setup();
}


void MemController::finish(void){
	munmap(memBuffer, memSize);
	if ( -1 != backing_fd ) close(backing_fd);

    backend->finish();

    Output out("", 0, 0, statsOutputTarget);
    out.output("\n--------------------------------------------------------------------\n");
    out.output("--- Main Memory Stats\n");
    out.output("--- Name: %s\n", getName().c_str());
    out.output("--------------------------------------------------------------------\n");
    out.output("- GetS received (read):  %"PRIu64"\n", GetSReqReceived);
    out.output("- GetX received (read):  %"PRIu64"\n", GetXReqReceived);
    out.output("- GetSEx received (read):  %"PRIu64"\n", GetSExReqReceived);
    out.output("- PutM received (write):  %"PRIu64"\n", PutMReqReceived);
    out.output("- Avg. Requests out: %.3f\n",float(numReqOutstanding)/float(numCycles));

}

void MemController::handleEvent(SST::Event *event){
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_,"Memory Controller - Event Received. Cmd = %s\n", CommandString[ev->getCmd()]);
    Command cmd = ev->getCmd();
    
    if(cmd == GetS || cmd == GetX || cmd == GetSEx || cmd == PutM){
        if(cmd == GetS)         GetSReqReceived++;
        else if(cmd == GetX)    GetXReqReceived++;
        else if(cmd == GetSEx)  GetSExReqReceived++;
        else if(cmd == PutM)    PutMReqReceived++;
    
        addRequest(ev);
    }
    else if(cmd == PutS || cmd == PutE) return;
    else _abort(MemController, "MemController:  Command not supported, Cmd = %s", CommandString[cmd]);
    
    delete event;
}


void MemController::addRequest(MemEvent *ev){
	dbg.debug(_L10_,"New Memory Request for %"PRIx64"\n", ev->getAddr());
    Command cmd = ev->getCmd();
    DRAMReq *req;
    assert(isRequestAddressValid(ev));

    req = new DRAMReq(ev, requestWidth, cacheLineSize);
    dbg.debug(_L10_,"Creating DRAM Request for %"PRIx64", Size: %zu, %s\n", req->addr, req->size, CommandString[cmd]);
    requests.push_back(req);
    requestQueue.push_back(req);
}



bool MemController::clock(Cycle_t cycle){
    backend->clock();

    while ( !requestQueue.empty() ) {
        DRAMReq *req = requestQueue.front();
        req->status = DRAMReq::PROCESSING;

        bool issued = backend->issueRequest(req);
        if (!issued) break;

        req->amt_in_process += requestSize;

        if ( req->amt_in_process >= req->size ) {
            dbg.debug(_L10_, "Completed issue of request\n");
            performRequest(req);
#ifdef HAVE_LIBZ
            if(traceFP)
                gzprintf(traceFP, "%c %#08llx %"PRIx64"\n", req->isWrite ? 'w' : 'r', req->addr, cycle);
#else
            if(traceFP)
                fprintf(traceFP, "%c %#08llx %"PRIx64"\n", req->isWrite ? 'w' : 'r', req->addr, cycle);
#endif
            requestQueue.pop_front();
        }
    }

    /* Clean out old requests */
    while ( requests.size() ) {
        DRAMReq *req = requests.front();
        if ( DRAMReq::DONE == req->status ) {
            requests.pop_front();
            delete req;
        }
        else break;
    }

    numReqOutstanding += requests.size();
    numCycles++;

	return false;
}


bool MemController::isRequestAddressValid(MemEvent *ev){
    Addr addr = ev->getAddr();

    if (0 == numPages) return (addr >= rangeStart && addr < (rangeStart + memSize));
    if (addr < rangeStart)
        return false;
    
    addr = addr - rangeStart;
    Addr step = addr / interleaveStep;
    if (step >= numPages)
        return false;

    Addr offset = addr % interleaveStep;
    if (offset >= interleaveSize)
        return false;

    return true;
}


Addr MemController::convertAddressToLocalAddress(Addr addr){
    if (0 == numPages) return addr - rangeStart;
    
    addr = addr - rangeStart;
    Addr step = addr / interleaveStep;
    Addr offset = addr % interleaveStep;
    return (step * interleaveSize) + offset;
}


void MemController::performRequest(DRAMReq *req){
    bool uncached = req->reqEvent->queryFlag(MemEvent::F_UNCACHED);
    Addr localAddr = convertAddressToLocalAddress(req->addr);

    if(req->cmd == PutM){  /* Write request to memory */
        dbg.debug(_L10_,"WRITE.  Addr = %"PRIx64", Request size = %i , Uncached Req = %s\n",localAddr, req->reqEvent->getSize(), uncached ? "true" : "false");
		for ( size_t i = 0 ; i < req->reqEvent->getSize() ; i++ )
            memBuffer[localAddr + i] = req->reqEvent->getPayload()[i];
	}
    else{
        if(uncached && req->cmd == GetX) {
            Addr localUncachedAddr = convertAddressToLocalAddress(req->reqEvent->getAddr());
            dbg.debug(_L10_,"WRITE. Uncached request, Addr = %"PRIx64", Request size = %i\n", localUncachedAddr, req->reqEvent->getSize());
            for ( size_t i = 0 ; i < req->reqEvent->getSize() ; i++ )
                memBuffer[localUncachedAddr + i] = req->reqEvent->getPayload()[i];
        }

    	req->respEvent = req->reqEvent->makeResponse();
        req->respEvent->setSize(cacheLineSize);
    
        dbg.debug(_L10_, "READ.  Addr = %"PRIx64", Request size = %i\n",localAddr, req->reqEvent->getSize());
		for ( size_t i = 0 ; i < req->respEvent->getSize() ; i++ )
            req->respEvent->getPayload()[i] = memBuffer[localAddr + i];

        if(req->reqEvent->getCmd() == GetX) req->respEvent->setGrantedState(M);
        else{
            if(protocol) req->respEvent->setGrantedState(E);
            else req->respEvent->setGrantedState(S);
        }
	}
}


void MemController::sendResponse(DRAMReq *req){
    if(req->reqEvent->getCmd() != PutM)
        upstream_link->send(req->respEvent);
    req->status = DRAMReq::DONE;
}


void MemController::printMemory(DRAMReq *req, Addr localAddr){
    dbg.debug(_L10_,"Resp. Data: ");
    for(unsigned int i = 0; i < cacheLineSize; i++) dbg.debug(_L10_,"%d",(int)memBuffer[localAddr + i]);
}

void MemController::handleMemResponse(DRAMReq *req){
    req->amt_processed += requestSize;
    if (req->amt_processed >= req->size) req->status = DRAMReq::RETURNED;

    dbg.debug(_L10_, "Finishing processing for req %"PRIx64" %s\n", req->addr, req->status == DRAMReq::RETURNED ? "RETURNED" : "");

    if(DRAMReq::RETURNED == req->status) sendResponse(req);
}

void MemController::cancelEvent(MemEvent* ev){}

void MemController::sendBusPacket(Bus::key_t key){}

void MemController::sendBusCancel(Bus::key_t key) {}

void MemController::handleBusEvent(SST::Event *event){}

