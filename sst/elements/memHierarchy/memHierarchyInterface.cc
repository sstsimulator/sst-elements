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


MemHierarchyInterface::MemHierarchyInterface(SST::Component *comp, Params &params) :
    SimpleMem(comp, params), owner(comp), recvHandler(NULL), link(NULL)
{ }


void MemHierarchyInterface::sendInitData(SimpleMem::Request *req){
    MemEvent *me = createMemEvent(req);
    link->sendInitData(me);
}


void MemHierarchyInterface::sendRequest(SimpleMem::Request *req){
    MemEvent *me = createMemEvent(req);
    requests[me->getID()] = req;
    //fprintf(stderr, "Sending request.  Cmd = %s, Addr = %"PRIx64", respID = %"PRIx64"\n", CommandString[me->getCmd()], me->getAddr(), me->getID().first);
    link->send(me);
}


SimpleMem::Request* MemHierarchyInterface::recvResponse(void){
    SST::Event *ev = link->recv();
    if (NULL != ev) {
        MemEvent *me = static_cast<MemEvent*>(ev);
        SimpleMem::Request *req = processIncoming(me);
        delete me;
        return req;
    }
    return NULL;
}


MemEvent* MemHierarchyInterface::createMemEvent(SimpleMem::Request *req) const{
    Command cmd = NULLCMD;
    
    switch ( req->cmd ) {
    case SimpleMem::Request::Read:      cmd = GetS;     break;
    case SimpleMem::Request::Write:     cmd = GetX;     break;
    case SimpleMem::Request::ReadResp:  cmd = GetXResp; break;
    case SimpleMem::Request::WriteResp: cmd = GetSResp; break;
    }
    
    MemEvent *me = new MemEvent(owner, req->addr, cmd);
    me->setSize(req->size);

    if (SimpleMem::Request::Write == req->cmd)  me->setPayload(req->data);

    if (req->flags & SimpleMem::Request::F_UNCACHED)     me->setFlag(MemEvent::F_UNCACHED);
    if (req->flags & SimpleMem::Request::F_LOCKED) {
        me->setFlag(MemEvent::F_LOCKED);
        if (req->cmd == SimpleMem::Request::Read)
            me->setCmd(GetSEx);
    }

    return me;
}


void MemHierarchyInterface::handleIncoming(SST::Event *ev){
    MemEvent *me = static_cast<MemEvent*>(ev);
    SimpleMem::Request *req = processIncoming(me);
    if (req) (*recvHandler)(req);
    delete me;
}


SimpleMem::Request* MemHierarchyInterface::processIncoming(MemEvent *ev){
    SimpleMem::Request *req = NULL;
    MemEvent::id_type origID = ev->getResponseToID();
    BOOST_ASSERT_MSG(Inv != ev->getCmd(), "Interal Error: Invalidates should not be sent by MemHierarchy to CPU");

    std::map<MemEvent::id_type, SimpleMem::Request*>::iterator i = requests.find(origID);
    if(i != requests.end()){
        req = i->second;
        
        if ( NACK == ev->getCmd() ) {                       /* We received a NACK.  Just re-issue it */
            MemEvent *me = createMemEvent(req);
            requests[me->getID()] = req;
            link->send(me);
            return NULL;
        }

        requests.erase(i);
        updateRequest(req, ev);
    }
    else{
        fprintf(stderr, "Unable to find matching request.  Cmd = %s, Addr = %"PRIx64", respID = %"PRIx64"\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getResponseToID().first); //TODO
        std::cout << std::flush;
        std::cout << std::flush;

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
        break;
    default:
        fprintf(stderr, "Don't know how to deal with command %s\n", CommandString[me->getCmd()]);         //TODO
    }
}

bool MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *handler){
    recvHandler = handler;
    if ( NULL == recvHandler ) {
        link = owner->configureLink(linkName);
    } else {
        link = owner->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));
    }
    return (link != NULL);
}
