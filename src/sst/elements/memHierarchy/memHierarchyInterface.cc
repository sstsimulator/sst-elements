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


void MemHierarchyInterface::init(unsigned int phase) {
    if (!phase) {
        link_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::CPU, false, 0));
    }

    while (SST::Event * ev = link_->recvInitData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            // Pick up info for initializing MemEvents
            if (memEvent->getCmd() == Command::NULLCMD) {
//                output.output("%s received init event with cmd: %s, src: %s, type: %d, inclusive: %d, line size: %" PRIu64 "\n",
//                        this->getName().c_str(), CommandString[(int)memEvent->getCmd()], memEvent->getSrc().c_str(), (int)memEvent->getType(), memEvent->getInclusive(), memEvent->getLineSize());

                baseAddrMask_ = ~(memEvent->getLineSize() - 1);
                rqstr_ = memEvent->getSrc();
            }
        }
        delete ev;
    }
}


void MemHierarchyInterface::sendInitData(SimpleMem::Request *req){
    MemEventInit *me = new MemEventInit(getName(), Command::GetX, req->addrs[0], req->data);
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

    //output.output("MemHInterface. Created event. %s\n", me->getVerboseString().c_str());

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
        output.fatal(CALL_INFO, -1, "Unable to find matching request.  Cmd = %s, Addr = %" PRIx64 ", respID = %" PRIx64 "\n", CommandString[(int)ev->getCmd()], ev->getAddr(), ev->getResponseToID().first);
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
        if(me->success()) req->flags |= (SimpleMem::Request::F_LLSC_RESP);
        break;
        case Command::FlushLineResp:
        req->cmd = SimpleMem::Request::FlushLineResp;
        if (me->success()) req->flags |= (SimpleMem::Request::F_FLUSH_SUCCESS);
        break;
    default:
        fprintf(stderr, "Don't know how to deal with command %s\n", CommandString[(int)me->getCmd()]);
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
