// -*- mode: c++ -*-
// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
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
{ }


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
        case SimpleMem::Request::FlushCache:    cmd = FlushAll;     break;
        case SimpleMem::Request::FlushLineResp: cmd = FlushLineResp; break;
        case SimpleMem::Request::FlushCacheResp: cmd = FlushAllResp; break;
    }
    
    MemEvent *me = new MemEvent(owner_, req->addr, req->addr, cmd);
    
    me->setGroupId(req->groupId);
    me->setSize(req->size);

    if (SimpleMem::Request::Write == req->cmd)  me->setPayload(req->data);

    if(req->flags & SimpleMem::Request::F_NONCACHEABLE)
        me->setFlag(MemEvent::F_NONCACHEABLE);
    
    if(req->flags & SimpleMem::Request::F_LOCKED) {
        me->setFlag(MemEvent::F_LOCKED);
        if (req->cmd == SimpleMem::Request::Read)
            me->setCmd(GetSEx);
    }
    
    if(req->flags & SimpleMem::Request::F_LLSC){
        if (req->cmd == SimpleMem::Request::Read)
            me->setLoadLink();
        else if(req->cmd == SimpleMem::Request::Write)
            me->setStoreConditional();
    }

    me->setVirtualAddress(req->getVirtualAddress());
    me->setInstructionPointer(req->getInstructionPointer());

    me->setMemFlags(req->memFlags);

    //totalRequests_++;
    return me;
}


void MemHierarchyInterface::handleIncoming(SST::Event *_ev){
    MemEvent *me = static_cast<MemEvent*>(_ev);
    SimpleMem::Request *req = processIncoming(me);
    if(req) (*recvHandler_)(req);
    delete me;
}


SimpleMem::Request* MemHierarchyInterface::processIncoming(MemEvent *_ev){
    SimpleMem::Request *req = NULL;
    Command cmd = _ev->getCmd();
    MemEvent::id_type origID = _ev->getResponseToID();
    
    BOOST_ASSERT_MSG(MemEvent::isResponse(cmd), "Interal Error: Request Type event (eg GetS, GetX, etc) should not be sent by MemHierarchy to CPU. " \
    "Make sure you L1's cache 'high network port' is connected to the CPU, and the L1's 'low network port' is connected to the next level cache.");

    std::map<MemEvent::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        requests_.erase(i);
        updateRequest(req, _ev);
    }
    else{
        fprintf(stderr, "Unable to find matching request.  Cmd = %s, Addr = %" PRIx64 ", respID = %" PRIx64 "\n", CommandString[_ev->getCmd()], _ev->getAddr(), _ev->getResponseToID().first); //TODO
        assert(0);
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
        if(me->isAtomic()) req->flags |= (SimpleMem::Request::F_LLSC_RESP);
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

bool MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *_handler){
    recvHandler_ = _handler;
    if ( NULL == recvHandler_) link_ = owner_->configureLink(linkName);
    else                       link_ = owner_->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));

    return (link_ != NULL);
}
