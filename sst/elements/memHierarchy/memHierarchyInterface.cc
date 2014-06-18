// -*- mode: c++ -*-
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
//

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "memHierarchyInterface.h"

#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


MemHierarchyInterface::MemHierarchyInterface(SST::Component *_comp, Params &_params) :
    SimpleMem(_comp, _params), owner_(_comp), recvHandler_(NULL), link_(NULL)
{ }


void MemHierarchyInterface::sendInitData(SimpleMem::Request *_req){
    MemEvent *me = createMemEvent(_req);
    link_->sendInitData(me);
}


void MemHierarchyInterface::sendRequest(SimpleMem::Request *_req){
    MemEvent *me = createMemEvent(_req);
    requests_[me->getID()] = _req;
    //fprintf(stderr, "Sending request.  Cmd = %s, Addr = %"PRIx64", respID = %"PRIx64"\n", CommandString[me->getCmd()], me->getAddr(), me->getID().first);
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


MemEvent* MemHierarchyInterface::createMemEvent(SimpleMem::Request *_req) const{
    Command cmd = NULLCMD;
    
    switch ( _req->cmd ) {
        case SimpleMem::Request::Read:      cmd = GetS;     break;
        case SimpleMem::Request::Write:     cmd = GetX;     break;
        case SimpleMem::Request::ReadResp:  cmd = GetXResp; break;
        case SimpleMem::Request::WriteResp: cmd = GetSResp; break;
    }
    
    MemEvent *me = new MemEvent(owner_, _req->addr, cmd);
    
    me->setGroupId(_req->groupId);
    me->setSize(_req->size);

    if (SimpleMem::Request::Write == _req->cmd)  me->setPayload(_req->data);

    if(_req->flags & SimpleMem::Request::F_UNCACHED)     me->setFlag(MemEvent::F_UNCACHED);
    if(_req->flags & SimpleMem::Request::F_LOCKED) {
        me->setFlag(MemEvent::F_LOCKED);
        if (_req->cmd == SimpleMem::Request::Read)
            me->setCmd(GetSEx);
    }
    if(_req->flags & SimpleMem::Request::F_LLSC){
        if (_req->cmd == SimpleMem::Request::Read)
            me->setLoadLink();
        else if(_req->cmd == SimpleMem::Request::Write)
            me->setStoreConditional();
    }

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
    
    BOOST_ASSERT_MSG(MemEvent::isResponse(cmd), "Interal Error: Invalidates should not be sent by MemHierarchy to CPU");

    std::map<MemEvent::id_type, SimpleMem::Request*>::iterator i = requests_.find(origID);
    if(i != requests_.end()){
        req = i->second;
        
        /*if(NACK == cmd || InV) {                       // We received a NACK.  Just re-issue it
            MemEvent *me = createMemEvent(req);
            requests_[me->getID()] = req;
            link_->send(me);
            requests_.erase(i);
            assert(0); assert(0);
            //return NULL;
        }
        */

        requests_.erase(i);
        updateRequest(req, _ev);
    }
    else{
        fprintf(stderr, "Unable to find matching request.  Cmd = %s, Addr = %"PRIx64", respID = %"PRIx64"\n", CommandString[_ev->getCmd()], _ev->getAddr(), _ev->getResponseToID().first); //TODO
        assert(0);
    }
    return req;
}


void MemHierarchyInterface::updateRequest(SimpleMem::Request* _req, MemEvent *_me) const{
    switch (_me->getCmd()) {
    case GetSResp:
        _req->cmd   = SimpleMem::Request::ReadResp;
        _req->data  = _me->getPayload();
        _req->size  = _me->getPayload().size();
        break;
    case GetXResp:
        _req->cmd   = SimpleMem::Request::WriteResp;
        break;
    default:
        fprintf(stderr, "Don't know how to deal with command %s\n", CommandString[_me->getCmd()]);
    }
}

bool MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *_handler){
    recvHandler_ = _handler;
    if ( NULL == recvHandler_) link_ = owner_->configureLink(linkName);
    else                       link_ = owner_->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));

    return (link_ != NULL);
}
