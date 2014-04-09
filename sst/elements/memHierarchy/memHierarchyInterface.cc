// -*- mode: c++ -*-
// Copyright 2009-2013 Sandia Corporation. Under the terms
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


uint64_t MemHierarchyInterface::Request::main_id = 0;


MemHierarchyInterface::MemHierarchyInterface(SST::Component *comp, Params &params) :
    owner(comp), recvHandler(NULL), link(NULL)
{ }

void MemHierarchyInterface::initialize(const std::string &linkName, HandlerBase *handler)
{
    recvHandler = handler;
    if ( NULL == recvHandler ) {
        link = owner->configureLink(linkName);
    } else {
        link = owner->configureLink(linkName, new Event::Handler<MemHierarchyInterface>(this, &MemHierarchyInterface::handleIncoming));
    }
}


void MemHierarchyInterface::sendRequest(MemHierarchyInterface::Request *req)
{
    Interfaces::MemEvent *me = createMemEvent(req);
    requests[me->getID()] = req;
    link->send(me);
}


MemHierarchyInterface::Request* MemHierarchyInterface::recvResponse(void)
{
    SST::Event *ev = link->recv();
    if ( NULL != ev ) {
        Interfaces::MemEvent *me = static_cast<Interfaces::MemEvent*>(ev);
        Request *req = processIncoming(me);
        delete me;
        return req;
    }
    return NULL;
}




Interfaces::MemEvent* MemHierarchyInterface::createMemEvent(MemHierarchyInterface::Request *req) const
{
    Interfaces::Command cmd;
    switch ( req->cmd ) {
    case Request::Read:      cmd = Interfaces::GetS;     break;
    case Request::Write:     cmd = Interfaces::GetX;     break;
    case Request::ReadResp:  cmd = Interfaces::GetXResp; break;
    case Request::WriteResp: cmd = Interfaces::GetSResp; break;
    }
    Interfaces::MemEvent *me = new Interfaces::MemEvent(owner, req->addr, cmd);

    if ( req->flags & Request::F_UNCACHED )     me->setFlag(Interfaces::MemEvent::F_UNCACHED);
    if ( req->flags & Request::F_EXCLUSIVE )    me->setFlag(Interfaces::MemEvent::F_EXCLUSIVE);
    if ( req->flags & Request::F_LOCKED )       me->setFlag(Interfaces::MemEvent::F_LOCKED);

    return me;
}



/**
 * Convert any incoming events to updated Requests, and fire handler.
 */
void MemHierarchyInterface::handleIncoming(SST::Event *ev)
{
    Interfaces::MemEvent *me = static_cast<Interfaces::MemEvent*>(ev);
    Request *req = processIncoming(me);
    if ( req ) {
        (*recvHandler)(req);
    }
    delete me;
}


/**
 * Process MemEvents into updated Requests
 */
MemHierarchyInterface::Request* MemHierarchyInterface::processIncoming(Interfaces::MemEvent *ev)
{
    Request *req = NULL;
    Interfaces::MemEvent::id_type origID = ev->getResponseToID();
    if ( Interfaces::Inv == ev->getCmd() ) return NULL; //Ignore Invalidate request

    std::map<Interfaces::MemEvent::id_type, Request*>::iterator i = requests.find(origID);
    if ( i != requests.end() ) {
        req = i->second;
        requests.erase(i);
        updateRequest(req, ev);
    } else {
        //TODO
        fprintf(stderr, "Unable to find matching request\n");
    }
    return req;
}


/**
 * Update Request with results of MemEvent
 */
void MemHierarchyInterface::updateRequest(MemHierarchyInterface::Request* req, Interfaces::MemEvent *me) const
{
    switch (me->getCmd()) {
    case Interfaces::GetSResp:
        req->cmd = Request::ReadResp;
        req->data = me->getPayload();
        req->size = me->getSize();
        break;
    case Interfaces::GetXResp:
        req->cmd = Request::WriteResp;
        break;
    default:
        //TODO
        fprintf(stderr, "Don't know how to deal with command %s\n", Interfaces::CommandString[me->getCmd()]);
    }
}
