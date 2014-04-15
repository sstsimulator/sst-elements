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


MemBackend::MemBackend(Component *comp, Params &params) :
    Module()
{
    ctrl = dynamic_cast<MemController*>(comp);
    if ( !ctrl ) {
        _abort(MemBackend, "MemBackends expect to be loaded into MemControllers.\n");
    }
}


/*************************** Simple Backend ********************/
SimpleMemory::SimpleMemory(Component *comp, Params &params) :
    MemBackend(comp, params)
{
    std::string access_time = params.find_string("access_time", "100 ns");
    self_link = ctrl->configureSelfLink("Self", access_time,
            new Event::Handler<SimpleMemory>(this, &SimpleMemory::handleSelfEvent));
}

void SimpleMemory::handleSelfEvent(SST::Event *event)
{
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    MemController::DRAMReq *req = ev->req;
    ctrl->handleMemResponse(req);
    delete event;
}


bool SimpleMemory::issueRequest(MemController::DRAMReq *req)
{
    uint64_t addr = req->addr + req->amt_in_process;
    ctrl->dbg.debug(CALL_INFO,6,0, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    self_link->send(1, new MemCtrlEvent(req));
    return true;
}



#if defined(HAVE_LIBDRAMSIM)
/*************************** DRAMSim Backend ********************/
DRAMSimMemory::DRAMSimMemory(Component *comp, Params &params) :
    MemBackend(comp, params)
{
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


bool DRAMSimMemory::issueRequest(MemController::DRAMReq *req)
{
    uint64_t addr = req->addr + req->amt_in_process;
    bool ok = memSystem->willAcceptTransaction(addr);
    if ( !ok ) return false;
    ok = memSystem->addTransaction(req->isWrite, addr);
    if ( !ok ) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(CALL_INFO,6,0, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}


void DRAMSimMemory::clock()
{
    memSystem->update();
}


void DRAMSimMemory::finish()
{
    memSystem->printStats(true);
}


void DRAMSimMemory::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle)
{
    std::deque<MemController::DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(CALL_INFO,6,0, "Memory Request for %"PRIx64" Finished [%zu reqs]\n", (Addr)addr, reqs.size());
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
HybridSimMemory::HybridSimMemory(Component *comp, Params &params) :
    MemBackend(comp, params)
{
    std::string hybridIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if ( hybridIniFilename == NO_STRING_DEFINED )
        _abort(MemController, "XML must define a 'system_ini' file parameter\n");

    memSystem = HybridSim::getMemorySystemInstance( 1, hybridIniFilename);

    typedef HybridSim::Callback <HybridSimMemory, void, uint, uint64_t, uint64_t> hybridsim_callback_t;
    HybridSim::TransactionCompleteCB *read_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    HybridSim::TransactionCompleteCB *write_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    memSystem->RegisterCallbacks(read_cb, write_cb);
}


bool HybridSimMemory::issueRequest(MemController::DRAMReq *req)
{
    uint64_t addr = req->addr + req->amt_in_process;

    bool ok = memSystem->WillAcceptTransaction();
    if ( !ok ) return false;
    ok = memSystem->addTransaction(req->isWrite, addr);
    if ( !ok ) return false;  // This *SHOULD* always be ok
    ctrl->dbg.debug(CALL_INFO,6,0, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    dramReqs[addr].push_back(req);
    return true;
}


void HybridSimMemory::clock()
{
    memSystem->update();
}

void HybridSimMemory::finish()
{
    memSystem->printLogfile();
}

void HybridSimMemory::hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle)
{
    std::deque<MemController::DRAMReq *> &reqs = dramReqs[addr];
    ctrl->dbg.debug(CALL_INFO,6,0, "Memory Request for %"PRIx64" Finished [%zu reqs]\n", addr, reqs.size());
    assert(reqs.size());
    MemController::DRAMReq *req = reqs.front();
    reqs.pop_front();
    if ( reqs.size() == 0 )
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}

#endif


/*************************** VaultSim Backend ********************/
VaultSimMemory::VaultSimMemory(Component *comp, Params &params) :
    MemBackend(comp, params)
{
    std::string access_time = params.find_string("access_time", "100 ns");
    cube_link = ctrl->configureLink( "cube_link", access_time,
            new Event::Handler<VaultSimMemory>(this, &VaultSimMemory::handleCubeEvent));
}


bool VaultSimMemory::issueRequest(MemController::DRAMReq *req)
{
    uint64_t addr = req->addr + req->amt_in_process;
    ctrl->dbg.debug(CALL_INFO,6,0, "Issued transaction to Cube Chain for address %"PRIx64"\n", (Addr)addr);
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (outToCubes.size() > 255) {
        req->status = MemController::DRAMReq::NEW;
        return false;
    }
    MemEvent::id_type reqID = req->reqEvent->getID();
    assert(outToCubes.find(reqID) == outToCubes.end());
    outToCubes[reqID] = req; // associate the memEvent w/ the DRAMReq
    MemEvent *outgoingEvent = new MemEvent(req->reqEvent); // we make a copy, because the dramreq keeps to 'original'
    cube_link->send(outgoingEvent); // send the event off
    return true;
}


void VaultSimMemory::handleCubeEvent(SST::Event *event)
{
  MemEvent *ev = dynamic_cast<MemEvent*>(event);
  if (ev) {
    memEventToDRAMMap_t::iterator ri = outToCubes.find(ev->getResponseToID());
    if (ri != outToCubes.end()) {
      ctrl->handleMemResponse(ri->second);
      outToCubes.erase(ri);
      delete event;
    } else {
      _abort(MemController, "Could not match incoming request from cubes\n");
    }
  } else {
    _abort(MemController, "Recived wrong event type from cubes\n");
  }
}



/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id)
{
    dbg.init("", 7, 0, (Output::output_location_t)params.find_integer("debug", 0));
    dbg.debug(C,L1,0,"---");
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
    } else {
        traceFP = NULL;
    }


    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
	if ( 0 == ramSize )
		_abort(MemController, "Must specify RAM size (mem_size) in MB\n");
	memSize = ramSize * (1024*1024ul);
	rangeStart = (Addr)params.find_integer("range_start", 0);
	interleaveSize = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize *= 1024;
	interleaveStep = (Addr)params.find_integer("interleave_step", 0);
    interleaveStep *= 1024;
    if ( interleaveStep > 0 && interleaveSize > 0 ) {
        numPages = memSize / interleaveSize;
    } else {
        numPages = 0;
    }

	std::string memoryFile = params.find_string("memory_file", NO_STRING_DEFINED);

	std::string clock_freq = params.find_string("clock", "");

    cacheLineSize = params.find_integer("request_width", 64);
    
    requestWidth = cacheLineSize;
    requestSize = cacheLineSize;

    registerClock(clock_freq, new Clock::Handler<MemController>(this,
				&MemController::clock));
    registerTimeBase("1 ns", true);

    // do we divert directory to the self-link?
    divert_DC_lookups = params.find_integer("divert_DC_lookups", 0);

    // check for and initialize dramsim
    std::string backendName = params.find_string("backend", "memHierarchy.simpleMem");
    backend = dynamic_cast<MemBackend*>(loadModuleWithComponent(backendName, this, params));
    if ( !backend ) {
        _abort(MemController, "Unable to load Module %s as backend\n", backendName.c_str());
    }



	int mmap_flags = MAP_PRIVATE;
	if ( NO_STRING_DEFINED != memoryFile ) {
		backing_fd = open(memoryFile.c_str(), O_RDWR);
		if ( backing_fd < 0 ) {
			_abort(MemController, "Unable to open backing file!\n");
		}
	} else {
		backing_fd = -1;
		mmap_flags |= MAP_ANON;
	}

	memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, mmap_flags, backing_fd, 0);
	if ( !memBuffer ) {
		_abort(MemController, "Unable to MMAP backing store for Memory\n");
	}

	upstream_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<MemController>(this, &MemController::handleBusEvent));
    use_bus = (NULL != upstream_link );

    if ( !upstream_link ) {
        std::string link_lat = params.find_string("direct_link_latency", "100 ns");
        upstream_link = configureLink( "direct_link", link_lat,
                new Event::Handler<MemController>(this, &MemController::handleEvent));
    }

    std::string protocolStr = params.find_string("coherence_protocol");
    assert(!protocolStr.empty());
    
    if(protocolStr == "mesi" || protocolStr == "MESI") protocol = 1;
    else protocol = 0;

    respondToInvalidates = false;

    numReadsSupplied = 0;
    numReadsCanceled = 0;
    numWrites = 0;
    numReqOutstanding = 0;
    numCycles = 0;
}


MemController::~MemController()
{
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


void MemController::init(unsigned int phase)
{
	if ( !phase ) {
        //upstream_link->sendInitData(new StringEvent("SST::MemHierarchy::MemEvent"));
	}

	SST::Event *ev = NULL;
    while ( NULL != (ev = upstream_link->recvInitData()) ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            /* Push data to memory */
            if ( GetX == me->getCmd() || WriteReq == me->getCmd() ) {
                if ( isRequestAddressValid(me) ) {
                    Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                    for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
                        memBuffer[localAddr + i] = me->getPayload()[i];
                    }
                }
            } else {
                Output out("", 0, 0, Output::STDERR);
                out.debug(CALL_INFO,6,0,"Memory received unexpected Init Command: %d\n", me->getCmd() );
            }
        }
        delete ev;
    }

}

void MemController::setup(void)
{
    backend->setup();
}


void MemController::finish(void)
{
	munmap(memBuffer, memSize);
	if ( -1 != backing_fd ) {
		close(backing_fd);
	}

    backend->finish();

    Output out("", 0, 0, statsOutputTarget);
    out.output("Memory %s stats:\n"
            "\t # Reads:             %"PRIu64"\n"
            "\t # Writes:            %"PRIu64"\n"
            "\t # Canceled Reads:    %"PRIu64"\n"
            "\t # Avg. Requests out: %.3f\n",
            getName().c_str(),
            numReadsSupplied,
            numWrites,
            numReadsCanceled,
            float(numReqOutstanding)/float(numCycles)
);

}

void MemController::handleEvent(SST::Event *event)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.output("\n\n----------------------------------------------------------------------------------------\n");
    dbg.output("Memory Controller - Event Received. Cmd = %s\n", CommandString[ev->getCmd()]);
    bool to_me = (!use_bus || ( ev->getDst() == getName() || BROADCAST_TARGET == ev->getDst() ));
    switch ( ev->getCmd() ) {
    /* Poseidon */
    case PutM:
        addRequest(ev);
        break;
    case PutS:
    case PutE:
        break;
    case GetS:
    case GetSEx:
        if ( to_me ) addRequest(ev);
        break;
    case GetX:
        if ( use_bus ) // don't cancel from what we sent.
            cancelEvent(ev);
        if ( !use_bus || ev->queryFlag(MemEvent::F_WRITEBACK) )
            addRequest(ev);
        break;
    default:
        _abort(MemController, "Command not supported");
        break;
    }
    delete event;
}

void MemController::handleBusEvent(SST::Event *event)
{
/*
    BusEvent *be = static_cast<BusEvent*>(event);
    switch ( be->getCmd() ) {
    case BusEvent::ClearToSend:
        sendBusPacket(be->getKey());
        break;
    case BusEvent::SendData:
        handleEvent(be->getPayload());
        break;
    default:
        _abort(MemController, "Bus Client should not be receiving this command.\n");
    }
    delete be;
*/
}

void MemController::addRequest(MemEvent *ev)
{
	dbg.debug(C,6,0, "New Memory Request for %"PRIx64"\n", ev->getAddr());
    Command cmd = ev->getCmd();
    DRAMReq *writeReq, *writeResp, *readReq;
    assert(isRequestAddressValid(ev));
    
    switch(cmd){
    case GetX:
    case PutM:
        writeReq = new DRAMReq(ev, requestWidth, cacheLineSize, NULLCMD);
        requests.push_back(writeReq);
        requestQueue.push_back(writeReq);
        dbg.debug(C,6,0, "Creating DRAM Request for %"PRIx64", Size: %zu, %s\n", writeReq->addr, writeReq->size, writeReq->isWrite ? "WRITE" : "READ");
        
        if(cmd != PutM){
            writeResp = new DRAMReq(ev, requestWidth, cacheLineSize, GetXResp);  //read back what we just wrote
            requests.push_back(writeResp);
            requestQueue.push_back(writeResp);
            dbg.debug(C,6,0, "Creating DRAM Request for %"PRIx64", Size: %zu, %s\n", writeResp->addr, writeResp->size, writeResp->isWrite ? "WRITE" : "READ");
        }
        break;
    
    case GetS:
    case GetSEx:
        readReq = new DRAMReq(ev, requestWidth, cacheLineSize, GetSResp);
        requests.push_back(readReq);
        requestQueue.push_back(readReq);
        dbg.debug(C,6,0, "Creating DRAM Request for %"PRIx64", Size: %zu, %s\n", readReq->addr, readReq->size, readReq->isWrite ? "WRITE" : "READ");
        break;
    default:
        _abort(MemController, "Command not supported, %s\n", CommandString[cmd]);
        break;
    }
}


void MemController::cancelEvent(MemEvent* ev)
{
	dbg.output(CALL_INFO, "Looking to cancel for (%"PRIx64")\n", ev->getAddr());
    for ( size_t i = 0 ; i < requests.size() ; ++i ) {
        if ( requests[i]->isSatisfiedBy(ev) ) {
            if ( !requests[i]->isWrite && !requests[i]->canceled ) {
                requests[i]->canceled = true;
                numReadsCanceled++;
                if ( NULL != requests[i]->respEvent )
                    dbg.debug(CALL_INFO,6,0, "Canceling request %"PRIx64").\n", requests[i]->addr);
                else
                    dbg.debug(CALL_INFO,6,0, "Canceling request %"PRIx64" (Not yet processed).\n", requests[i]->addr);
                if ( DRAMReq::RETURNED == requests[i]->status ) {
                    sendBusCancel(requests[i]->respEvent->getID());
                }
            }
        }
    }
}

bool MemController::clock(Cycle_t cycle)
{
    backend->clock();

    while ( !requestQueue.empty() ) {
        DRAMReq *req = requestQueue.front();
        if ( req->canceled ) {
            requestQueue.pop_front();
            if ( DRAMReq::NEW == req->status ) // Haven't started processing
                req->status = DRAMReq::DONE;
            continue;
        }

        req->status = DRAMReq::PROCESSING;

        bool issued = backend->issueRequest(req);
        if ( !issued ) break;

        req->amt_in_process += requestSize;

        if ( req->amt_in_process >= req->size ) {
            dbg.debug(CALL_INFO, 6,0, "Completed issue of request\n");
            performRequest(req);
#ifdef HAVE_LIBZ
            if ( traceFP ) {
                gzprintf(traceFP, "%c %#08llx %"PRIx64"\n",
                        req->isWrite ? 'w' : 'r', req->addr, cycle);
            }
#else
            if ( traceFP ) {
                fprintf(traceFP, "%c %#08llx %"PRIx64"\n",
                        req->isWrite ? 'w' : 'r', req->addr, cycle);
            }
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
        } else {
            break;
        }
    }

    numReqOutstanding += requests.size();
    numCycles++;

	return false;
}


bool MemController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if ( 0 == numPages ) {
        return ( addr >= rangeStart && addr < (rangeStart + memSize) );
    } else {
        if ( addr < rangeStart ) return false;

        addr = addr - rangeStart;
        Addr step = addr / interleaveStep;
        if ( step >= numPages ) return false;

        Addr offset = addr % interleaveStep;
        if ( offset >= interleaveSize ) return false;

        return true;
    }

}


Addr MemController::convertAddressToLocalAddress(Addr addr)
{
    if ( 0 == numPages ) {
        return addr - rangeStart;
    } else {
        addr = addr - rangeStart;
        Addr step = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        return (step * interleaveSize) + offset;
    }
}


void MemController::performRequest(DRAMReq *req)
{
   
	MemEvent *resp = req->reqEvent->makeResponse(this);
    if(req->GetXRespType && req->cmd == GetX) resp->setCmd(GetXResp);
    Addr localAddr = convertAddressToLocalAddress(req->addr);

    req->respEvent = resp;
    resp->setSize(cacheLineSize);
    
    //TODO: in MESI, initial GetX response should be in E state (this is for performance optimization, not correctness)
    //TODO: No need to write memory on GetX... only on PutM
	if ( req->isWrite || req->cmd == PutM) {  /* Write request to memory */
        dbg.debug(C,L1,0,"WRITE.  Addr = %"PRIx64", Request size = %i\n",localAddr, req->reqEvent->getSize());
		for ( size_t i = 0 ; i < req->reqEvent->getSize() ; i++ ) memBuffer[localAddr + i] = req->reqEvent->getPayload()[i];
        
        printMemory(req, localAddr);
        
	} else {
        dbg.debug(C,0,0,"READ.  Addr = %"PRIx64", Request size = %i\n",localAddr, req->reqEvent->getSize());
		for ( size_t i = 0 ; i < resp->getSize() ; i++ ) resp->getPayload()[i] = memBuffer[localAddr + i];

        if(req->GetXRespType) resp->setGrantedState(M);
        else{
            if(protocol) resp->setGrantedState(E);
            else resp->setGrantedState(S);
        }

        printMemory(req, localAddr);
	}
}



void MemController::sendResponse(DRAMReq *req)
{
    /*
    if ( use_bus ) {
        busReqs.push_back(req);
        Bus::key_t key = req->respEvent->getID();
        dbg.debug(CALL_INFO,6,0, "Requesting bus for event (%"PRIx64", %d)\n", key.first, key.second);
        upstream_link->send(new BusEvent(BusEvent::RequestBus, key));
    } else { */
        if(!req->isWrite){
            upstream_link->send(req->respEvent);
        }
        req->status = DRAMReq::DONE;
        if ( req->respEvent->getCmd() == GetXResp || req->respEvent->getCmd() == WriteResp ) numWrites++;
        else if ( req->respEvent->getCmd() == SupplyData ) numReadsSupplied++;
    //}
}


void MemController::printMemory(DRAMReq *req, Addr localAddr)
{
    dbg.debug(C,L1,0,"Resp. Data: ");
    for(unsigned int i = 0; i < cacheLineSize; i++) dbg.debug(C,L1,0,"%d",(int)memBuffer[localAddr + i]);
    dbg.debug(C,L1,0,"\n");
}

void MemController::handleMemResponse(DRAMReq *req)
{
    dbg.debug(CALL_INFO, 6,0, "Finishing processing for req %"PRIx64"\n", req->addr);
    req->amt_processed += requestSize;
    if ( req->amt_processed >= req->size ) {
        req->status = DRAMReq::RETURNED;
    }

    if ( DRAMReq::RETURNED == req->status ) {
        if ( !req->canceled )
            sendResponse(req);
        else
            req->status = DRAMReq::DONE;
    } else {
       if ( req->canceled ) {
           if ( req->amt_processed >= req->amt_in_process )
               req->status = DRAMReq::DONE;
       }
    }

}


void MemController::sendBusPacket(Bus::key_t key)
{
/*
    assert(use_bus);
	for (;;) {
		if ( 0 == busReqs.size() ) {
            dbg.output(CALL_INFO, "Sending cancelation, as we have nothing in the queue.\n");
			upstream_link->send(new BusEvent(BusEvent::CancelRequest, key));
			break;
		} else {
            DRAMReq *req = busReqs.front();
			busReqs.pop_front();
            req->status = DRAMReq::DONE;
			if ( !req->canceled ) {
                MemEvent *ev = req->respEvent;
				dbg.output(CALL_INFO, "Sending  Addr: %"PRIx64" in response to (%"PRIx64", %d) \n",
                        ev->getAddr(),
						ev->getResponseToID().first, ev->getResponseToID().second
						);
                assert(ev->getID() == key);
                if ( req->respEvent->getCmd() == WriteResp ) numWrites++;
                else if ( req->respEvent->getCmd() == SupplyData ) numReadsSupplied++;
				upstream_link->send(0, new BusEvent(ev));
				break;
            } else {
                if ( req->respEvent->getID() == key ) {
                    // Bus asked for this item, but it is canceled.
                    //This most likely means that we raced a cancelation
                    //and the bus giving us permission.  A bus cancelation
                    //will be received by the bus, and we don't need to do
                    //anything.  Pretend this thing never happened.
                    //
                    dbg.output(CALL_INFO, "Choosing to not send event for (%"PRIx64", %d), because we think it was recently canceled.\n", key.first, key.second);
                    break;
                } else {
                    dbg.output(CALL_INFO, "Skipping over canceled event (%"PRIx64", %d)\n", req->respEvent->getID().first, req->respEvent->getID().second);
                }
            }
		}
	}
*/
}

void MemController::sendBusCancel(Bus::key_t key) {
    /*
    assert(use_bus);
    dbg.output(CALL_INFO, "Sending cancelation of event (%"PRIx64", %d)\n", key.first, key.second);
    upstream_link->send(new BusEvent(BusEvent::CancelRequest, key));
    */
}

