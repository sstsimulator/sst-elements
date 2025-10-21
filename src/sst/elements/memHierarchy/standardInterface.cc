// -*- mode: c++ -*-
// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#include <sstream>
#include <vector>
#include <sst/core/sst_config.h>
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
    setDefaultTimeBase(*time); // Links are required to have a timebase

    debug_level_ = params.find<int>("debug_level", 0);
    // Output object for warnings/debug/etc.
    output_.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);
    debug_.init("", debug_level_, 0, (Output::output_location_t)params.find<int>("debug", 0));

    rqstr_ = "";
    init_done_ = false;

    converter_ = new StandardInterface::MemEventConverter(this);
    untimed_converter_ = new StandardInterface::UntimedMemEventConverter(this);

    // Handler - if nullptr then polling will be assumed
    recv_handler_ = handler;

    link_ = loadUserSubComponent<MemLinkBase>("lowlink", ComponentInfo::SHARE_NONE, getDefaultTimeBase());
    if (!link_) {
        link_ = loadUserSubComponent<MemLinkBase>("memlink", ComponentInfo::SHARE_NONE, getDefaultTimeBase());
        if (link_) {
            output_.output("%s, DEPRECATION WARNING: The StandardInterface's 'memlink' subcomponent slot has been renamed to 'lowlink' to improve name standardization. Please change this in your input file.\n", getName().c_str());
        }
    }
    if (!link_) {
        // Default is a regular non-network link on port 'port'
        std::string port = "";
        if (isPortConnected("lowlink")) port = "lowlink";
        else if (isPortConnected("port")) {
            port = "port";
            output_.output("%s, DEPRECATION WARNING: The StandardInterface's port named 'port' has been renamed to 'lowlink' to improve name standardization. Please change this in your input file.\n", getName().c_str());
        }
        else port = params.find<std::string>("port", "");

        if (port == "") {
            output_.fatal(CALL_INFO, -1, "%s, Error: Unable to configure link. Three options: (1) Fill the 'lowlink' subcomponent slot and connect the subcomponent's port(s). (2) Connect this interface's 'lowlink' port. (3) Connect this interface's parent component's port and pass its name as a parameter to this interface.\n", getName().c_str());
        }

        Params lparams;
        lparams.insert("port", port);
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "lowlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, lparams, getDefaultTimeBase());
    }

    if (!link_)
        output_.fatal(CALL_INFO, -1, "%s, Error: unable to configure link. Three options: (1) Fill the 'lowlink' subcomponent slot and connect the subcomponent's port(s). (2) Connect this interface's 'lowlink' port. (3) Connect this interface's parent component's port and pass its name as a parameter to this interface.\n", getName().c_str());

    link_->setRecvHandler( new SST::Event::Handler2<StandardInterface, &StandardInterface::receive>(this));
    link_->setName(getName());

    /* Set region to default (all addresses) */
    region_.start = 0;
    region_.end = std::numeric_limits<uint64_t>::max();
    region_.interleaveStep = 0;
    region_.interleaveSize = 0;
    endpoint_type_ = Endpoint::CPU;
    link_->setRegion(region_);

    base_addr_mask_ = 0;
    line_size_ = 0;

    std::vector<uint64_t> noncache;
    params.find_array<uint64_t>("noncacheable_regions", noncache);

    for (int i = 0; i < noncache.size(); i+=2) {
        MemRegion reg;
        reg.setEmpty();
        reg.start = noncache[i];
        reg.end = noncache[i+1];
        noncacheable_regions_.insert(std::make_pair(reg.start, reg));
    }
}

void StandardInterface::setMemoryMappedAddressRegion(Addr start, Addr size) {
    region_.start = start;
    region_.end = start + size - 1;
    region_.interleaveStep = 0;
    region_.interleaveSize = 0;
    endpoint_type_ = Endpoint::MMIO;
    link_->setRegion(region_);
}

void StandardInterface::init(unsigned int phase) {
    link_->init(phase);
    /* Send region message */
    if (!phase) {

        /* Broadcast our name, type, and coherence configuration parameters on link */
        MemEventInitCoherence * event = new MemEventInitCoherence(getName(), endpoint_type_, false, false, 0, false);
        link_->sendUntimedData(event);

        /*
         * If we are addressable (MMIO), broadcast that info across the system
         * For now, treat all MMIO regions as noncacheable
         */
        if (endpoint_type_ == Endpoint::MMIO) {
            MemEventInitEndpoint * epEv = new MemEventInitEndpoint(getName().c_str(), endpoint_type_, region_, false);
            link_->sendUntimedData(epEv);
        }
    }

    while (SST::Event * ev = link_->recvUntimedData()) {
        MemEventInit * memEvent = dynamic_cast<MemEventInit*>(ev);
        StandardMem::Request* recv_req = nullptr;
        if (memEvent) {
            switch ( memEvent->getCmd() ) {
                case Command::NULLCMD:
                    if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                        MemEventInitCoherence * memEventC = static_cast<MemEventInitCoherence*>(memEvent);
                        debug_.debug(_L10_, "%s, Received initCoherence message: %s\n", getName().c_str(), memEventC->getVerboseString(debug_level_).c_str());
                        if (memEventC->getType() == Endpoint::Cache) {
                            // Cache takes care of figuring out whether WriteResp is read or write response and other address-related issues
                            cache_is_dst_ = true;
                        }
                        if (memEventC->getType() == Endpoint::Cache || memEventC->getType() == Endpoint::Directory) {
                            base_addr_mask_ = ~(memEventC->getLineSize() - 1);
                            line_size_ = memEventC->getLineSize();
                            debug_.debug(_L10_, "%s, Mask: 0x%" PRIx64 ", Line size: %" PRIu64 "\n", getName().c_str(), base_addr_mask_, line_size_);
                        }
                        init_done_ = true;
                    } else if (memEvent->getInitCmd() == MemEventInit::InitCommand::Endpoint) {
                        MemEventInitEndpoint * memEventE = static_cast<MemEventInitEndpoint*>(memEvent);
                        debug_.debug(_L10_, "%s, Received initEndpoint message: %s\n", getName().c_str(), memEventE->getVerboseString(debug_level_).c_str());
                        std::vector<std::pair<MemRegion,bool>> regions = memEventE->getRegions();
                        for (auto it = regions.begin(); it != regions.end(); it++) {
                            if (!it->second) {
                                noncacheable_regions_.insert(std::make_pair(it->first.start, it->first));
                            }
                        }
                    }
                    delete ev;
                    break;
                case Command::GetS:
                    recv_req = new StandardMem::Read(memEvent->getAddr(), memEvent->getSize());
                    responses_[recv_req->getID()] = memEvent;
                    init_recv_queue_.push(recv_req);
                    break;
                case Command::GetX:
                    recv_req = new StandardMem::Write(memEvent->getAddr(), memEvent->getSize(), memEvent->getPayload(), true);
                    responses_[recv_req->getID()] = memEvent;
                    init_recv_queue_.push(recv_req);
                    break;
                case Command::GetSResp:
                case Command::GetXResp:
                {
                    MemEventBase::id_type orig_id = memEvent->getResponseToID();
                    std::map<MemEventBase::id_type,std::pair<StandardMem::Request*,Command>>::iterator reqit = requests_.find(orig_id);
                    if (reqit == requests_.end()) {
                        output_.fatal(CALL_INFO, -1, "%s, Error: Received response but cannot locate matching request. Response: %s\n",
                            getName().c_str(), memEvent->getVerboseString(debug_level_).c_str());
                    }
                    recv_req = reqit->second.first;
                    requests_.erase(reqit);

                    // Construct StandardMem::ReadResp
                    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(recv_req->makeResponse());
                    resp->data = memEvent->getPayload();
                    init_recv_queue_.push(resp);
                    delete recv_req;
                    delete ev;
                    break;
                }
                default:
                    break;
            };
        } else {
            delete ev;
        }
    }

    if (init_done_) { // Drain send queue
        while (!init_send_queue_.empty()) {
            link_->sendUntimedData(init_send_queue_.front(), false);
            init_send_queue_.pop();
        }
    }

}

/* Nothing to do, just report some debug info about our configuration */
void StandardInterface::setup() {
    debug_.debug(_L9_, "%s, INFO: Line size: %" PRIu64 ", Mask: 0x%" PRIx64 "\n", getName().c_str(), line_size_, base_addr_mask_);
    if (noncacheable_regions_.empty()) {
        debug_.debug(_L9_, "%s, INFO: No noncacheable regions discovered\n", getName().c_str());
    } else {
        std::ostringstream regstr;
        regstr << getName() << ", INFO: Discovered noncacheable regions:";
        for (std::multimap<Addr, MemRegion>::iterator it = noncacheable_regions_.begin(); it != noncacheable_regions_.end(); it++) {
            regstr << " [" << it->second.toString() << "]";
        }
        debug_.debug(_L9_, "%s\n", regstr.str().c_str());
    }
    link_->setup();
}

void StandardInterface::complete(unsigned int phase) {
    link_->complete(phase);
}

void StandardInterface::finish() { }

/* Writes are allowed during init() but nothing else */
void StandardInterface::sendUntimedData(StandardMem::Request *req) {
    MemEventInit* me = static_cast<MemEventInit*>(req->convert(untimed_converter_));
/*
#ifdef __SST_DEBUG_OUTPUT__
    StandardMem::Write* wr = dynamic_cast<StandardMem::Write*>(req);
    if (!wr)
        output_.fatal(CALL_INFO, -1, "%s, Error: sendUntimedData(Request*) only accepts Write requests\n", getName().c_str());
#else
    StandardMem::Write* wr = static_cast<StandardMem::Write*>(req);
#endif

    MemEventInit *me = new MemEventInit(getName(), Command::Write, wr->pAddr, wr->data);
*/
    if (init_done_)
        link_->sendUntimedData(me, false);
    else
        init_send_queue_.push(me);

}

StandardMem::Request* StandardInterface::recvUntimedData() {
    if (!init_recv_queue_.empty()) {
        StandardMem::Request* req = init_recv_queue_.front();
        init_recv_queue_.pop();
        return req;
    }
    return nullptr;
}

/* This could be a request or a response. */
void StandardInterface::send(StandardMem::Request* req) {
    MemEventBase *me = static_cast<MemEventBase*>(req->convert(converter_));
#ifdef __SST_DEBUG_OUTPUT__
      debug_.debug(_L5_, "E: %-40" PRIu64 "  %-20s Req:Convert   EventID: <%" PRIu64", %" PRIu32 "> (%s)\n", getCurrentSimCycle(), getName().c_str(), me->getID().first, me->getID().second, req->getString().c_str());
      if (me->getCmd() == Command::Write) {
        MemEvent* mev = static_cast<MemEvent*>(me);
        debug_.debug(_L11_, "V: %-40" PRIu64 " %-20s  WRITE         0x%-16" PRIx64 "              B: %-3" PRIu32 "   %s\n", getCurrentSimCycle(), getName().c_str(), mev->getAddr(), mev->getSize(), getDataString(&(mev->getPayload())).c_str() );
      }
    fflush(stdout);
#endif

    if (req->needsResponse())
        requests_[me->getID()] = std::make_pair(req,me->getCmd());   /* Save this request so we can use it when a response is returned */
    else
        delete req;
#ifdef __SST_DEBUG_OUTPUT__
    debug_.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Send    (%s)\n",
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
    //debug_.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Recv    (%s)\n", getCurrentSimCycle(), getName().c_str(), me->getBriefString().c_str());
#endif
    /* Handle responses to requests we sent */
    if (isResponse) {
        MemEventBase::id_type origID = me->getResponseToID();
        std::map<MemEventBase::id_type,std::pair<StandardMem::Request*,Command>>::iterator reqit = requests_.find(origID);
        if (reqit == requests_.end()) {
            output_.fatal(CALL_INFO, -1, "%s, Error: Received response but cannot locate matching request. Response: %s\n",
                getName().c_str(), me->getVerboseString(debug_level_).c_str());
        }
        StandardMem::Request* origReq = reqit->second.first;
        Command origCmd = reqit->second.second;
        if (origCmd == Command::GetS || origCmd == Command::GetSX)
            cmd = Command::GetSResp;
        requests_.erase(reqit);
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
                output_.fatal(CALL_INFO, -1, "%s, Error: Received response with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString(debug_level_).c_str());
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
                output_.fatal(CALL_INFO, -1, "%s, Error: Received request with unhandled command type '%s'. Event: %s\n",
                    getName().c_str(), CommandString[(int)cmd], me->getVerboseString(debug_level_).c_str());
        };
        if (deliverReq->needsResponse()) /* Endpoint will need to send a response to this */
            responses_[deliverReq->getID()] = me;
        else
            delete me;
    }

    if (deliverReq) {
#ifdef __SST_DEBUG_OUTPUT__
        debug_.debug(_L5_, "E: %-40" PRIu64 "  %-20s Req:Deliver   (%s)\n", getCurrentSimCycle(), getName().c_str(), deliverReq->getString().c_str());
#endif
        (*recv_handler_)(deliverReq);
    }
}


/********************************************************************************************
 * Conversion methods: StandardMem::Request -> MemEventBase
 ********************************************************************************************/

SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::Read* req) {
    bool noncacheable = false;

    if (req->getNoncacheable()) {
        noncacheable = true;
    } else if (!((iface->noncacheable_regions_).empty())) {
        // Check if addr lies in noncacheable regions.
        // For simplicity we are not dealing with the case where the address range splits a noncacheable + cacheable region
        std::multimap<Addr, MemRegion>::iterator ep = (iface->noncacheable_regions_).upper_bound(req->pAddr);
        for (std::multimap<Addr, MemRegion>::iterator it = (iface->noncacheable_regions_).begin(); it != ep; it++) {
            if (it->second.contains(req->pAddr)) {
                noncacheable = true;
                break;
            }
        }
    }

    Addr bAddr = (iface->line_size_ == 0 || noncacheable) ? req->pAddr : req->pAddr & iface->base_addr_mask_; // Line address
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
    } else if (!((iface->noncacheable_regions_).empty())) {
        // Check if addr lies in noncacheable regions.
        // For simplicity we are not dealing with the case where the address range splits a noncacheable + cacheable region
        std::multimap<Addr, MemRegion>::iterator ep = (iface->noncacheable_regions_).upper_bound(req->pAddr);
        for (std::multimap<Addr, MemRegion>::iterator it = (iface->noncacheable_regions_).begin(); it != ep; it++) {
            if (it->second.contains(req->pAddr)) {
                noncacheable = true;
                break;
            }
        }
    }

    Addr bAddr = (iface->line_size_ == 0 || noncacheable) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
        iface->debug_.verbose(CALL_INFO, 1, 0, "Warning (%s): Write request size is %" PRIu64 " and payload size is %zu. MemEvent will use payload size.\n",
            iface->getName().c_str(), req->size, req->data.size());
    }
    debugChecks(write);
#endif
    return write;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushAddr* req) {
    Addr bAddr = (iface->line_size_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
        iface->output_.fatal(CALL_INFO, -1, "%s, Error: Received noncacheable flush request. This combination is not supported.\n",
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
    Addr bAddr = (iface->line_size_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
    Addr bAddr = (iface->line_size_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
    Addr bAddr = (iface->line_size_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
    Addr bAddr = (iface->line_size_ == 0 || req->getNoncacheable()) ? req->pAddr : req->pAddr & iface->base_addr_mask_;
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
    Addr bAddrSrc = (iface->line_size_ == 0) ? req-> pSrc : req->pSrc & iface->base_addr_mask_;
    Addr bAddrDst = (iface->line_size_ == 0) ? req-> pDst : req->pDst & iface->base_addr_mask_;
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
        iface->output_.fatal(CALL_INFO, -1, "%s, Error: Handling a ReadResp but no matching Read found\n", iface->getName().c_str());
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
        iface->output_.fatal(CALL_INFO, -1, "%s, Error: Handling a WriteResp but no matching Write found\n", iface->getName().c_str());
    MemEvent* mereq = static_cast<MemEvent*>(it->second); // Matching memEvent req
    iface->responses_.erase(it);
    MemEvent* meresp = mereq->makeResponse();
    if (!resp->getSuccess()) {
        meresp->setFail();
    }
    return meresp;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::FlushResp* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: FlushResp converter not implemented\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::CustomResp* req) {

    iface->output_.fatal(CALL_INFO, -1, "%s, Error: CustomResp converter not implemented\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::MemEventConverter::convert(StandardMem::InvNotify* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: InvNotify converter not implemented\n", iface->getName().c_str());
    return nullptr;
}

/********************************************************************************************
 * Conversion methods: StandardMem::Request -> MemEventInit (used in untimed phases)
 ********************************************************************************************/

SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::Read* req) {
    MemEventInit *me = new MemEventInit(iface->getName(), Command::GetS, req->pAddr, req->size);
    iface->requests_[me->getID()] = std::make_pair(req,me->getCmd());   /* Save this request so we can use it when a response is returned */
    return me;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::ReadResp* resp) {
    std::map<StandardMem::Request::id_t, MemEventBase*>::iterator it = iface->responses_.find(resp->getID());
    if (it == iface->responses_.end())
        iface->output_.fatal(CALL_INFO, -1, "%s, Error: Handling a ReadResp but no matching Read found\n", iface->getName().c_str());

    MemEventInit* req = static_cast<MemEventInit*>(it->second);
    iface->responses_.erase(it);

    MemEventInit* meresp = req->makeResponse();
    meresp->setPayload(resp->data);
    return meresp;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::Write* req) {
    MemEventInit *me = new MemEventInit(iface->getName(), Command::Write, req->pAddr, req->data);
    return me;
}


// Not supported
SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::WriteResp* req) {
    // Currently, writes to memory during init() are not ack'd;
    // adding the ability may change visible memH behavior since writes default to acking
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: WriteResp is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::FlushAddr* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: FlushAddr is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::FlushCache* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: FlushCache is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::FlushResp* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: FlushResp is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::ReadLock* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: ReadLock is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::WriteUnlock* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: WriteUnlock is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::LoadLink* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: LoadLink is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::StoreConditional* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: StoreConditional is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::MoveData* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: MoveData is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::CustomReq* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: CustomReq is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::CustomResp* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: CustomResp is not supported during untimed phases\n", iface->getName().c_str());
    return nullptr;
}


SST::Event* StandardInterface::UntimedMemEventConverter::convert(StandardMem::InvNotify* req) {
    iface->output_.fatal(CALL_INFO, -1, "%s, Error: InvNotify is not supported during untimed phases\n", iface->getName().c_str());
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

    output_.fatal(CALL_INFO, -1, "%s, Error: Not implemented\n", getName().c_str());
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
    debug_.debug(_L4_, "E: %-40" PRIu64 "  %-20s Event:Resend  (%s)\n",
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
   /* if (!init_done_) {
        output_.fatal(CALL_INFO, -1, "Error: In memHierarcnyInterface (%s), init() was never completed and line address mask was not set.\n",
                getName().c_str());
    }
*/
    // Check that the request doesn't span cache lines
    if (iface->line_size_ != 0 && !(me->queryFlag(MemEventBase::F_NONCACHEABLE))) {
        Addr lastAddr = me->getAddr() + me->getSize() - 1;
        lastAddr &= iface->base_addr_mask_;
        if (lastAddr != me->getBaseAddr()) {
            iface->output_.fatal(CALL_INFO, -1, "Error: In memHierarchy Interface (%s), Request cannot span multiple cache lines! Line mask = 0x%" PRIx64 ". Event is: %s\n",
                    iface->getName().c_str(), iface->base_addr_mask_, me->getVerboseString(iface->output_.getVerboseLevel()).c_str());
        }
    }
}


/********************************************************************************************
 * Serialization functions
 ********************************************************************************************/
/*
 * Default constructor
*/
StandardInterface::StandardInterface() : StandardMem() {}

/*
 * Serialize function
*/
void StandardInterface::serialize_order(SST::Core::Serialization::serializer& ser) {
    StandardMem::serialize_order(ser);

    SST_SER(output_);
    SST_SER(debug_);
    SST_SER(debug_level_);

    SST_SER(base_addr_mask_);
    SST_SER(line_size_);
    SST_SER(rqstr_);
    SST_SER(requests_);
    SST_SER(responses_);
    SST_SER(link_);
    SST_SER(cache_is_dst_);

    SST_SER(init_done_);
    SST_SER(init_send_queue_);
    SST_SER(init_recv_queue_);

    SST_SER(region_);
    SST_SER(endpoint_type_);

    SST_SER(noncacheable_regions_);

    SST_SER(recv_handler_);
    SST_SER(converter_);
    SST_SER(untimed_converter_);
}
