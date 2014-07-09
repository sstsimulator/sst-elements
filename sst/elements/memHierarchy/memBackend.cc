//
//  memBackend.cpp
//  SST
//
//  Created by Caesar De la Paz III on 7/9/14.
//  Copyright (c) 2014 De la Paz, Cesar. All rights reserved.
//

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memBackend.h"
#include "memoryController.h"
#include "util.h"

#define NO_STRING_DEFINED "N/A"

using namespace SST;
using namespace SST::MemHierarchy;

MemBackend::MemBackend(Component *comp, Params &params) : Module(){
    ctrl = dynamic_cast<MemController*>(comp);
    if (!ctrl)
        _abort(MemBackend, "MemBackends expect to be loaded into MemControllers.\n");
}


/*------------------------------- Simple Backend ------------------------------- */
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
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    ctrl->dbg.debug(_L10_, "Issued transaction for address %"PRIx64"\n", (Addr)addr);
    self_link->send(1, new MemCtrlEvent(req));
    return true;
}



#if defined(HAVE_LIBDRAMSIM)
/*------------------------------- DRAMSim Backend -------------------------------*/
DRAMSimMemory::DRAMSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string deviceIniFilename = params.find_string("device_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == deviceIniFilename)
        _abort(MemController, "XML must define a 'device_ini' file parameter\n");
    std::string systemIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == systemIniFilename)
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
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    bool ok = memSystem->willAcceptTransaction(addr);
    if(!ok) return false;
    ok = memSystem->addTransaction(req->isWrite_, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
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
    if(0 == reqs.size())
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}
#endif



#if defined(HAVE_LIBHYBRIDSIM)
/*------------------------------- HybridSim Backend -------------------------------*/
HybridSimMemory::HybridSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string hybridIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if(hybridIniFilename == NO_STRING_DEFINED)
        _abort(MemController, "XML must define a 'system_ini' file parameter\n");

    memSystem = HybridSim::getMemorySystemInstance( 1, hybridIniFilename);

    typedef HybridSim::Callback <HybridSimMemory, void, uint, uint64_t, uint64_t> hybridsim_callback_t;
    HybridSim::TransactionCompleteCB *read_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    HybridSim::TransactionCompleteCB *write_cb = new hybridsim_callback_t(this, &HybridSimMemory::hybridSimDone);
    memSystem->RegisterCallbacks(read_cb, write_cb);
}



bool HybridSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;

    bool ok = memSystem->WillAcceptTransaction();
    if(!ok) return false;
    ok = memSystem->addTransaction(req->isWrite_, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
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
    if(reqs.size() == 0)
        dramReqs.erase(addr);

    ctrl->handleMemResponse(req);
}

#endif


/*------------------------------- VaultSim Backend -------------------------------*/
VaultSimMemory::VaultSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string access_time = params.find_string("access_time", "100 ns");
    cube_link = ctrl->configureLink( "cube_link", access_time,
            new Event::Handler<VaultSimMemory>(this, &VaultSimMemory::handleCubeEvent));
}



bool VaultSimMemory::issueRequest(MemController::DRAMReq *req){
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    ctrl->dbg.debug(_L10_, "Issued transaction to Cube Chain for address %"PRIx64"\n", (Addr)addr);
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (outToCubes.size() > 255) {
        req->status_ = MemController::DRAMReq::NEW;
        return false;
    }
    MemEvent::id_type reqID = req->reqEvent_->getID();
    assert(outToCubes.find(reqID) == outToCubes.end());
    outToCubes[reqID] = req; // associate the memEvent w/ the DRAMReq
    MemEvent *outgoingEvent = new MemEvent(*req->reqEvent_); // we make a copy, because the dramreq keeps to 'original'
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

