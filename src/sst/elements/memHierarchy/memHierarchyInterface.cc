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
#include "memHierarchyInterface.h"

#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


MemHierarchyInterface::MemHierarchyInterface(SST::Component *comp, Params &params) :
    SimpleMem(comp, params), owner_(comp), recvHandler_(NULL), link_(NULL)
{ 
    output.init("", 1, 0, Output::STDOUT);
    rqstr_ = "";
    initDone_ = false;
}


void MemHierarchyInterface::init(unsigned int phase) {
    /* Send region message */
    if (!phase) {
        MemRegion region;
        region.start = 0;
        region.end = (uint64_t) - 1;
        region.interleaveStep = 0;
        region.interleaveSize = 0;
        link_->sendInitData(new MemEventInitRegion(parent->getName(), region, false));

        MemEventInitCoherence * event = new MemEventInitCoherence(parent->getName(), Endpoint::CPU, false, false, 0, false);
        link_->sendInitData(event);

    }

    while (SST::Event * ev = link_->recvInitData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            if (memEvent->getCmd() == Command::NULLCMD) {
                rqstr_ = memEvent->getSrc();
                if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                    MemEventInitCoherence * memEventC = static_cast<MemEventInitCoherence*>(memEvent);
                    baseAddrMask_ = ~(memEventC->getLineSize() - 1);
                    initDone_ = true;
                }
            }
        }
        delete ev;
    }

    if (initDone_) { // Drain send queue
        while (!initSendQueue_.empty()) {
            link_->sendInitData(initSendQueue_.front());
            initSendQueue_.pop();
        }
    }
    
}

void MemHierarchyInterface::sendInitData(SimpleMem::Request *req){
    MemEventInit *me = new MemEventInit(getName(), Command::GetX, req->addrs[0], req->data);
    if (initDone_)
        link_->sendInitData(me);
    else
        initSendQueue_.push(me);
}


void MemHierarchyInterface::sendRequest(SimpleMem::Request *req){
    MemEventBase *me;
    if (req->cmd == SimpleMem::Request::CustomCmd) {
        me = createCustomEvent(req);
    } else {
        me = createMemEvent(req);
    }
    requests_[me->getID()] = req;
    link_->send(me);
}


SimpleMem::Request* MemHierarchyInterface::recvResponse(void){
    SST::Event *ev = link_->recv();
    if (NULL != ev) {
        MemEventBase *me = static_cast<MemEventBase*>(ev);
        Request *req = processIncoming(me);
        delete me;
        return req;
    }
    return NULL;
}


MemEventBase* MemHierarchyInterface::createMemEvent(SimpleMem::Request *req) const{
    Command cmd = Command::NULLCMD;
    
    switch ( req->cmd ) {
        case SimpleMem::Request::Read:          cmd = Command::GetS;         break;
        case SimpleMem::Request::Write:         cmd = Command::GetX;         break;
        case SimpleMem::Request::ReadResp:      cmd = Command::GetXResp;     break;
        case SimpleMem::Request::WriteResp:     cmd = Command::GetSResp;     break;
        case SimpleMem::Request::FlushLine:     cmd = Command::FlushLine;    break;
        case SimpleMem::Request::FlushLineInv:  cmd = Command::FlushLineInv; break;
        case SimpleMem::Request::FlushLineResp: cmd = Command::FlushLineResp; break;
        default: output.fatal(CALL_INFO, -1, "Unknown req->cmd in createMemEvent()\n");
    }

    Addr baseAddr = (req->addrs[0]) & baseAddrMask_;
    
    MemEvent *me = new MemEvent(owner_, req->addrs[0], baseAddr, cmd);
    
    me->setRqstr(rqstr_);
    me->setDst(rqstr_);
    me->setSize(req->size);

    if (SimpleMem::Request::Write == req->cmd)  {
        if (req->data.size() == 0) {
            req->data.resize(req->size, 0);    
        }
        if (req->data.size() != req->size) 
            output.output("Warning: In memHierarchyInterface, write request size does not match payload size. Request size: %zu. Payload size: %zu. MemEvent will use payload size\n", req->size, req->data.size());

        me->setPayload(req->data);
    }

    if(req->flags & SimpleMem::Request::F_NONCACHEABLE)
        me->setFlag(MemEvent::F_NONCACHEABLE);
    
    if(req->flags & SimpleMem::Request::F_LOCKED) {
        me->setFlag(MemEvent::F_LOCKED);
        if (req->cmd == SimpleMem::Request::Read)
            me->setCmd(Command::GetSX);
    }
    
    if(req->flags & SimpleMem::Request::F_LLSC){
        me->setFlag(MemEvent::F_LLSC);
    }

    me->setVirtualAddress(req->getVirtualAddress());
    me->setInstructionPointer(req->getInstructionPointer());

    me->setMemFlags(req->memFlags);

    return me;
}


MemEventBase* MemHierarchyInterface::createCustomEvent(SimpleMem::Request * req) const {
    Addr baseAddr = (req->addrs[0]) & baseAddrMask_;
    CustomCmdEvent * cme = new CustomCmdEvent(getName().c_str(), req->addrs[0], baseAddr, Command::CustomReq, req->getCustomOpc(), req->size);
    cme->setRqstr(rqstr_);
    cme->setDst(rqstr_);

    if(req->flags & SimpleMem::Request::F_NONCACHEABLE)
        cme->setFlag(MemEvent::F_NONCACHEABLE);
    
    if (req->data.size() != 0) {
        cme->setPayload(req->data); // Note this updates cme->size to payload.size()...
        cme->setSize(req->size);    // Assume this is what we want, not the copied payload size
    }
    cme->setVirtualAddress(req->getVirtualAddress());
    cme->setInstructionPointer(req->getInstructionPointer());

    cme->setMemFlags(req->memFlags);
    
    return cme;
}

/* Handle (response) events from memHierarchy 
 *  Update original request
 *  Call owner's callback
 */
void MemHierarchyInterface::handleIncoming(SST::Event *ev){
    MemEventBase *me = static_cast<MemEventBase*>(ev);
    SimpleMem::Request *req = processIncoming(me);
    if (req) (*recvHandler_)(req);
    delete me;
}

/* Match response to request. Update request with results. Return request to processor */
SimpleMem::Request* MemHierarchyInterface::processIncoming(MemEventBase *ev){
    SimpleMem::Request *req = NULL;
    Command cmd = ev->getCmd();
    MemEventBase::id_type origID = ev->getResponseToID();
    
    std::map<MemEventBase::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        requests_.erase(i);
        if (req->cmd == SimpleMem::Request::CustomCmd) {
            updateCustomRequest(req, ev);
        } else {
            updateRequest(req, static_cast<MemEvent*>(ev));
        }
    } else {
        output.fatal(CALL_INFO, -1, "(%s interface) Unable to find matching request. Event: %s\n", owner_->getName().c_str(), ev->getVerboseString().c_str());
    }
    return req;
}


void MemHierarchyInterface::updateRequest(SimpleMem::Request* req, MemEvent *me) const{
    switch (me->getCmd()) {
        case Command::GetSResp:
            req->cmd   = SimpleMem::Request::ReadResp;
            req->data  = me->getPayload();
            req->size  = me->getPayload().size();
            break;
        case Command::GetXResp:
            req->cmd   = SimpleMem::Request::WriteResp;
            if (me->success()) 
                req->flags |= (SimpleMem::Request::F_LLSC_RESP);
            break;
        case Command::FlushLineResp:
            req->cmd = SimpleMem::Request::FlushLineResp;
            if (me->success()) 
                req->flags |= (SimpleMem::Request::F_FLUSH_SUCCESS);
            break;
    default:
        output.fatal(CALL_INFO, -1, "Don't know how to deal with command %s\n", CommandString[(int)me->getCmd()]);
    }
   // Always update memFlags to faciliate mem->processor communication
    req->memFlags = me->getMemFlags();
    
}


void MemHierarchyInterface::updateCustomRequest(SimpleMem::Request* req, MemEventBase *ev) const{
    CustomCmdEvent* cev = static_cast<CustomCmdEvent*>(ev);
    req->cmd = SimpleMem::Request::CustomCmd;
    req->memFlags = cev->getMemFlags();
    req->data = cev->getPayload();
}

bool MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *handler){
    recvHandler_ = handler;
    if ( NULL == recvHandler_) link_ = owner_->configureLink(linkName);
    else                       link_ = owner_->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));

    return (link_ != NULL);
}
