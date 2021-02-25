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

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/timeLord.h>

#include "cacheController.h"
#include "memEvent.h"
#include "mshr.h"
#include "coherencemgr/coherenceController.h"


using namespace SST;
using namespace SST::MemHierarchy;

/**************************************************************************
 * Handlers for various links
 * linkUp/down -> handleEvent()
 * prefetcher -> handlePrefetchEvent()
 * prefetch self link -> processPrefetchEvent()
 **************************************************************************/

/* Handle incoming event on the cache links */
void Cache::handleEvent(SST::Event * ev) {
    MemEventBase* event = static_cast<MemEventBase*>(ev);
    if (!clockIsOn_)
        turnClockOn();

    // Record the time at which requests arrive for latency statistics
    if (CommandClassArr[(int)event->getCmd()] == CommandClass::Request && !CommandWriteback[(int)event->getCmd()])
        coherenceMgr_->recordIncomingRequest(event);

    // Record that an event was received
    if (MemEventTypeArr[(int)event->getCmd()] != MemEventType::Cache || event->queryFlag(MemEventBase::F_NONCACHEABLE)) {
        statUncacheRecv[(int)event->getCmd()]->addData(1);
    } else {
        statCacheRecv[(int)event->getCmd()]->addData(1);
    }

    eventBuffer_.push_back(event);
}

/* 
 * Handle event from cache listener (prefetcher) 
 * -> Delay prefetch using a self link since prefetcher can 
 *  return a prefetch request in the same cycle it identifies
 *  a prefetch target
 */
void Cache::handlePrefetchEvent(SST::Event * ev) {
    prefetchSelfLink_->send(prefetchDelay_, ev);
}

/* Handle event from prefetch self link */
void Cache::processPrefetchEvent(SST::Event * ev) {
    MemEvent * event = static_cast<MemEvent*>(ev);
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    event->setRqstr(getName());
    event->setSrc(getName());

    if (!clockIsOn_) {
        turnClockOn();
    }

    // Record received prefetch
    statPrefetchRequest->addData(1);
    statCacheRecv[(int)event->getCmd()]->addData(1);
    prefetchBuffer_.push(event);
}

/**************************************************************************
 * Clock handler and management
 **************************************************************************/

/* Clock handler */
bool Cache::clockTick(Cycle_t time) {
    timestamp_++;

    // Drain any outgoing messages
    bool idle = coherenceMgr_->sendOutgoingEvents();

    if (clockUpLink_) {
        idle &= linkUp_->clock();
    }
    if (clockDownLink_) {
        idle &= linkDown_->clock();
    }

    // MSHR occupancy
    statMSHROccupancy->addData(mshr_->getSize());

    // Clear bank status to prepare for event handling
    for (unsigned int bank = 0; bank < bankStatus_.size(); bank++)
        bankStatus_[bank] = false;

    addrsThisCycle_.clear();

    // Handle events from each of the buffers
    // 1. Retry buffer      -> Events that need to be retried, e.g., were stalled due to a pending action that is now resolved
    // 2. Event buffer      -> Incoming (new) events
    // 3. Prefetch buffer   -> Drop any prefetch that can't be handled immediately

    int accepted = 0;
    size_t entries = retryBuffer_.size();

    std::list<MemEventBase*>::iterator it = retryBuffer_.begin();
    while (it != retryBuffer_.end()) {
        if (accepted == maxRequestsPerCycle_)
            break;
        if (is_debug_event((*it))) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Retry   (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), (*it)->getVerboseString().c_str());
            fflush(stdout);
        }
        if (processEvent(*it, true)) {
            accepted++;
            statRetryEvents->addData(1);
            it = retryBuffer_.erase(it);
        } else {
            it++;
        }
    }

    // Event buffer has both requests and responses
    // Deadlock will not occur because an event cannot indefinitely block another one
    // 1. An event can be accepted, in which case a later response moves up the queue
    // 2. An event can be rejected, in which case we check the next one with no penalty (doesn't block a later response)
    it = eventBuffer_.begin();
    while (it != eventBuffer_.end()) {
        if (accepted == maxRequestsPerCycle_)
            break;
        Command cmd = (*it)->getCmd();
        if (is_debug_event((*it))) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), (*it)->getVerboseString().c_str());
            fflush(stdout);
        }
        if (processEvent(*it, false)) {
            accepted++;
            statRecvEvents->addData(1);
            it = eventBuffer_.erase(it);
        } else {
            it++;
        }
    }
    while (!prefetchBuffer_.empty()) {
        if (is_debug_event(prefetchBuffer_.front())) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Pref    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), prefetchBuffer_.front()->getVerboseString().c_str());
            fflush(stdout);
        }
        if (accepted != maxRequestsPerCycle_ && processEvent(prefetchBuffer_.front(), false)) {
            accepted++;
            // Accepted prefetches are profiled in the coherence manager
        } else {
            statPrefetchDrop->addData(1);
        }
        prefetchBuffer_.pop();
    }

    // Push any events that need to be retried next cycle onto the retry buffer
    std::vector<MemEventBase*>* rBuf = coherenceMgr_->getRetryBuffer();
    std::copy( rBuf->begin(), rBuf->end(), std::back_inserter(retryBuffer_) );
    coherenceMgr_->clearRetryBuffer();

    idle &= coherenceMgr_->checkIdle();

    // Disable lower-level cache clocks if they're idle
    if (eventBuffer_.empty() && retryBuffer_.empty() && idle) {
        turnClockOff();
        return true;
    }

    // Keep the clock on
    return false;
}

void Cache::turnClockOn() {
    Cycle_t time = reregisterClock(defaultTimeBase_, clockHandler_);
    timestamp_ = time - 1;
    coherenceMgr_->updateTimestamp(timestamp_);
    int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
    for (int64_t i = 0; i < cyclesOff; i++) {           // TODO more efficient way to do this? Don't want to add in one-shot or we get weird averages/sum sq.
        statMSHROccupancy->addData(mshr_->getSize());
    }
    //d_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), time, timestamp_, getCurrentSimTimeNano());
    clockIsOn_ = true;
}

void Cache::turnClockOff() {
    clockIsOn_ = false;
    lastActiveClockCycle_ = timestamp_;
}

/**************************************************************************
 * Event processing
 **************************************************************************/

/*
 * Main function for processing events
 * - Dispatches events to appropriate handlers
 *   -> Cache events go to coherence manager
 *   -> Noncacheable events are handled by processNoncacheable()
 * - Arbitrates bank access
 *
 *   Returns: whether event was accepted/can be popped off event queue
 */
bool Cache::processEvent(MemEventBase* ev, bool inMSHR) {
    // Global noncacheable request flag
    if (allNoncacheableRequests_) {
        ev->setFlag(MemEvent::F_NONCACHEABLE);
    }

    if (MemEventTypeArr[(int)ev->getCmd()] != MemEventType::Cache || ev->queryFlag(MemEventBase::F_NONCACHEABLE)) {
        processNoncacheable(ev);
        return true;
    }

    /* Handle cache events */
    MemEvent * event = static_cast<MemEvent*>(ev);

    Addr addr = event->getBaseAddr();

    /* Arbitrate cache access - bank/link. Reject request on failure */
    if (!arbitrateAccess(addr)) { // Disallow multiple requests to same line and/or bank in a single cycle
        if (is_debug_addr(addr)) {
            std::stringstream id;
            id << "<" << event->getID().first << "," << event->getID().second << ">";
            dbg_->debug(_L5_, "A: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), CommandString[(int)event->getCmd()],
                    addr, id.str().c_str(), "", "", "Stall", "(bank busy)");
        }
        return false;
    }

    bool dbgevent = is_debug_event(event);
    bool accepted = false;

    switch (event->getCmd()) {
        case Command::GetS:
            accepted = coherenceMgr_->handleGetS(event, inMSHR);
            break;
        case Command::GetX:
            accepted = coherenceMgr_->handleGetX(event, inMSHR);
            break;
        case Command::GetSX:
            accepted = coherenceMgr_->handleGetSX(event, inMSHR);
            break;
        case Command::FlushLine:
            accepted = coherenceMgr_->handleFlushLine(event, inMSHR);
            break;
        case Command::FlushLineInv:
            accepted = coherenceMgr_->handleFlushLineInv(event, inMSHR);
            break;
        case Command::GetSResp:
            accepted = coherenceMgr_->handleGetSResp(event, inMSHR);
            break;
        case Command::GetXResp:
            accepted = coherenceMgr_->handleGetXResp(event, inMSHR);
            break;
        case Command::FlushLineResp:
            accepted = coherenceMgr_->handleFlushLineResp(event, inMSHR);
            break;
        case Command::PutS:
            accepted = coherenceMgr_->handlePutS(event, inMSHR);
            break;
        case Command::PutX:
            accepted = coherenceMgr_->handlePutX(event, inMSHR);
            break;
        case Command::PutE:
            accepted = coherenceMgr_->handlePutE(event, inMSHR);
            break;
        case Command::PutM:
            accepted = coherenceMgr_->handlePutM(event, inMSHR);
            break;
        case Command::FetchInv:
            accepted = coherenceMgr_->handleFetchInv(event, inMSHR);
            break;
        case Command::FetchInvX:
            accepted = coherenceMgr_->handleFetchInvX(event, inMSHR);
            break;
        case Command::ForceInv:
            accepted = coherenceMgr_->handleForceInv(event, inMSHR);
            break;
        case Command::Inv:
            accepted = coherenceMgr_->handleInv(event, inMSHR);
            break;
        case Command::Fetch:
            accepted = coherenceMgr_->handleFetch(event, inMSHR);
            break;
        case Command::FetchResp:
            accepted = coherenceMgr_->handleFetchResp(event, inMSHR);
            break;
        case Command::FetchXResp:
            accepted = coherenceMgr_->handleFetchXResp(event, inMSHR);
            break;
        case Command::AckInv:
            accepted = coherenceMgr_->handleAckInv(event, inMSHR);
            break;
        case Command::AckPut:
            accepted = coherenceMgr_->handleAckPut(event, inMSHR);
            break;
        case Command::NACK:
            accepted = coherenceMgr_->handleNACK(event, inMSHR);
            break;
        case Command::NULLCMD:
            accepted = coherenceMgr_->handleNULLCMD(event, inMSHR);
            break;
        default:
            out_->fatal(CALL_INFO, -1, "%s, Error: Received an unsupported command. Event: %s. Time = %" PRIu64 "ns.\n",
                    getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (dbgevent)
        coherenceMgr_->printDebugInfo();

    if (accepted)
        updateAccessStatus(addr);

    return accepted;
}

/* Arbitrate for access. Return whether successful */
bool Cache::arbitrateAccess(Addr addr) {
    if (!banked_) {
        if (addrsThisCycle_.find(addr) == addrsThisCycle_.end()) {
            return true;
        }
        return false;
    }

    Addr bank = coherenceMgr_->getBank(addr);
    if (bankStatus_[bank]) {
        statBankConflicts->addData(1);
        return false;
    } else {
        return true;
    }
}

/* Block banks that have been accessed */
void Cache::updateAccessStatus(Addr addr) {
    addrsThisCycle_.insert(addr);
    if (banked_) {
        Addr bank = coherenceMgr_->getBank(addr);
        bankStatus_[bank] = true;
    }
}


/* For handling non-cache commands (including NONCACHEABLE data requests) */
void Cache::processNoncacheable(MemEventBase* event) {

    if (CommandCPUSide[(int)event->getCmd()]) {
        if (!(event->queryFlag(MemEvent::F_NORESPONSE))) {
            noncacheableResponseDst_.insert(std::make_pair(event->getID(), event->getSrc()));
        }
        coherenceMgr_->forwardTowardsMem(event);
    } else {
        std::map<SST::Event::id_type,std::string>::iterator it = noncacheableResponseDst_.find(event->getResponseToID());
        if (it == noncacheableResponseDst_.end()) {
            out_->fatal(CALL_INFO, 01, "%s, Error: noncacheable response received does not match a request. Event: (%s). Time: %" PRIu64 "\n",
                    getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
        coherenceMgr_->forwardTowardsCPU(event, it->second);
        noncacheableResponseDst_.erase(it);
    }
}

/**************************************************************************
 * Timeout checking
 **************************************************************************/

/* Handler for timeoutSelfLink_ */
void Cache::timeoutWakeup(SST::Event * ev) {
    checkTimeout();
    delete ev; // TODO do we need to delete this?
    if (timeout_ != 0)
        timeoutSelfLink_->send(1, nullptr);
}

/* Check that no MSHR entries have been waiting in excess of the timeout limit */
void Cache::checkTimeout() {
    MSHREntry * entry = mshr_->getOldestEntry();

    if (entry) {
        SimTime_t curTime = getCurrentSimTimeNano();
        SimTime_t startTime = (getSimulation()->getTimeLord()->getNano())->convertFromCoreTime(entry->getStartTime());
        SimTime_t waitTime = curTime - startTime;
        if (waitTime > timeout_) {
            out_->fatal(CALL_INFO, -1, "%s, Error: Maximum cache timeout reached - potential deadlock or other error. Event: %s. Current time: %" PRIu64 "ns. Event start time: %" PRIu64 "ns.\n",
                    getName().c_str(), entry->getEvent()->getVerboseString().c_str(), curTime, startTime);
        }
    }
}

/**************************************************************************
 * Simulation flow
 * - init: coordinate protocols/configuration between components
 * - setup
 * - finish
 * - printStatus - called on SIGUSR2 and emergencyShutdown
 * - emergenyShutdown - called on fatal()
 **************************************************************************/

void Cache::init(unsigned int phase) {

    // Case: 1 link
    if (linkUp_ == linkDown_) {
        linkDown_->init(phase);

        // Exchange coherence configuration information
        if (!phase)
            linkDown_->sendInitData(coherenceMgr_->getInitCoherenceEvent());

        while(MemEventInit *event = linkDown_->recvInitData()) {
            if (event->getCmd() == Command::NULLCMD) {
                dbg_->debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                        getName().c_str(), event->getVerboseString().c_str());
            }
            /* If event is from one of our destinations, update parameters - link only returns events from destinations */
            if (event->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(event);
                processInitCoherenceEvent(eventC, linkDown_->isSource(eventC->getSrc()));
            }
            delete event;
        }
        return;
    }

    // Case: 2 links
    linkUp_->init(phase);
    linkDown_->init(phase);

    if (!phase) {
        linkUp_->sendInitData(coherenceMgr_->getInitCoherenceEvent());
        linkDown_->sendInitData(coherenceMgr_->getInitCoherenceEvent());
    }

    while (MemEventInit * memEvent = linkUp_->recvInitData()) {
        if (memEvent->getCmd() == Command::NULLCMD) {
            dbg_->debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), memEvent->getVerboseString().c_str());
            coherenceMgr_->hasUpperLevelCacheName(memEvent->getSrc());
            if (memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(memEvent);
                processInitCoherenceEvent(eventC, true);
            }
        } else {
            dbg_->debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), memEvent->getVerboseString().c_str());
            MemEventInit * mEv = memEvent->clone();
            mEv->setSrc(getName());
            mEv->setDst(linkDown_->findTargetDestination(mEv->getRoutingAddress()));
            linkDown_->sendInitData(mEv);
        }
        delete memEvent;
    }

    while (MemEventInit * memEvent = linkDown_->recvInitData()) {
        if (memEvent->getCmd() == Command::NULLCMD) {
            dbg_->debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), memEvent->getVerboseString().c_str());

            if (linkDown_->isDest(memEvent->getSrc()) && memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(memEvent);
                processInitCoherenceEvent(eventC, false);
            }
        }
        delete memEvent;
    }
}

/* Facilitate sharing InitCoherenceEvents between coherence managers */
void Cache::processInitCoherenceEvent(MemEventInitCoherence* event, bool src) {
    coherenceMgr_->processInitCoherenceEvent(event, src);
}

void Cache::setup() {
    // Check that our sources and destinations exist or configure if needed

    linkUp_->setup();
    if (linkUp_ != linkDown_) linkDown_->setup();

    // Enqueue the first wakeup event to check for deadlock
    if (timeout_ != 0)
        timeoutSelfLink_->send(1, nullptr);
}


void Cache::finish() {
    if (!clockIsOn_) { // Correct statistics
        turnClockOn();
    }
    for (int i = 0; i < listeners_.size(); i++)
        listeners_[i]->printStats(*out_);
    linkDown_->finish();
    if (linkUp_ != linkDown_) linkUp_->finish();
}


void Cache::printStatus(Output &out) {
    out.output("MemHierarchy::Cache %s\n", getName().c_str());
    out.output("  Clock is %s. Last active cycle: %" PRIu64 "\n", clockIsOn_ ? "on" : "off", timestamp_);
    out.output("  Events in queues: Retry = %zu, Event = %zu, Prefetch = %zu\n", retryBuffer_.size(), eventBuffer_.size(), prefetchBuffer_.size());
    if (mshr_) {
        out.output("  MSHR Status:\n");
        mshr_->printStatus(out);
    }
    if (linkUp_ && linkUp_ != linkDown_) {
        out.output("  Up link status: ");
        linkUp_->printStatus(out);
        out.output("  Down link status: ");
    } else {
        out.output("  Link status: ");
    }
    if (linkDown_) linkDown_->printStatus(out);

    out.output("  Cache coherence manager and array:\n");
    coherenceMgr_->printStatus(out);
    out.output("End MemHierarchy::Cache\n\n");
}

void Cache::emergencyShutdown() {
    if (out_->getVerboseLevel() > 1) {
        if (out_->getOutputLocation() == Output::STDOUT)
            out_->setOutputLocation(Output::STDERR);
        printStatus(*out_);
        if (linkUp_ && linkUp_ != linkDown_) {
            out_->output("  Checking for unreceived events on up link: \n");
            linkUp_->emergencyShutdownDebug(*out_);
            out_->output("  Checking for unreceived events on down link: \n");
        } else {
            out_->output("  Checking for unreceived events on link: \n");
        }
        linkDown_->emergencyShutdownDebug(*out_);
    }
}

