// -*- mode: c++ -*-
// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
#include "sst/elements/memHierarchy/moveEvent.h"
#include "sst/elements/memHierarchy/memEventCustom.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


StandardInterface::StandardInterface(SST::ComponentId_t id, Params &params, TimeConverter * time, HandlerBase* handler) :
    StandardMem(id, params, time, handler)
{
    setDefaultTimeBase(time); // Links are required to have a timebase

    dlevel = params.find<int>("debug_level", 0);
    // Output object for warnings/debug/etc.
    output.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);
    debug.init("", dlevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    rqstr_ = "";
    initDone_ = false;

    converter_ = new StandardInterface::MemEventConverter(this);
    converter_->output = debug;

    // Handler - if nullptr then polling will be assumed
    recvHandler_ = handler;

    link_ = loadUserSubComponent<MemLinkBase>("lowlink", ComponentInfo::SHARE_NONE, getDefaultTimeBase());
    if (!link_) {
        link_ = loadUserSubComponent<MemLinkBase>("memlink", ComponentInfo::SHARE_NONE, getDefaultTimeBase());
        if (link_) {
            output.output("%s, DEPRECATION WARNING: The StandardInterface's 'memlink' subcomponent slot has been renamed to 'lowlink' to improve name standardization. Please change this in your input file.\n", getName().c_str());
        }
    }
    if (!link_) {
        // Default is a regular non-network link on port 'port'
        std::string port = "";
        if (isPortConnected("lowlink")) port = "lowlink";
        else if (isPortConnected("port")) {
            port = "port";
            output.output("%s, DEPRECATION WARNING: The StandardInterface's port named 'port' has been renamed to 'lowlink' to improve name standardization. Please change this in your input file.\n", getName().c_str());
        }
        else port = params.find<std::string>("port", "");

        if (port == "") {
            output.fatal(CALL_INFO, -1, "%s, Error: Unable to configure link. Three options: (1) Fill the 'lowlink' subcomponent slot and connect the subcomponent's port(s). (2) Connect this interface's 'lowlink' port. (3) Connect this interface's parent component's port and pass its name as a parameter to this interface.\n", getName().c_str());
        }

        Params lparams;
        lparams.insert("port", port);
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "lowlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, lparams, getDefaultTimeBase());
    }

    if (!link_)
        output.fatal(CALL_INFO, -1, "%s, Error: unable to configure link. Three options: (1) Fill the 'lowlink' subcomponent slot and connect the subcomponent's port(s). (2) Connect this interface's 'lowlink' port. (3) Connect this interface's parent component's port and pass its name as a parameter to this interface.\n", getName().c_str());
    
    link_->setRecvHandler( new SST::Event::Handler<StandardInterface>(this, &StandardInterface::receive));
    link_->setName(getName());

    /* Set region to default (all addresses) */
    region.start = 0;
    region.end = std::numeric_limits<uint64_t>::max();
    region.interleaveStep = 0;
    region.interleaveSize = 0;
    epType = Endpoint::CPU;
    link_->setRegion(region);

    baseAddrMask_ = 0;
    lineSize_ = 0;

    std::vector<uint64_t> noncache;
    params.find_array<uint64_t>("noncacheable_regions", noncache);

    for (int i = 0; i < noncache.size(); i+=2) {
        MemRegion reg;
        reg.setEmpty();
        reg.start = noncache[i];
        reg.end = noncache[i+1];
        noncacheableRegions.insert(std::make_pair(reg.start, reg));
    }
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
        link_->sendUntimedData(event);
        
        /* 
         * If we are addressable (MMIO), broadcast that info across the system 
         * For now, treat all MMIO regions as noncacheable
         */
        if (epType == Endpoint::MMIO) {
            MemEventInitEndpoint * epEv = new MemEventInitEndpoint(getName().c_str(), epType, region, false);
            link_->sendUntimedData(epEv);
        }
    }

    while (SST::Event * ev = link_->recvUntimedData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        if (memEvent) {
            if (memEvent->getCmd() == Command::NULLCMD) {
                if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                    MemEventInitCoherence * memEventC = static_cast<MemEventInitCoherence*>(memEvent);
                    debug.debug(_L10_, "%s, Received initCoherence message: %s\n", getName().c_str(), memEventC->getVerboseString(dlevel).c_str());
                    if (memEventC->getType() == Endpoint::Cache) {
                        cacheDst_ = true; // Cache takes care of figuring out whether WriteResp is read or write response and other address-related issues
                    }
                    if (memEventC->getType() == Endpoint::Cache || memEventC->getType() == Endpoint::Directory) {
                        baseAddrMask_ = ~(memEventC->getLineSize() - 1);
                        lineSize_ = memEventC->getLineSize();
                        debug.debug(_L10_, "%s, Mask: 0x%" PRIx64 ", Line size: %" PRIu64 "\n", getName().c_str(), baseAddrMask_, lineSize_);
                    }
                    initDone_ = true;
                } else if (memEvent->getInitCmd() == MemEventInit::InitCommand::Endpoint) {
                    MemEventInitEndpoint * memEventE = static_cast<MemEventInitEndpoint*>(memEvent);
                    debug.debug(_L10_, "%s, Received initEndpoint message: %s\n", getName().c_str(), memEventE->getVerboseString(dlevel).c_str());
                    std::vector<std::pair<MemRegion,bool>> regions = memEventE->getRegions();
                    for (auto it = regions.begin(); it != regions.end(); it++) {
                        if (!it->second) {
                            noncacheableRegions.insert(std::make_pair(it->first.start, it->first));
                        }
                    }
                }
            }
        }
        delete ev;
    }

    if (initDone_) { // Drain send queue
        while (!initSendQueue_.empty()) {
            link_->sendUntimedData(initSendQueue_.front(), false);
            initSendQueue_.pop();
        }
    }

}

/* Nothing to do, just report some debug info about our configuration */
void StandardInterface::setup() { 
    debug.debug(_L9_, "%s, INFO: Line size: %" PRIu64 ", Mask: 0x%" PRIx64 "\n", getName().c_str(), lineSize_, baseAddrMask_);
    if (noncacheableRegions.empty()) {
        debug.debug(_L9_, "%s, INFO: No noncacheable regions discovered\n", getName().c_str());
    } else {
        std::ostringstream regstr;
        regstr << getName() << ", INFO: Discovered noncacheable regions:";
        for (std::multimap<Addr, MemRegion>::iterator it = noncacheableRegions.begin(); it != noncacheableRegions.end(); it++) {
            regstr << " [" << it->second.toString() << "]";
        }
        debug.debug(_L9_, "%s\n", regstr.str().c_str());
    }
    link_->setup();
}

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
    
    MemEventInit *me = new MemEventInit(getName(), Command::Write, wr->pAddr, wr->data);
    if (initDone_)
        link_->sendUntimedData(me, false);
    else
        initSendQueue_.push(me);
}

StandardMem::Request* StandardInterface::recvUntimedData() {
    return nullptr; // TODO FIGURE THIS OUT    
}

/* This could be a request or a response. */
void StandardInterface::send(StandardMem::Request* req) {
    MemEventBase *me = static_cast<MemEventBase*>(req->convert(converter_));
#ifdef __SST_DEBUG_OUTPUT__
      debug.debug(_L5_, "E: %-40" PRIu64 "  %-20s Req:Convert   EventID: <%" PRIu64", %" PRIu32 "> (%s)\n", getCurrentSimCycle(), getName().c_str(), me->getID().first, me->getID().second, req->getString().c_str());
    fflush(stdout);
#endif

    if (req->needsResponse())
        requests_[me->getID()] = std::make_pair(req,me->getCmd());   /* Save this request so we can use it when a response is returned */
    else
        delete req;
#ifdef __SST_DEBUG_OUTPUT__
    debug.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Send    (%s)\n", 
        getCurrentSimCycle(), getName().c_str(), me->getBriefString().c_str());
#endif
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
#ifdef __SST_DEBUG_OUTPUT__ 
    //debug.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Recv    (%s)\n", getCurrentSimCycle(), getName().c_str(), me->getBriefString().c_str());
#endif
    /* Handle responses to requests we sent */
    if (isResponse) {
        MemEventBase::id_type origID = me->getResponseToID();
        std::map<MemEventBase::id_type,std::pair<StandardMem::Request*,Command>>::iterator reqit = requests_.find(origID);
        if (reqit == requests_.end()) {
            output.fatal(CALL_INFO, -1, "%s, Error: Received response but cannot locate matching request. Response: %s\n",
                getName().c_str(), me->getVerboseString(dlevel).c_str());
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
            case Command::WriteResp:
                deliverReq = convertResponseWriteResp(origReq, me);
                break;
            case Command::FlushLineResp:
                deliverReq = convertResponseFlushResp(origReq, me);
                break;
            case Command::FlushAllResp:
                deliverReq = convertResponseFlushAllResp(origReq, me);
                break;
            case Command::AckMove:
                deliverReq = convertResponseAckMove(origReq, me);
                break;
            case Command::CustomResp:
                deliverReq = convertResponseCustomResp(origReq, me);
                break;
            case Command::NACK:
                handleNACK(me);
                delete me;
                return;
            default:
                output.fatal(CALL_INFO, -1, "%s, Error: Received response with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString(dlevel).c_str());
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
            case Command::Write:
                if (me->queryFlag(MemEventBase::F_LLSC))
                    deliverReq = convertRequestSC(me);
                else if (me->queryFlag(MemEventBase::F_LOCKED)) {
                    deliverReq = convertRequestUnlock(me);
                } else {
                    deliverReq = convertRequestWrite(me);
                }
                break;
            case Command::CustomReq:
                deliverReq = convertRequestCustom(me);
                break;
            default:
                output.fatal(CALL_INFO, -1, "%s, Error: Received request with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString(dlevel).c_str());
        };
        if (deliverReq->needsResponse()) /* Endpoint will need to send a response to this */
            responses_[deliverReq->getID()] = me;
        else 
            delete me;
    }

    if (deliverReq) {
#ifdef __SST_DEBUG_OUTPUT__
        debug.debug(_L5_, "E: %-40" PRIu64 "  %-20s Req:Deliver   (%s)\n", getCurrentSimCycle(), getName().c_str(), deliverReq->getString().c_str());
#endif
        (*recvHandler_)(deliverReq);
    }
}

/********************************************************************************************
 * Conversion methods: StandardMem::Request -> MemEventBase
 ********************************************************************************************/

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::Read* req) {
    bool noncacheable = false;

    if (req->getNoncacheable()) {
        noncacheable = true;
    } else if (!((iface->noncacheableRegions).empty())) {
        // Check if addr lies in noncacheable regions. 
        // For simplicity we are not dealing with the case where the address range splits a noncacheable + cacheable region
        std::multimap<Addr, MemRegion>::iterator ep = (iface->noncacheableRegions).upper_bound(req->pAddr);
        for (std::multimap<Addr, MemRegion>::iterator it = (iface->noncacheableRegions).begin(); it != ep; it++) {
            if (it->second.contains(req->pAddr)) {
                noncacheable = true;
                break;
            }
        }
    }

    Addr bAddr = (iface->lineSize_ == 0 || noncacheable) ? req->pAddr : req->pAddr & iface->baseAddrMask_; // Line address
    MemEvent* read = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::GetS, req->size);
    read->setRqstr(iface->getName());
    read->setThreadID(req->tid);
    read->setDst(iface->link_->getTargetDestination(bAddr));
    read->setVirtualAddress(req->vAddr);
    read->setInstructionPointer(req->iPtr);
    if (noncacheable)
        read->setFlag(MemEvent::F_NONCACHEABLE);

#ifdef __SST_DEBUG_OUTPUT__
    debugChecks(read);
#endif
    return read;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::Write* req) {
    bool noncacheable = false;
    
    if (req->getNoncacheable()) {
        noncacheable = true;
    } else if (!((iface->noncacheableRegions).empty())) {
        // Check if addr lies in noncacheable regions. 
        // For simplicity we are not dealing with the case where the address range splits a noncacheable + cacheable region
        std::multimap<Addr, MemRegion>::iterator ep = (iface->noncacheableRegions).upper_bound(req->pAddr);
        for (std::multimap<Addr, MemRegion>::iterator it = (iface->noncacheableRegions).begin(); it != ep; it++) {
            if (it->second.contains(req->pAddr)) {
                noncacheable = true;
                break;
            }
        }
    }
    
    Addr bAddr = (iface->lineSize_ == 0 || noncacheable) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    MemEvent* write = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::Write, req->data);
    
    write->setRqstr(iface->getName());
    write->setThreadID(req->tid);
    write->setDst(iface->link_->getTargetDestination(bAddr));
    write->setVirtualAddress(req->vAddr);
    write->setInstructionPointer(req->iPtr);
    
    if (noncacheable)    
        write->setFlag(MemEvent::F_NONCACHEABLE);

    if (req->posted)
        write->setFlag(MemEvent::F_NORESPONSE);
    
    if (req->data.size() == 0) { // Endpoint isn't using real data values & didn't give a dummy payload
        write->setZeroPayload(req->size);
    }
#ifdef __SST_DEBUG_OUTPUT__
    else if (req->data.size() != req->size) {
        output.verbose(CALL_INFO, 1, 0, "Warning (%s): Write request size is %" PRIu64 " and payload size is %zu. MemEvent will use payload size.\n",
            iface->getName().c_str(), req->size, req->data.size());
    } 
    debugChecks(write);
#endif
    return write;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushAddr* req) {
    Addr bAddr = (iface->lineSize_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    Command cmd = req->inv ? Command::FlushLineInv : Command::FlushLine;

    MemEvent* flush = new MemEvent(iface->getName(), req->pAddr, bAddr, cmd, req->size);
    flush->setRqstr(iface->getName());
    flush->setThreadID(req->tid);
    flush->setDst(iface->link_->getTargetDestination(bAddr));
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

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushCache* req) {
    MemEvent* flush = new MemEvent(iface->getName(), Command::FlushAll);
    flush->setRqstr(iface->getName());
    flush->setThreadID(req->tid);
    std::string dst = iface->link_->getDests()->begin()->name;
    flush->setDst(dst);
    flush->setVirtualAddress(0); /* Not routed by address, will be 0 */
    flush->setInstructionPointer(req->iPtr);
    return flush;
}

Event* StandardInterface::MemEventConverter::convert(StandardMem::ReadLock* req) {
    Addr bAddr = (iface->lineSize_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    MemEvent* read = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::GetSX, req->size);
    read->setRqstr(iface->getName());
    read->setThreadID(req->tid);
    read->setDst(iface->link_->getTargetDestination(bAddr));
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
    Addr bAddr = (iface->lineSize_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    MemEvent* write = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::Write, req->data);
    write->setRqstr(iface->getName());
    write->setThreadID(req->tid);
    write->setDst(iface->link_->getTargetDestination(bAddr));
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
    Addr bAddr = (iface->lineSize_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    MemEvent* load = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::GetSX, req->size);
    load->setFlag(MemEvent::F_LLSC);
    load->setRqstr(iface->getName());
    load->setThreadID(req->tid);
    load->setDst(iface->link_->getTargetDestination(bAddr));
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
    Addr bAddr = (iface->lineSize_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->baseAddrMask_;
    MemEvent* store = new MemEvent(iface->getName(), req->pAddr, bAddr, Command::Write, req->data);
    store->setFlag(MemEvent::F_LLSC);
    store->setRqstr(iface->getName());
    store->setThreadID(req->tid);
    store->setDst(iface->link_->getTargetDestination(bAddr));
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
    Addr bAddrSrc = (iface->lineSize_ == 0) ? req-> pSrc : req->pSrc & iface->baseAddrMask_;
    Addr bAddrDst = (iface->lineSize_ == 0) ? req-> pDst : req->pDst & iface->baseAddrMask_;
    // The memHierarchy scratchpad implementation has its local addresses lower than the memory addresses
    // This would need to change if that invariant is removed
    // TODO May work to replace both Get/Put with generic Move and let scratchpad/other component decide
    // how to treat it based on the addresses
    Command cmd = (req->pDst > req->pSrc) ? Command::Put : Command::Get;
    MoveEvent* move = new MoveEvent(iface->getName(), req->pSrc, bAddrSrc, req->pDst, bAddrDst, cmd);
        
    if (req->posted) {
        move->setFlag(MemEventBase::F_NORESPONSE);
    }
    move->setSize(req->size);
    move->setSrcVirtualAddress(req->vSrc);
    move->setDstVirtualAddress(req->vDst);
    move->setInstructionPointer(req->iPtr);

    // No debugChecks() as this request is OK to span cachelines
    return move;
}
Event* StandardInterface::MemEventConverter::convert(StandardMem::CustomReq* req) {
    CustomMemEvent* creq = new CustomMemEvent(iface->getName(), Command::CustomReq, req->data);
    if (!req->needsResponse())
        creq->setFlag(MemEventBase::F_NORESPONSE);

    return creq;
}

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::ReadResp* resp) { 
    std::map<StandardMem::Request::id_t, MemEventBase*>::iterator it = iface->responses_.find(resp->getID());
    if (it == iface->responses_.end())
        iface->output.fatal(CALL_INFO, -1, "%s, Error: Handling a ReadResp but no matching Read found\n", iface->getName().c_str());
    MemEvent* mereq = static_cast<MemEvent*>(it->second); // Matching memEvent req
    iface->responses_.erase(it);
    MemEvent* meresp = mereq->makeResponse();
    meresp->setPayload(resp->data);
    if (!resp->getSuccess()) {
        meresp->setFail();
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
    if (!resp->getSuccess()) {
        meresp->setFail();
    }
    return meresp;
}
SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushResp* req) { 
    output.fatal(CALL_INFO, -1, "%s, Error: FlushResp converter not implemented\n", iface->getName().c_str());
    return nullptr; 
}

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::CustomResp* req) {

    output.fatal(CALL_INFO, -1, "%s, Error: CustomResp converter not implemented\n", iface->getName().c_str());
    return nullptr; 
}

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::InvNotify* req) { 
    output.fatal(CALL_INFO, -1, "%s, Error: InvNotify converter not implemented\n", iface->getName().c_str());
    return nullptr; 
}

/********************************************************************************************
 * Conversion methods: MemEventBase -> StandardMem::Request
 ********************************************************************************************/
StandardMem::Request* StandardInterface::convertResponseGetSResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(req->makeResponse());
    if (resp->size == me->getSize()) {
        resp->data = me->getPayload();
    } else { // Need to extract just the relevant bit of the payload
        Addr offset = me->getAddr() - me->getBaseAddr();
        auto payload = me->getPayload();
        resp->data.assign(payload.begin() + offset, payload.begin() + offset + resp->size);
    }
    if (!me->success()) {
        resp->setFail();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseWriteResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::Request* resp = req->makeResponse();
    if (!me->success()) {
        resp->setFail();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseFlushResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::Request* resp = req->makeResponse();
    if (!me->success()) {
        resp->setFail();
        resp->unsetSuccess();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseFlushAllResp(StandardMem::Request* req, MemEventBase* meb) {
    MemEvent* me = static_cast<MemEvent*>(meb);
    StandardMem::Request* resp = req->makeResponse();
    if (!me->success()) {
        resp->setFail();
        resp->unsetSuccess();
    }
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseAckMove(StandardMem::Request* req, UNUSED(MemEventBase* meb)) {
    StandardMem::Request* resp = req->needsResponse()? req->makeResponse() : nullptr;
    return resp;
}

StandardMem::Request* StandardInterface::convertResponseCustomResp(StandardMem::Request* req, MemEventBase* meb) {
    StandardMem::Request* resp = req->needsResponse() ? req->makeResponse() : nullptr;
    StandardMem::CustomResp* cresp = static_cast<StandardMem::CustomResp*>(resp);
    if (cresp) { 
        cresp->data = static_cast<CustomMemEvent*>(meb)->getCustomData(); 
    }
    return cresp;
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

StandardMem::Request* StandardInterface::convertRequestWrite(MemEventBase* ev) {
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

/********************************************************************************************
 * NACK handling
 ********************************************************************************************/

void StandardInterface::handleNACK(MemEventBase* ev) {
    MemEvent* nackedEvent = (static_cast<MemEvent*>(ev))->getNACKedEvent();
    
    // No backoff possible as MemLinkBase interface can't stall requests
    // and we don't either
    nackedEvent->incrementRetries();

#ifdef __SST_DEBUG_OUTPUT__
    debug.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Resend  (%s)\n", 
        getCurrentSimCycle(), getName().c_str(), nackedEvent->getBriefString().c_str());
#endif

    link_->send(nackedEvent);
}

/********************************************************************************************
 * Debug functions
 ********************************************************************************************/
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
    if (iface->lineSize_ != 0 && !(me->queryFlag(MemEventBase::F_NONCACHEABLE))) {
        Addr lastAddr = me->getAddr() + me->getSize() - 1;
        lastAddr &= iface->baseAddrMask_;
        if (lastAddr != me->getBaseAddr()) {
            output.fatal(CALL_INFO, -1, "Error: In memHierarchy Interface (%s), Request cannot span multiple cache lines! Line mask = 0x%" PRIx64 ". Event is: %s\n", 
                    iface->getName().c_str(), iface->baseAddrMask_, me->getVerboseString(output.getVerboseLevel()).c_str());
        }
    }
}
