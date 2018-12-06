// -*- mode: c++ -*-
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
//

#include <sst_config.h>
#include "memHierarchyScratchInterface.h"

#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

/***
 * Interface to hierarchies with scratchpads
 * Some notes:
 *  - 'Move' requests (such as cp address X in memory to Y in scratch or vice versa) addr[0] and addr[1] populated in the SimpleRequest
 *  - Scratchpad accesses are to addresses [0 - scratchMaxAddr_), regular accesses are (scratchMaxAdd_+]
 *  - Supported configurations:
 *      CPU -> Scratch -> Memory                        Accesses to addresses above scratchMaxAddr_ get data from memory
 
 *      CPU -> Cache(s) -> Scratch -> Memory            Scratch accesses can be cached. Accesses to addresses scratchMaxAddr_ are not cached
 
 *      CPU ---->    Cache(s) -> Memory                 Moves from memory to scratch will search the cache hierarchy but accesses to scratch won't
 *          -> Scratch _|
 
 *      CPU ---------------->    Cache(s) -> Memory     Moves from memory to scratch will search the cache hierarchy but accesses to scratch won't. 
 *          -> Cache(s) -> Scratch _|                   Scratch accesses can be cached in the caches on the scratchpad path.
 *  
 *  - If there are caches along both paths (last case above), the scratchpad will not be able to figure out which port belongs to the scratchpad path and which belongs to the
 *  cache path. So port0 must be scratchpad path and port1 must be cache path.
 */

MemHierarchyScratchInterface::MemHierarchyScratchInterface(SST::Component *comp, Params &params) :
    SimpleMem(comp, params), owner_(comp), recvHandler_(NULL), link_(NULL)
{ 
    output.init("", 1, 0, Output::STDOUT);

    bool found;
    UnitAlgebra size = UnitAlgebra(params.find<std::string>("scratchpad_size", "0B", found));
    if (!found) 
        output.fatal(CALL_INFO, -1, "Param not specified (%s): scratchpad_size - size of scratchpad\n", getName().c_str());
    if (!size.hasUnits("B"))
        output.fatal(CALL_INFO, -1, "Invalid param (%s): scratchpad_size - must have units of 'B'. SI units ok. You specified '%s'\n", getName().c_str(), size.toString().c_str());
    scratchMaxAddr_ = size.getRoundedValue();
    
    rqstr_ = "";
    initDone_ = false;
}


void MemHierarchyScratchInterface::init(unsigned int phase) {
    /* 
     * Check that there's a default timebase on the link, otherwise set one to avoid error
     * What we choose won't matter since we're not adding extra latency
     */
    if (link_->getDefaultTimeBase() == nullptr) {
        link_->setDefaultTimeBase(getTimeConverter("1ns"));
    }

    /* Step 0: Send our init messages */
    if (!phase) {
        // Name, NULLCMD, Endpoint type, inclusive of all upper levels, will send writeback acks, line size
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::CPU, false, false, 0, false));
    }

    /* Step 2: Receive init messages on both links */
    while (SST::Event * ev = link_->recvInitData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * memEventC = static_cast<MemEventInitCoherence*>(memEvent);

                initDone_ = true;
                 
                // Get parameters
                addrMask_ = ~(memEventC->getLineSize() - 1);
                if (rqstr_ == "") rqstr_ = memEventC->getSrc();
                dst_ = memEventC->getSrc();
                
            }
        }
        delete ev;
    }
    
    /* Step 3: Send data to initialize memory's backing store */
    if (initDone_) {
        while (!initSendQueue_.empty()) {
            link_->sendInitData(initSendQueue_.front());
            initSendQueue_.pop();
        }
    }
}


/* Called by CPU to initialize memory's backing store */
void MemHierarchyScratchInterface::sendInitData(SimpleMem::Request *req) {
    Addr addr = req->addrs[0];
    
    MemEventInit * event = new MemEventInit(getName(), Command::GetX, addr, req->data);
  
    if (initDone_)
        link_->sendInitData(event);
    else 
        initSendQueue_.push(event);
}


/* Called by CPU to send a new request */
void MemHierarchyScratchInterface::sendRequest(SimpleMem::Request *req){
    if (req->addrs.size() > 1) {                        // Scratchpad move request
        MemEventBase * event = createMoveEvent(req);
        requests_[event->getID()] = req;
        link_->send(event);
    } else {
        MemEventBase * event = createMemEvent(req);
        requests_[event->getID()] = req;
        link_->send(event);
    }
}


/* Handle a response */
SimpleMem::Request* MemHierarchyScratchInterface::recvResponse(void){
    SST::Event *ev = link_->recv();
    if (NULL != ev) {
        MemEventBase *event = static_cast<MemEventBase*>(ev);
        Request *req = processIncoming(event);
        delete event;
        return req;
    }
    return NULL;
}


MemEvent * MemHierarchyScratchInterface::createMemEvent(SimpleMem::Request * req) const {
    Command cmd = Command::NULLCMD;

    switch (req->cmd) {
        case SimpleMem::Request::Read :         cmd = Command::GetS;            break;
        case SimpleMem::Request::Write :        cmd = Command::GetX;            break;
        case SimpleMem::Request::FlushLine:     cmd = Command::FlushLine;       break;
        case SimpleMem::Request::FlushLineInv:  cmd = Command::FlushLineInv;    break;
        default: output.fatal(CALL_INFO, -1, "MemHierarchyScratchInterface received unknown command in createMemEvent\n");
    }

    Addr baseAddr = req->addrs[0] & addrMask_;

    MemEvent * me = new MemEvent(owner_, req->addrs[0], baseAddr, cmd);
    me->setSize(req->size);
    me->setRqstr(rqstr_);
    me->setSrc(rqstr_);
    me->setDst(dst_);
   
    /* If using the scratch path for a non-scratch access, set noncacheable flag */
    if (me->getAddr() >= scratchMaxAddr_) {
        me->setFlag(MemEvent::F_NONCACHEABLE);
    }

    if (SimpleMem::Request::Write == req->cmd) {
        if (req->data.size() == 0) req->data.resize(req->size, 0);
        if (req->data.size() != req->size)
            output.output("Warning: In memHierarchyScratchInterface, write request size does not match payload size. Request size: %zu, Payload size: %zu. Using payload size.\n", req->size, req->data.size());
        me->setPayload(req->data);
    }
    return me;
}


MoveEvent* MemHierarchyScratchInterface::createMoveEvent(SimpleMem::Request *req) const{
   Command cmd = Command::NULLCMD;

    switch ( req->cmd ) {
        case SimpleMem::Request::Read:  cmd = Command::Get; break;
        case SimpleMem::Request::Write: cmd = Command::Put; break;
        default: output.fatal(CALL_INFO, -1, "Unknown req->cmd in createMoveEvent()\n");
    }

    MoveEvent *me = new MoveEvent(parent->getName(), req->addrs[1], req->addrs[1], req->addrs[0], req->addrs[0], cmd);

    if (cmd == Command::Get) {
        me->setDstBaseAddr(req->addrs[0] & addrMask_);
    } else {
        me->setSrcBaseAddr(req->addrs[1] & addrMask_);
    }

    me->setRqstr(rqstr_);
    me->setSrc(rqstr_);
    me->setDst(dst_);
    me->setSize(req->size);

    me->setDstVirtualAddress(req->getVirtualAddress());
    me->setInstructionPointer(req->getInstructionPointer());

    me->setMemFlags(req->memFlags);

    return me;
}


void MemHierarchyScratchInterface::handleIncoming(SST::Event *ev){
    MemEventBase *me = static_cast<MemEventBase*>(ev);
    SimpleMem::Request *req = processIncoming(me);
    if(req) (*recvHandler_)(req);
    delete me;
}


SimpleMem::Request* MemHierarchyScratchInterface::processIncoming(MemEventBase *ev){
    SimpleMem::Request *req = NULL;
    Command cmd = ev->getCmd();
    SST::Event::id_type origID = ev->getResponseToID();
    
    std::map<SST::Event::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        requests_.erase(i);
        updateRequest(req, ev);
    }
    else{
        output.fatal(CALL_INFO, -1, "%s, Error: Received a response but unable to find matching request. Event: (%s)\n", getName().c_str(), ev->getVerboseString().c_str());
    }
    return req;
}


void MemHierarchyScratchInterface::updateRequest(SimpleMem::Request* req, MemEventBase *me) const{
    switch (me->getCmd()) {
        case Command::AckMove:
            req->cmd = SimpleMem::Request::ReadResp;
            break;
        case Command::GetSResp:
            req->cmd   = SimpleMem::Request::ReadResp;
            req->data = static_cast<MemEvent*>(me)->getPayload();
            req->size  = req->data.size();
            break;
        case Command::GetXResp:
            req->cmd   = SimpleMem::Request::WriteResp;
            break;
        default:
            output.fatal(CALL_INFO, -1, "Don't know how to deal with command %s\n", CommandString[(int)me->getCmd()]);
    }
   // Always update memFlags to faciliate mem->processor communication
    req->memFlags = me->getMemFlags();
    
}

bool MemHierarchyScratchInterface::initialize(const std::string &linkName, HandlerBase *handler) {
    recvHandler_ = handler;
    std::string port;
    if (isPortConnected("port")) {
        port = "port";
        rqstr_ = getName();
    } else {
        port = linkName;
    }
    
    if ( NULL == recvHandler_) link_ = configureLink(port);
    else                       link_ = configureLink(port, new Event::Handler<MemHierarchyScratchInterface>(this, &MemHierarchyScratchInterface::handleIncoming));

    return (link_ != NULL);
}
