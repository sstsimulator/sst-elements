// -*- mode: c++ -*-
// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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


MemHierarchyScratchInterface::MemHierarchyScratchInterface(SST::Component *comp, Params &params) :
    SimpleMem(comp, params), owner_(comp), recvHandler_(NULL), link_(NULL)
{ 
    output.init("", 1, 0, Output::STDOUT); 
}


void MemHierarchyScratchInterface::sendInitData(SimpleMem::Request *req){
    ScratchEvent * event = createScratchEvent(req);
    link_->sendInitData(event);
}


void MemHierarchyScratchInterface::sendRequest(SimpleMem::Request *req){
    ScratchEvent *event = createScratchEvent(req);
    requests_[event->getID()] = req;
    link_->send(event);
}


SimpleMem::Request* MemHierarchyScratchInterface::recvResponse(void){
    SST::Event *ev = link_->recv();
    if (NULL != ev) {
        ScratchEvent *event = static_cast<ScratchEvent*>(ev);
        Request *req = processIncoming(event);
        delete event;
        return req;
    }
    return NULL;
}


ScratchEvent* MemHierarchyScratchInterface::createScratchEvent(SimpleMem::Request *req) const{
    ScratchCommand cmd = ScratchNullCmd;
    
    switch ( req->cmd ) {
        case SimpleMem::Request::Read:          
            if (req->addrs.size() == 1) cmd = Read;
            else cmd = ScratchGet;
            break;
        case SimpleMem::Request::Write:
            if (req->addrs.size() == 1) cmd = Write;
            else cmd = ScratchPut;
            break;
        case SimpleMem::Request::ReadResp:      cmd = ReadResp;     break;
        case SimpleMem::Request::WriteResp:     cmd = WriteResp;    break;
        default: output.fatal(CALL_INFO, -1, "Unknown req->cmd in createScratchEvent()\n");
    }
   
    ScratchEvent *me = new ScratchEvent(parent->getName(), req->addrs[0], req->addrs[0], cmd);

    me->setSize(req->size);

    if (cmd == Write) me->setPayload(req->data);

    if (cmd == ScratchGet || cmd == ScratchPut) {
        me->setSrcAddr(req->addrs[1]);
    }

    me->setVirtualAddress(req->getVirtualAddress());
    me->setInstructionPointer(req->getInstructionPointer());

    me->setMemFlags(req->memFlags);

    return me;
}


void MemHierarchyScratchInterface::handleIncoming(SST::Event *ev){
    ScratchEvent *me = static_cast<ScratchEvent*>(ev);
    SimpleMem::Request *req = processIncoming(me);
    if(req) (*recvHandler_)(req);
    delete me;
}


SimpleMem::Request* MemHierarchyScratchInterface::processIncoming(ScratchEvent *ev){
    SimpleMem::Request *req = NULL;
    ScratchCommand cmd = ev->getCmd();
    SST::Event::id_type origID = ev->getRespID();
    
    std::map<ScratchEvent::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        requests_.erase(i);
        updateRequest(req, ev);
    }
    else{
        output.fatal(CALL_INFO, -1, "Unable to find matching request.  Cmd = %s, Addr = %" PRIx64 ", respID = %" PRIx64 "\n", ScratchCommandString[ev->getCmd()], ev->getAddr(), ev->getRespID().first);
    }
    return req;
}


void MemHierarchyScratchInterface::updateRequest(SimpleMem::Request* req, ScratchEvent *me) const{
    switch (me->getCmd()) {
    case ReadResp:
        req->cmd   = SimpleMem::Request::ReadResp;
        if (!(me->getPayload().empty())) { // Payload empty for ScratchGet requests
            req->data = me->getPayload();
            req->size  = me->getPayload().size();
        }
        break;
    case WriteResp:
        req->cmd   = SimpleMem::Request::WriteResp;
        break;
    default:
        output.fatal(CALL_INFO, -1, "Don't know how to deal with command %s\n", ScratchCommandString[me->getCmd()]);
    }
   // Always update memFlags to faciliate mem->processor communication
    req->memFlags = me->getMemFlags();
    
}

bool MemHierarchyScratchInterface::initialize(const std::string &linkName, HandlerBase *handler){
    recvHandler_ = handler;
    if ( NULL == recvHandler_) link_ = owner_->configureLink(linkName);
    else                       link_ = owner_->configureLink(linkName, new Event::Handler<MemHierarchyScratchInterface>(this, &MemHierarchyScratchInterface::handleIncoming));

    return (link_ != NULL);
}
