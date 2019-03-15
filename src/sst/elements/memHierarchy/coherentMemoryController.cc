// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "coherentMemoryController.h"
#include "util.h"

#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"
#include "membackend/memBackendConvertor.h"
#include "memLink.h"
#include "memNIC.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__ )
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define Debug(level, fmt, ... )
#endif

/* Construct CoherentMemController
 *  Initialize directory_ variable to indicate whether a directory is in the system
 *      For a directory, a replacement is not synonymous with eviction
 */
CoherentMemController::CoherentMemController(ComponentId_t id, Params &params) : MemController(id, params) {
    directory_ = false; /* Updated during init */
    timestamp_ = 0;
}

/**
 * Init
 */
void CoherentMemController::init(unsigned int phase) {
    link_->init(phase);
    
    region_ = link_->getRegion(); // This can change during init, but should stabilize before we start receiving init data
    
    /* Inherit region from our source(s) */
    if (!phase) {
        /* Announce our presence on link */
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth(), true));
    }

    while (MemEventInit *ev = link_->recvInitData()) {
        processInitEvent(ev);
    }
}

/*
 * After init we know line size so initialize cacheStatus_
 */
void CoherentMemController::setup(void) {
    MemController::setup();

    cacheStatus_.resize(memSize_/lineSize_, false); /* Initialize all cache status to false (uncached) */
}


/*
 * Overrides MemController function
 * Intercept InitCoherence message to set directory_ and lineSize_ 
 */
void CoherentMemController::processInitEvent(MemEventInit* me) {
    if (Command::NULLCMD == me->getCmd()) {
        if (me->getInitCmd() == MemEventInit::InitCommand::Coherence) {
            MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(me);
            if (eventC->getType() == Endpoint::Directory)
                directory_ = true;
            lineSize_ = eventC->getLineSize();
        }
    }
    MemController::processInitEvent(me);
}


/* 
 * Overrides MemController function
 * msgQueue holds events generated in CoherentMemController
 *   such as shootdowns and nack retries
 * Other events are issued immediately from handleMemResponse(...)
 * 'timestamp_' is used to delay events in the queue and does not need to be sync'd with 'cycle' when the clock is reenabled
 */
bool CoherentMemController::clock(Cycle_t cycle) {
    timestamp_++;

    bool debug = false;
    while (!msgQueue_.empty() && msgQueue_.begin()->first < timestamp_) {
        MemEventBase * sendEv = msgQueue_.begin()->second;

        if (is_debug_event(sendEv)) {
            if (!debug) dbg.debug(_L4_, "\n");
            debug = true;
            dbg.debug(_L4_, "%" PRIu64 " (%s) Sending event to processor: %s\n", timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
        }

        link_->send(sendEv);
        msgQueue_.erase(msgQueue_.begin());
    }

    /* Unclock if nothing is in clocked queues anywhere (link, backend, here) */
    bool unclockLink = true;
    if (clockLink_)
        unclockLink = link_->clock();   /* OK to unclock link? */
    bool unclockBack = memBackendConvertor_->clock(cycle); /* OK to unclock backend? */

    if (unclockLink && unclockBack && msgQueue_.empty()) {
        memBackendConvertor_->turnClockOff();
        clockOn_ = false;
        return true;
    }

    return false;
}


/* 
 * Link handler, overrides MemController's 
 */
void CoherentMemController::handleEvent(SST::Event* event) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }
    
    MemEventBase * ev = static_cast<MemEventBase*>(event);

    if (is_debug_event(ev)) {
        Debug(_L3_, "\n%" PRIu64 " (%s) Received: %s\n", getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());
    }

    Command cmd = ev->getCmd();
    
    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            handleRequest(static_cast<MemEvent*>(ev));
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            handleFlush(static_cast<MemEvent*>(ev));
            break;
        case Command::PutM:
        case Command::PutS:
        case Command::PutE:
            handleReplacement(static_cast<MemEvent*>(ev));
            break;
        case Command::CustomReq:
            handleCustomCmd(ev);
            break;
        case Command::AckInv:
            handleAckInv(static_cast<MemEvent*>(ev));
            break;
        case Command::FetchResp:
            handleFetchResp(static_cast<MemEvent*>(ev));
            break;
        case Command::NACK:
            handleNack(static_cast<MemEvent*>(ev));
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Memory controller (%s) received unrecgonized command: %s. Time = %" PRIu64 "ns\n", getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/* 
 * Handle request events
 */
void CoherentMemController::handleRequest(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, ev->getBaseAddr())));
    notifyListeners(ev);

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
        if (!ev->queryFlag(MemEventBase::F_NONCACHEABLE)) {
            cacheStatus_.at(ev->getBaseAddr()/lineSize_) = true;
        }
        memBackendConvertor_->handleMemEvent(ev);
    } else {
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), ev->getCmd()));
    }
}


/*
 * Handle replacement events
 * Special case when replacement races with shootdown because
 * we may not receive an Ack for the shootdown or may not receive 
 * data with the Ack
 */
void CoherentMemController::handleReplacement(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    /* Handle shootdown race where no further Ack is expected */
    if (!directory_ && mshr_.find(ev->getBaseAddr()) != mshr_.end()) {
        MSHREntry * entry = &(mshr_.find(ev->getBaseAddr())->second.front());
        if (entry->cmd == Command::CustomReq && entry->shootdown) {
            ev->getPayload().empty() ? handleAckInv(ev) : handleFetchResp(ev);
            return;
        }
    }

    /* Drop clean writebacks after updating the cache */
    if (!ev->getDirty()) {
        cacheStatus_.at(ev->getBaseAddr()/lineSize_) = directory_; // If directory, writeback does not imply eviction
        delete ev;
        return;
    }

    ev->setFlag(MemEventBase::F_NORESPONSE);

    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, ev->getBaseAddr())));
    notifyListeners(ev);

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
        cacheStatus_.at(ev->getBaseAddr()/lineSize_) = directory_;
        memBackendConvertor_->handleMemEvent(ev);
    } else {
        /* Search for race with a shootdown where we might receive an Ack but not data */
        std::list<MSHREntry>* entryList = &(mshr_.find(ev->getBaseAddr())->second);
        for (std::list<MSHREntry>::iterator it = entryList->begin(); it != entryList->end(); it++) {
            if (it->cmd == Command::CustomReq && it->shootdown) {
                if (it == entryList->begin()) { /* Shootdown in progress, we will receive an Ack but possibly no data with it */
                    it->writebacks.insert(ev->getID());
                    memBackendConvertor_->handleMemEvent(ev);
                } else {
                    it = entryList->insert(it, MSHREntry(ev->getID(), ev->getCmd())); /* Process replacement before next shootdown */
                }
                return;
            }
        }
        /* No races, insert at back of MSHR list */
        entryList->push_back(MSHREntry(ev->getID(), ev->getCmd()));
    }
}


/* 
 * Handle Flush request
 * FlushInv means all caches have evicted the block
 * Flush is just a writeback of dirty data
 *
 */
void CoherentMemController::handleFlush(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    MemEvent* put = NULL;
    if (ev->getPayloadSize() != 0) {
        put = new MemEvent(this, ev->getBaseAddr(), ev->getBaseAddr(), Command::PutM, ev->getPayload());
        put->setFlag(MemEvent::F_NORESPONSE);
        outstandingEventList_.insert(std::make_pair(put->getID(), OutstandingEvent(put, put->getBaseAddr())));
        notifyListeners(ev);

        if (mshr_.find(put->getBaseAddr()) == mshr_.end()) {
            mshr_.insert(std::make_pair(put->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(put->getID(), put->getCmd()))));
            memBackendConvertor_->handleMemEvent(put);
        } else {
            mshr_.find(put->getBaseAddr())->second.push_back(MSHREntry(put->getID(), put->getCmd()));
        }
    }

    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, ev->getBaseAddr())));
    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
        if (ev->getCmd() == Command::FlushLineInv) {
            cacheStatus_.at(ev->getBaseAddr()/lineSize_) = false;
            ev->setCmd(Command::FlushLine);
        }
        memBackendConvertor_->handleMemEvent(ev);
    } else {
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), ev->getCmd()));
    }
}


/* Response to shootdown indicating block is no longer cached and is clean at memory */
void CoherentMemController::handleAckInv(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    Addr baseAddr = ev->getBaseAddr();

    delete ev;

    /* Update cache status */
    cacheStatus_.at(baseAddr/lineSize_) = false;

    /* Look up request */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;
    OutstandingEvent * outEv = &(outstandingEventList_.find(requestID)->second);

    entry->shootdown = false; /* Shootdown complete */

    if (entry->writebacks.empty()) {
        if (outEv->decrementCount() == 0) { /* All shootdowns for this event are done */
            CustomCmdInfo * info = customCommandHandler_->ready(outEv->request);
            memBackendConvertor_->handleCustomEvent(info);
        }
    }
}


void CoherentMemController::handleFetchResp(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    Addr baseAddr = ev->getBaseAddr();
    
    /* Update cache status */
    cacheStatus_.at(baseAddr/lineSize_) = false;

    /* Look up request */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;
    OutstandingEvent * outEv = &(outstandingEventList_.find(requestID)->second);

    // Write dirty data if needed
    if (ev->getDirty()) {
        MemEvent * write = new MemEvent(this, ev->getAddr(), baseAddr, Command::PutM, ev->getPayload());
        write->setRqstr(ev->getRqstr());
        ev->setFlag(MemEvent::F_NORESPONSE);

        entry->writebacks.insert(write->getID());
        outstandingEventList_.insert(std::make_pair(write->getID(), OutstandingEvent(write, baseAddr)));
        memBackendConvertor_->handleMemEvent(write);
    }

    delete ev;

    entry->shootdown = false; // Complete
    if (entry->writebacks.empty() && outEv->decrementCount() == 0) {
        CustomCmdInfo * info = customCommandHandler_->ready(outEv->request);
        memBackendConvertor_->handleCustomEvent(info);
    }
}


/* Handle NACKs
 * Only invalidations can be nacked since they are they 
 * only request the MemController sends to caches
 */
void CoherentMemController::handleNack(MemEvent * ev) {
    MemEvent * nackedEvent = ev->getNACKedEvent();
    Addr baseAddr = nackedEvent->isAddrGlobal() ? translateToLocal(nackedEvent->getBaseAddr()) : nackedEvent->getBaseAddr();

    /* NACKed event no longer needed due to race between replacement and Inv */
    if (mshr_.find(baseAddr) == mshr_.end()) {
        delete nackedEvent;
        delete ev;
        return;
    }

    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    if (entry->shootdown) { // Still need the shootdown
        /* Compute backoff to avoid excessive NACKing */
        int retries = nackedEvent->getRetries();
        if (retries > 10) retries = 10;
        uint64_t backoff = (0x1 << retries);
        nackedEvent->incrementRetries();

        msgQueue_.insert(std::make_pair(timestamp_ + backoff, nackedEvent));
    } else {
        delete nackedEvent;
    }
    delete ev;
}


/* Handle custom command */
void CoherentMemController::handleCustomCmd(MemEventBase * evb) {
    if (!customCommandHandler_) {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received custom event but no handler loaded. Ev = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), evb->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    CustomCmdMemHandler::MemEventInfo evInfo = customCommandHandler_->receive(evb);
    outstandingEventList_.insert(std::make_pair(evb->getID(), OutstandingEvent(evb, evInfo.addrs)));
    OutstandingEvent * outEv = &(outstandingEventList_.find(evb->getID())->second);

    /* Custom commands may touch multiple (base) addresses */
    for (std::set<Addr>::iterator it = outEv->addrs.begin(); it != outEv->addrs.end(); it++) {
        if (mshr_.find(*it) == mshr_.end()) {
            if (!evInfo.shootdown) {
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(evb->getID(), evb->getCmd()))));
                outEv->decrementCount();
            } else if (doShootdown(*it, evb)) {
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(evb->getID(), evb->getCmd(), evInfo.shootdown))));
            } else {
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(evb->getID(), evb->getCmd()))));
                outEv->decrementCount();
            }
        } else {
            mshr_.find(*it)->second.push_back(MSHREntry(evb->getID(), evb->getCmd(), evInfo.shootdown));
        }
    }

    /* If ready to issue, issue */
    if (outEv->getCount() == 0) {
        CustomCmdInfo * info = customCommandHandler_->ready(evb);
        memBackendConvertor_->handleCustomEvent(info);
    }
}


/* 
 * Do a shootdown for the specified address
 * Return whether shootdown was needed or not
 */
bool CoherentMemController::doShootdown(Addr addr, MemEventBase * ev) {
    if (cacheStatus_.at(addr/lineSize_) == true) {
        Addr globalAddr = translateToGlobal(addr);
        MemEvent * inv = new MemEvent(this, globalAddr, globalAddr, Command::FetchInv, lineSize_);
        inv->setRqstr(ev->getRqstr());
        inv->setDst(ev->getSrc());

        msgQueue_.insert(std::make_pair(timestamp_, inv)); /* Send on next clock. TODO timing needed? */
        return true;
    }
    return false;
}


/* Handle MemResponse */
void CoherentMemController::handleMemResponse(SST::Event::id_type id, uint32_t flags) {
    std::map<SST::Event::id_type,OutstandingEvent>::iterator it = 
      outstandingEventList_.find(id);
    if( it == outstandingEventList_.end() ){
      dbg.fatal(CALL_INFO, -1, "Coherent Memory controller (%s) received unrecgonized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);
    }

    if (outstandingEventList_.find(id)->second.request->getCmd() == Command::CustomReq) {
        finishCustomReq(id, flags);
    } else {
        finishMemReq(id, flags);
    }
}

/* Finish a MemEvent */
void CoherentMemController::finishMemReq(SST::Event::id_type id, uint32_t flags) {
    /* Find the event that returned */
    std::map<SST::Event::id_type,OutstandingEvent>::iterator it = outstandingEventList_.find(id);
    if (it == outstandingEventList_.end())
        dbg.fatal(CALL_INFO, -1, "Memory controller (%s) received unrecgonized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);

    OutstandingEvent * outEv = &(it->second);

    MemEvent * ev = static_cast<MemEvent*>(outEv->request);
    Addr baseAddr = *(outEv->addrs.begin()); /* Only one address */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());

    outstandingEventList_.erase(id);

    if (is_debug_event(ev)) {
        Debug(_L3_, "Memory Controller: %s - Response received to (%s)\n", getName().c_str(), ev->getVerboseString().c_str());
    }

    /* Update backing store */
    bool noncacheable = ev->queryFlag(MemEventBase::F_NONCACHEABLE);
    if (backing_ && (ev->getCmd() == Command::PutM || (ev->getCmd() == Command::GetX && noncacheable)) )
        MemController::writeData(ev);

    /* Special case - response was for a writeback needed for a shootdown */
    if (entry->writebacks.find(id) != entry->writebacks.end()) {
        entry->writebacks.erase(id);
        delete ev;
        if (entry->writebacks.empty() && !entry->shootdown) { /* Request now ready to issue */
            if (outstandingEventList_.find(entry->id)->second.decrementCount() == 0) {
                CustomCmdInfo * info = customCommandHandler_->ready(outstandingEventList_.find(entry->id)->second.request);
                memBackendConvertor_->handleCustomEvent(info);
            }
        }
        return;
    }

    /* Handle regular MemEvent */

    if (ev->queryFlag(MemEventBase::F_NORESPONSE)) {
        delete ev;
        updateMSHR(baseAddr);
        return;
    }

    MemEvent * resp = ev->makeResponse();

    if (resp->getCmd() == Command::GetSResp || (resp->getCmd() == Command::GetXResp && !noncacheable)) {
        MemController::readData(resp);
        if (!noncacheable) resp->setCmd(Command::GetXResp);
    }

    resp->setFlags(flags);

    if (ev->isAddrGlobal()) {
        resp->setBaseAddr(translateToGlobal(ev->getBaseAddr()));
        resp->setAddr(translateToGlobal(ev->getAddr()));
    }

    link_->send(resp);
    delete ev;
    updateMSHR(baseAddr);
}


/*
 * Finish custom command event
 * Calls handler's 'finish' function
 * Handler is responsible for:
 *  1. Translating local (contiguous/start at 0) addresses to global using translateAddrToGlobal()
 *  2. Updating or reading from backing store using readData() and writeData()
 *  3. Creating and returning response message if needed
 */
void CoherentMemController::finishCustomReq(SST::Event::id_type id, uint32_t flags) {
    OutstandingEvent * outEv = &(outstandingEventList_.find(id)->second);
    MemEventBase * ev = outEv->request;

    MemEventBase * resp = customCommandHandler_->finish(ev, flags);
    if (resp != nullptr)
        link_->send(resp);

    delete ev;

    /* Clean up states */
    for (std::set<Addr>::iterator it = outEv->addrs.begin(); it != outEv->addrs.end(); it++) {
        updateMSHR(*it);
    }
    outstandingEventList_.erase(id);
}


/*
 * Update MSHR state
 * 1. Remove finished event
 * 2. Start next event
 */
void CoherentMemController::updateMSHR(Addr baseAddr) {
    // Remove finished event
    mshr_.find(baseAddr)->second.pop_front();

    /* Delete address if no more events */
    if (mshr_.find(baseAddr)->second.empty()) {
        mshr_.erase(baseAddr);
    
    /* Start next event */
    } else { 
        MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
        OutstandingEvent * outEv = &(outstandingEventList_.find(entry->id)->second);

        /* If custom request check whether it's ready to replay */
        if (outEv->request->getCmd() == Command::CustomReq) {
            if (entry->shootdown && doShootdown(baseAddr, outEv->request)) { /* Start shootdown */
                return;
            } else {
                entry->shootdown = false; 
                if (entry->writebacks.empty() && outEv->decrementCount() == 0) { /* Command ready to issue */
                    CustomCmdInfo * info = customCommandHandler_->ready(outEv->request);
                    memBackendConvertor_->handleCustomEvent(info);
                }
            }
        } else { /* MemEvent */
            replayMemEvent(static_cast<MemEvent*>((outstandingEventList_.find(entry->id)->second).request));
        }
    }
}


void CoherentMemController::replayMemEvent(MemEvent * ev) {
    switch (ev->getCmd()) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            if (!ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
                cacheStatus_.at(ev->getBaseAddr()/lineSize_) = true;
            }
            memBackendConvertor_->handleMemEvent(ev);
            break;
        case Command::PutM:
            cacheStatus_.at(ev->getBaseAddr()/lineSize_) = directory_;
            memBackendConvertor_->handleMemEvent(ev);
            break;
        case Command::FlushLineInv:
            cacheStatus_.at(ev->getBaseAddr()/lineSize_) = false;
            ev->setCmd(Command::FlushLine);
        case Command::FlushLine:
            memBackendConvertor_->handleMemEvent(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Attempt to replay unknown event: %s. Time = %" PRIu64 "ns.\n", 
                    getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
            break;
    }
}

