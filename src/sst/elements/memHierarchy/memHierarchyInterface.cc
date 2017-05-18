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
#include "memHierarchyInterface.h"

#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


MemHierarchyInterface::MemHierarchyInterface(SST::Component *_comp, Params &_params) :
    SimpleMem(_comp, _params), owner_(_comp), recvHandler_(NULL), link_(NULL)
{ 
    output.init("", 1, 0, Output::STDOUT);
}


void MemHierarchyInterface::sendInitData(SimpleMem::Request *req){
    MemEvent *me = createMemEvent(req);
    link_->sendInitData(me);
}


void MemHierarchyInterface::sendRequest(SimpleMem::Request *req){
    MemEvent *me = createMemEvent(req);
    requests_[me->getID()] = req;
    link_->send(me);
}


SimpleMem::Request* MemHierarchyInterface::recvResponse(void){
    SST::Event *ev = link_->recv();
    if (NULL != ev) {
        MemEvent *me = static_cast<MemEvent*>(ev);
        Request *req = processIncoming(me);
        delete me;
        return req;
    }
    return NULL;
}


MemEvent* MemHierarchyInterface::createMemEvent(SimpleMem::Request *req) const{
    Command cmd = NULLCMD;
    
    switch ( req->cmd ) {
        case SimpleMem::Request::Read:          cmd = GetS;         break;
        case SimpleMem::Request::Write:         cmd = GetX;         break;
        case SimpleMem::Request::ReadResp:      cmd = GetXResp;     break;
        case SimpleMem::Request::WriteResp:     cmd = GetSResp;     break;
        case SimpleMem::Request::FlushLine:     cmd = FlushLine;    break;
        case SimpleMem::Request::FlushLineInv:  cmd = FlushLineInv; break;
        case SimpleMem::Request::FlushLineResp: cmd = FlushLineResp; break;
        default: output.fatal(CALL_INFO, -1, "Unknown req->cmd in createMemEvent()\n");
    }
    
    MemEvent *me = new MemEvent(owner_, req->addrs[0], req->addrs[0], cmd);
    
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
            me->setCmd(GetSEx);
    }
    
    if(req->flags & SimpleMem::Request::F_LLSC){
        me->setFlag(MemEvent::F_LLSC);
    }

    me->setVirtualAddress(req->getVirtualAddress());
    me->setInstructionPointer(req->getInstructionPointer());

    me->setMemFlags(req->memFlags);

    //totalRequests_++;
    return me;
}


void MemHierarchyInterface::handleIncoming(SST::Event *ev){
    MemEvent *me = static_cast<MemEvent*>(ev);
    SimpleMem::Request *req = processIncoming(me);
    if(req) (*recvHandler_)(req);
    delete me;
}


SimpleMem::Request* MemHierarchyInterface::processIncoming(MemEvent *ev){
    SimpleMem::Request *req = NULL;
    Command cmd = ev->getCmd();
    MemEvent::id_type origID = ev->getResponseToID();
    
    std::map<MemEvent::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        requests_.erase(i);
        updateRequest(req, ev);
    }
    else{
        output.fatal(CALL_INFO, -1, "Unable to find matching request.  Cmd = %s, Addr = %" PRIx64 ", respID = %" PRIx64 "\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getResponseToID().first);
    }
    return req;
}


void MemHierarchyInterface::updateRequest(SimpleMem::Request* req, MemEvent *me) const{
    switch (me->getCmd()) {
    case GetSResp:
        req->cmd   = SimpleMem::Request::ReadResp;
        req->data  = me->getPayload();
        req->size  = me->getPayload().size();
        break;
    case GetXResp:
        req->cmd   = SimpleMem::Request::WriteResp;
        if(me->success()) req->flags |= (SimpleMem::Request::F_LLSC_RESP);
        break;
    case FlushLineResp:
        req->cmd = SimpleMem::Request::FlushLineResp;
        if (me->success()) req->flags |= (SimpleMem::Request::F_FLUSH_SUCCESS);
        break;
    default:
        fprintf(stderr, "Don't know how to deal with command %s\n", CommandString[me->getCmd()]);
    }
   // Always update memFlags to faciliate mem->processor communication
    req->memFlags = me->getMemFlags();
    
}

bool MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *handler){
    recvHandler_ = handler;
    if ( NULL == recvHandler_) link_ = owner_->configureLink(linkName);
    else                       link_ = owner_->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));

    return (link_ != NULL);
}
