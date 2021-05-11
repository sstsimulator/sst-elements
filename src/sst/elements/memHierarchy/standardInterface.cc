// -*- mode: c++ -*-
// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "standardInterface.h"

#include <sst/core/component.h>
#include <sst/core/link.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memEvent.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Experimental::Interfaces;

StandardInterface::StandardInterface(SST::ComponentId_t id, Params &params, TimeConverter * time, HandlerBase* handler) :
    StandardMem(id, params, time, handler)
{
    setDefaultTimeBase(time); // Links are required to have a timebase

    // Output object for warnings/debug/etc.
    output.init("", 1, 0, Output::STDOUT);
    rqstr_ = "";
    initDone_ = false;

    converter_ = new StandardInterface::MemEventConverter(this);
    converter_->output = output;
    converter_->name = getName();
    converter_->baseAddrMask_ = baseAddrMask_;
    converter_->rqstr_ = rqstr_;

    // Handler - if nullptr then polling will be assumed
    recvHandler_ = handler;

    link_ = loadUserSubComponent<MemLinkBase>("memlink", ComponentInfo::SHARE_NONE, getDefaultTimeBase());
    if (!link_) {
        // Default is a regular non-network link on port 'port'
        Params lparams;
        lparams.insert("port", "port");
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "link", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, lparams, getDefaultTimeBase());
    }

/*
    if ( NULL == recvHandler_)
        link_ = configureLink("port");
    else
        link_ = configureLink("port", new Event::Handler<StandardInterface>(this, &StandardInterface::receive));
*/
    if (!link_)
        output.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port 'port'\n", getName().c_str());
    
    link_->setRecvHandler( new SST::Event::Handler<StandardInterface>(this, &StandardInterface::receive));
    link_->setName(getName());

    /* Set region to default (all addresses) */
    region.start = 0;
    region.end = (uint64_t) - 1;
    region.interleaveStep = 0;
    region.interleaveSize = 0;
    epType = Endpoint::CPU;
    link_->setRegion(region);
}

void StandardInterface::setMemoryMappedAddressRegion(Addr start, Addr size) {
    region.start = start;
    region.end = start + size - 1;
    region.interleaveStep = 0;
    region.interleaveSize = 0;
    epType = Endpoint::MMIO;
    link_->setRegion(region);
}

void StandardInterface::init(unsigned int phase) {
    link_->init(phase);
    /* Send region message */
    if (!phase) {
        
        /* Broadcast our name, type, and coherence configuration parameters on link */
        MemEventInitCoherence * event = new MemEventInitCoherence(getName(), epType, false, false, 0, false);
        link_->sendInitData(event);
    }

    while (SST::Event * ev = link_->recvInitData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            if (memEvent->getCmd() == Command::NULLCMD) {
                rqstr_ = memEvent->getSrc();
                converter_->rqstr_ = rqstr_;
                if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                    MemEventInitCoherence * memEventC = static_cast<MemEventInitCoherence*>(memEvent);
                    baseAddrMask_ = ~(memEventC->getLineSize() - 1);
                    converter_->baseAddrMask_ = baseAddrMask_;
                    lineSize_ = memEventC->getLineSize();
                    if (memEventC->getType() == Endpoint::Cache)
                        cacheDst_ = true; // Cache takes care of figuring out whether GetXResp is read or write response and other address-related issues
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

void StandardInterface::setup() { }

void StandardInterface::finish() { }

/* Writes are allowed during init() but nothing else */
void StandardInterface::sendUntimedData(StandardMem::Request *req) {
#ifdef __SST_DEBUG_OUTPUT__
    StandardMem::Write* wr = dynamic_cast<StandardMem::Write*>(req);
    if (!wr)
        output.fatal(CALL_INFO, -1, "%s, Error: sendUntimedData(Request*) only accepts Write requests\n", getName().c_str());
#else
    StandardMem::Write* wr = static_cast<StandardMem::Write*>(req);
#endif

    MemEventInit *me = new MemEventInit(getName(), Command::GetX, wr->pAddr, wr->data);
    if (initDone_)
        link_->sendInitData(me);
    else
        initSendQueue_.push(me);
}

StandardMem::Request* StandardInterface::recvUntimedData() {
    return nullptr; // TODO FIGURE THIS OUT    
}

/* This could be a request or a response. */
void StandardInterface::send(StandardMem::Request* req) {
    MemEventBase *me = static_cast<MemEventBase*>(req->convert(converter_));
    if (req->needsResponse())
        requests_[me->getID()] = std::make_pair(req,me->getCmd());   /* Save this request so we can use it when a response is returned */
    else
        delete req;
    link_->send(me);
}

StandardMem::Request* StandardInterface::poll() {
    // TODO FIX THIS
    return nullptr;
}

void StandardInterface::receive(SST::Event* ev) {
    MemEventBase *me = static_cast<MemEventBase*>(ev);
    StandardMem::Request* deliverReq = nullptr;
    Command cmd = me->getCmd();
    bool isResponse = (BasicCommandClassArr[(int)cmd] == BasicCommandClass::Response);

    /* Handle responses to requests we sent */
    if (isResponse) {
        MemEventBase::id_type origID = me->getResponseToID();
        std::map<MemEventBase::id_type,std::pair<StandardMem::Request*,Command>>::iterator reqit = requests_.find(origID);
        if (reqit == requests_.end()) {
            output.fatal(CALL_INFO, -1, "%s, Error: Received response but cannot locate matching request. Response: %s\n",
                getName().c_str(), me->getVerboseString().c_str());
        }
        StandardMem::Request* origReq = reqit->second.first;
        Command origCmd = reqit->second.second;
        if (origCmd == Command::GetS || origCmd == Command::GetSX)
            cmd = Command::GetSResp;
        requests_.erase(reqit);
        response = me;
        switch (cmd) {
            case Command::GetSResp:
                deliverReq = convertResponseGetSResp(origReq, me);
                break;
            case Command::GetXResp:
                deliverReq = convertResponseGetXResp(origReq, me);
                break;
            case Command::FlushLineResp:
                deliverReq = convertResponseFlushResp(origReq, me);
                break;
            default:
                output.fatal(CALL_INFO, -1, "%s, Error: Received response with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString().c_str());
        };
        delete me;
        delete origReq;
    
    /* Handle incoming requests */
    } else {
        switch (cmd) {
            case Command::Inv:
                deliverReq = convertRequestInv(me);
                break;
            case Command::GetS:
                if (me->queryFlag(MemEventBase::F_LLSC))
                    deliverReq = convertRequestLL(me);
                else if (me->queryFlag(MemEventBase::F_LOCKED)) {
                    deliverReq = convertRequestLock(me);
                } else {
                    deliverReq = convertRequestGetS(me);
                }
                break;
            case Command::GetSX: // Read for ownership
                if (me->queryFlag(MemEventBase::F_LLSC))
                    deliverReq = convertRequestLL(me);
                else if (me->queryFlag(MemEventBase::F_LOCKED)) {
                    deliverReq = convertRequestLock(me);
                } else {
                    deliverReq = convertRequestGetS(me);
                }
                break;
            case Command::GetX:
                if (me->queryFlag(MemEventBase::F_LLSC))
                    deliverReq = convertRequestSC(me);
                else if (me->queryFlag(MemEventBase::F_LOCKED)) {
                    deliverReq = convertRequestUnlock(me);
                } else {
                    deliverReq = convertRequestGetX(me);
                }
                break;
            case Command::CustomReq:
                deliverReq = convertRequestCustom(me);
                break;
            default:
                output.fatal(CALL_INFO, -1, "%s, Error: Received request with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString().c_str());
        };
        if (deliverReq->needsResponse()) /* Endpoint will need to send a response to this */
            responses_[deliverReq->getID()] = me;
        else 
            delete me;
    }
    (*recvHandler_)(deliverReq);
}

/********************************************************************************************
 * Conversion methods: StandardMem::Request -> MemEventBase
 ********************************************************************************************/

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::Read* req) {
    Addr bAddr = req->pAddr & iface->baseAddrMask_; // Line address
    MemEvent* read = new MemEvent(getName(), req->pAddr, bAddr, Command::GetS, req->size);
    read->setRqstr(iface->rqstr_);
    read->setDst(iface->rqstr_);
    read->setVirtualAddress(req->vAddr);
    read->setInstructionPointer(req->iPtr);
    if (req->getNoncacheable())
        read->setFlag(MemEvent::F_NONCACHEABLE);
#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(read);
#endif
    return read;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::Write* req) {
    Addr bAddr = req->pAddr & iface->baseAddrMask_; // Line address
    MemEvent* write = new MemEvent(getName(), req->pAddr, bAddr, Command::GetX, req->data);
    
    write->setRqstr(iface->rqstr_);
    write->setDst(iface->rqstr_);
    write->setVirtualAddress(req->vAddr);
    write->setInstructionPointer(req->iPtr);
    
    if (req->getNoncacheable())
        write->setFlag(MemEvent::F_NONCACHEABLE);
    if (req->posted)
        write->setFlag(MemEvent::F_NORESPONSE);
    
    if (req->data.size() == 0) { // Endpoint isn't using real data values & didn't give a dummy payload
        write->setZeroPayload(req->size);
    }
#ifdef __SST_DEBUG_OUTPUT__
    else if (req->data.size() != req->size) {
        output.output("Warning (%s): Write request size is %zu and payload size is %zu. MemEvent will use payload size.\n",
            iface->getName().c_str(), req->size, req->data.size());
    } 
    debugChecks(write);
#endif
    return write;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushAddr* req) {
    Addr bAddr = req->pAddr & iface->baseAddrMask_; // Line address
    Command cmd = req->inv ? Command::FlushLineInv : Command::FlushLine;

    MemEvent* flush = new MemEvent(getName(), req->pAddr, bAddr, cmd, req->size);
    flush->setRqstr(iface->rqstr_);
    flush->setDst(iface->rqstr_);
    flush->setVirtualAddress(req->vAddr);
    flush->setInstructionPointer(req->iPtr);
#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(flush);
    if (req->getNoncacheable())
        iface->output.fatal(CALL_INFO, -1, "%s, Error: Received noncacheable flush request. This combination is not supported.\n",
            iface->getName().c_str());
#endif
    return flush;
}

Event* StandardInterface::MemEventConverter::convert(StandardMem::ReadLock* req) {
    Addr bAddr = req->pAddr & baseAddrMask_; // Line address
    MemEvent* read = new MemEvent(getName(), req->pAddr, bAddr, Command::GetSX, req->size);
    read->setRqstr(rqstr_);
    read->setDst(rqstr_);
    read->setVirtualAddress(req->vAddr);
    read->setInstructionPointer(req->iPtr);
    read->setFlag(MemEvent::F_LOCKED);
    if (req->getNoncacheable())
        read->setFlag(MemEvent::F_NONCACHEABLE);

#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(read);
#endif
    return read;
}
SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::WriteUnlock* req) {
    Addr bAddr = req->pAddr & baseAddrMask_; // Line address
    MemEvent* write = new MemEvent(getName(), req->pAddr, bAddr, Command::GetX, req->data);
    write->setRqstr(rqstr_);
    write->setDst(rqstr_);
    write->setVirtualAddress(req->vAddr);
    write->setInstructionPointer(req->iPtr);
    write->setFlag(MemEvent::F_LOCKED);
    if (req->data.size() == 0) { // Endpoint isn't using real data values & didn't give a dummy payload
        write->setZeroPayload(req->size);
    }
    if (req->getNoncacheable())
        write->setFlag(MemEvent::F_NONCACHEABLE);
#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(write);
#endif
    return write;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::LoadLink* req) {
    Addr bAddr = req->pAddr & baseAddrMask_; // Line address
    MemEvent* load = new MemEvent(getName(), req->pAddr, bAddr, Command::GetS, req->size);
    load->setFlag(MemEvent::F_LLSC);
    load->setRqstr(rqstr_);
    load->setDst(rqstr_);
    load->setVirtualAddress(req->vAddr);
    load->setInstructionPointer(req->iPtr);
    if (req->getNoncacheable())
        load->setFlag(MemEvent::F_NONCACHEABLE);
#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(load);
#endif
    return load;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::StoreConditional* req) {
    Addr bAddr = req->pAddr & baseAddrMask_; // Line address
    MemEvent* store = new MemEvent(getName(), req->pAddr, bAddr, Command::GetX, req->data);
    store->setFlag(MemEvent::F_LLSC);
    store->setRqstr(rqstr_);
    store->setDst(rqstr_);
    store->setVirtualAddress(req->vAddr);
    store->setInstructionPointer(req->iPtr);
    
    if (req->data.size() == 0) { // Endpoint isn't using real data values & didn't give a dummy payload
        store->setZeroPayload(req->size);
    }
    if (req->getNoncacheable())
        store->setFlag(MemEvent::F_NONCACHEABLE);

#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(store);
#endif
    return store;
}


Event* StandardInterface::MemEventConverter::convert(StandardMem::MoveData* req) {
    Addr bAddr = req->pSrc & baseAddrMask_; // Line address
    MemEvent* move = nullptr;
    output.fatal(CALL_INFO, -1, "%s, Error: MoveData converter not implemented\n", getName().c_str());
#ifdef __SST_DEBUG_OUTPUT__
    //debugChecks(move);
#endif
    return move;
}
Event* StandardInterface::MemEventConverter::convert(StandardMem::CustomReq* req) {
    Addr bAddr = req->data->getRoutingAddress() & baseAddrMask_;
    MemEvent* creq = nullptr;
    output.fatal(CALL_INFO, -1, "%s, Error: CustomReq converter not implemented\n", getName().c_str());
#ifdef __SST_DEBUG_OUTPUT__
    //debugChecks(creq);
#endif
    return creq;
}

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::ReadResp* resp) { 
    std::map<StandardMem::Request::id_t, MemEventBase*>::iterator it = iface->responses_.find(resp->getID());
    if (it == iface->responses_.end())
        iface->output.fatal(CALL_INFO, -1, "%s, Error: Handling a WriteResp but no matching Write found\n", iface->getName().c_str());
    MemEvent* mereq = static_cast<MemEvent*>(it->second); // Matching memEvent req
    iface->responses_.erase(it);
    MemEvent* meresp = mereq->makeResponse();
    meresp->setPayload(resp->data);
    if (resp->getSuccess()) {
        meresp->setFlag(MemEventBase::F_SUCCESS);
    }
    return meresp;
}
SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::WriteResp* resp) {
    std::map<StandardMem::Request::id_t, MemEventBase*>::iterator it = iface->responses_.find(resp->getID());
    if (it == iface->responses_.end())
        iface->output.fatal(CALL_INFO, -1, "%s, Error: Handling a WriteResp but no matching Write found\n", iface->getName().c_str());
    MemEvent* mereq = static_cast<MemEvent*>(it->second); // Matching memEvent req
    iface->responses_.erase(it);
    MemEvent* meresp = mereq->makeResponse();
    if (resp->getSuccess()) {
        meresp->setFlag(MemEventBase::F_SUCCESS);
    }
    return meresp;
}
SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushResp* req) { 
    output.fatal(CALL_INFO, -1, "%s, Error: FlushResp converter not implemented\n", getName().c_str());
    return nullptr; 
}

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::CustomResp* req) { 
    output.fatal(CALL_INFO, -1, "%s, Error: CustomResp converter not implemented\n", getName().c_str());
    return nullptr; }
SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::InvNotify* req) { 
    output.fatal(CALL_INFO, -1, "%s, Error: InvNotify converter not implemented\n", getName().c_str());
    return nullptr; }

/********************************************************************************************
 * Conversion methods: MemEventBase -> StandardMem::Request
 ********************************************************************************************/
StandardMem::Request* StandardInterface::convertResponseGetSResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(req->makeResponse());
    resp->data = me->getPayload();
    if (me->queryFlag(MemEventBase::F_SUCCESS)) {
        resp->setSuccess();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseGetXResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::Request* resp = req->makeResponse();
    if (me->queryFlag(MemEventBase::F_SUCCESS)) {
        resp->setSuccess();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseFlushResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::Request* resp = req->makeResponse();
    if (!(me->queryFlag(MemEventBase::F_SUCCESS))) {
        resp->unsetSuccess();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertRequestInv(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    StandardMem::InvNotify* req = new StandardMem::InvNotify(event->getAddr(), event->getSize(), 
        0, event->getVirtualAddress(), event->getInstructionPointer(), 0);
    return req;
}
StandardMem::Request* StandardInterface::convertRequestGetS(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    StandardMem::Read* req = new StandardMem::Read(event->getAddr(), event->getSize(), 0, 
        event->getVirtualAddress(), event->getInstructionPointer(), 0);
    return req;
}
StandardMem::Request* StandardInterface::convertRequestGetX(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    StandardMem::Write* req = new StandardMem::Write(event->getAddr(), event->getSize(), event->getPayload(),
        event->queryFlag(MemEventBase::F_NORESPONSE), 0, event->getVirtualAddress(), 
        event->getInstructionPointer(), 0);
    return req;
}
StandardMem::Request* StandardInterface::convertRequestLL(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    return new StandardMem::LoadLink(event->getAddr(), event->getSize(), 0, event->getVirtualAddress(), 
        event->getInstructionPointer(), 0);
}
StandardMem::Request* StandardInterface::convertRequestSC(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    return new StandardMem::StoreConditional(event->getAddr(), event->getSize(), event->getPayload(), 0, 
        event->getVirtualAddress(), event->getInstructionPointer(), 0);
}

StandardMem::Request* StandardInterface::convertRequestLock(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    return new StandardMem::ReadLock(event->getAddr(), event->getSize(), 0, event->getVirtualAddress(), 
        event->getInstructionPointer(), 0);
    }
StandardMem::Request* StandardInterface::convertRequestUnlock(MemEventBase* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    return new StandardMem::WriteUnlock(event->getAddr(), event->getSize(), event->getPayload(), event->queryFlag(MemEventBase::F_NORESPONSE),
        0, event->getVirtualAddress(), event->getInstructionPointer(), 0);
}

StandardMem::Request* StandardInterface::convertRequestCustom(MemEventBase* ev) {

    output.fatal(CALL_INFO, -1, "%s, Error: Not implemented\n", getName().c_str());
    return nullptr;
}

/* Debug checks for creating events 
 * Only enabled if sst-core is configured with "--enable-debug"
 */
void StandardInterface::MemEventConverter::debugChecks(MemEvent* me) {
    // Check that we did actually complete the initialization during init, really should only check this once but no good place to do it
   /* if (!initDone_) {
        output.fatal(CALL_INFO, -1, "Error: In memHierarcnyInterface (%s), init() was never completed and line address mask was not set.\n", 
                getName().c_str());
    }
*/
    // Check that the request doesn't span cache lines
    Addr lastAddr = me->getAddr() + me->getSize() - 1;
    lastAddr &= baseAddrMask_;
    if (lastAddr != me->getBaseAddr()) {
        output.fatal(CALL_INFO, -1, "Error: In memHierarchy Interface (%s), Request cannot span multiple cache lines! Line mask = %" PRIu64 ". Event is: %s\n", 
                getName().c_str(), baseAddrMask_, me->getVerboseString().c_str());
    }
}
