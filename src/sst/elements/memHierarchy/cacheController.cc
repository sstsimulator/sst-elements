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

#include <sst/core/sst_config.h>
#include <sst/core/params.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/timeLord.h>

#include "cacheController.h"
#include "memEvent.h"
#include "mshr.h"
#include "coherencemgr/coherenceController.h"


using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

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
    if (mem_h_is_debug_event((event))) {
        dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Recv    (%s)\n",
                getCurrentSimCycle(), timestamp_, getName().c_str(), event->getVerboseString().c_str());
        fflush(stdout);
    }

    eventBuffer_.push_back(event);
    //printf("DBG: %s, inserted <%" PRIu64 ", %d>, size=%zu\n", getName().c_str(), event->getID().first, event->getID().second, eventBuffer_.size());

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

    // Record the time at which requests arrive for latency statistics
    coherenceMgr_->recordIncomingRequest(event);

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
        if (mem_h_is_debug_event((*it))) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Retry   (%s)\n",
                    getCurrentSimCycle(), timestamp_, getName().c_str(), (*it)->getVerboseString().c_str());
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
        Event::id_type id = (*it)->getID();
        Command cmd = (*it)->getCmd();
        if (mem_h_is_debug_event((*it))) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                    getCurrentSimCycle(), timestamp_, getName().c_str(), (*it)->getVerboseString().c_str());
            fflush(stdout);
        }
        if (processEvent(*it, false)) {
            accepted++;
            statRecvEvents->addData(1);
            it = eventBuffer_.erase(it);
            //printf("DBG: %s, erased <%" PRIu64 ", %d>, it=%d, size=%zu\n", getName().c_str(), id.first, id.second, it == eventBuffer_.end(), eventBuffer_.size());
        } else {
            it++;
            //printf("DBG: %s, left <%" PRIu64 ", %d>, it=%d, size=%zu\n", getName().c_str(), id.first, id.second, it == eventBuffer_.end(), eventBuffer_.size());
        }
    }
    while (!prefetchBuffer_.empty()) {
        if (mem_h_is_debug_event(prefetchBuffer_.front())) {
            dbg_->debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Pref    (%s)\n",
                    getCurrentSimCycle(), timestamp_, getName().c_str(), prefetchBuffer_.front()->getVerboseString().c_str());
            fflush(stdout);
        }
        if (accepted != maxRequestsPerCycle_ && processEvent(prefetchBuffer_.front(), false)) {
            accepted++;
            // Accepted prefetches are profiled in the coherence manager
	    prefetchBuffer_.pop();
        } else {
            statPrefetchDrop->addData(1);
            coherenceMgr_->removeRequestRecord(prefetchBuffer_.front()->getID());
	    MemEventBase* ev = prefetchBuffer_.front();
	    prefetchBuffer_.pop();
	    delete ev;
        }
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
    if (clockIsOn_) return;
    Cycle_t time = reregisterClock(defaultTimeBase_, clockHandler_);
    timestamp_ = time - 1;
    coherenceMgr_->updateTimestamp(timestamp_);
    int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
    statMSHROccupancy->addDataNTimes(cyclesOff, mshr_->getSize());
    //dbg_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), getCurrentSimCycle(), timestamp_, getCurrentSimTimeNano());
    clockIsOn_ = true;
}

void Cache::turnClockOff() {
    //dbg_->debug(_L3_, "%s turning clock OFF at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), getCurrentSimCycle(), timestamp_, getCurrentSimTimeNano());
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
bool Cache::processEvent(MemEventBase* ev, bool retry) {
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
        if (mem_h_is_debug_addr(addr)) {
            std::stringstream id;
            id << "<" << event->getID().first << "," << event->getID().second << ">";
            dbg_->debug(_L5_, "A: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s\n",
                    getCurrentSimCycle(), timestamp_, getName().c_str(), CommandString[(int)event->getCmd()],
                    addr, id.str().c_str(), "", "", "Stall", "(bank busy)");
        }
        return false;
    }

    bool dbgevent = mem_h_is_debug_event(event);
    bool accepted = false;

    switch (event->getCmd()) {
        case Command::GetS:
            accepted = coherenceMgr_->handleGetS(event, retry);
            break;
        case Command::GetX:
            accepted = coherenceMgr_->handleGetX(event, retry);
            break;
        case Command::Write:
            accepted = coherenceMgr_->handleWrite(event, retry);
            break;
        case Command::GetSX:
            accepted = coherenceMgr_->handleGetSX(event, retry);
            break;
        case Command::FlushLine:
            accepted = coherenceMgr_->handleFlushLine(event, retry);
            break;
        case Command::FlushLineInv:
            accepted = coherenceMgr_->handleFlushLineInv(event, retry);
            break;
        case Command::FlushAll:
            accepted = coherenceMgr_->handleFlushAll(event, retry);
            break;
        case Command::GetSResp:
            accepted = coherenceMgr_->handleGetSResp(event, retry);
            break;
        case Command::WriteResp:
            accepted = coherenceMgr_->handleWriteResp(event, retry);
            break;
        case Command::GetXResp:
            accepted = coherenceMgr_->handleGetXResp(event, retry);
            break;
        case Command::FlushLineResp:
            accepted = coherenceMgr_->handleFlushLineResp(event, retry);
            break;
        case Command::FlushAllResp:
            accepted = coherenceMgr_->handleFlushAllResp(event, retry);
            break;
        case Command::PutS:
            accepted = coherenceMgr_->handlePutS(event, retry);
            break;
        case Command::PutX:
            accepted = coherenceMgr_->handlePutX(event, retry);
            break;
        case Command::PutE:
            accepted = coherenceMgr_->handlePutE(event, retry);
            break;
        case Command::PutM:
            accepted = coherenceMgr_->handlePutM(event, retry);
            break;
        case Command::FetchInv:
            accepted = coherenceMgr_->handleFetchInv(event, retry);
            break;
        case Command::FetchInvX:
            accepted = coherenceMgr_->handleFetchInvX(event, retry);
            break;
        case Command::ForceInv:
            accepted = coherenceMgr_->handleForceInv(event, retry);
            break;
        case Command::Inv:
            accepted = coherenceMgr_->handleInv(event, retry);
            break;
        case Command::Fetch:
            accepted = coherenceMgr_->handleFetch(event, retry);
            break;
        case Command::FetchResp:
            accepted = coherenceMgr_->handleFetchResp(event, retry);
            break;
        case Command::FetchXResp:
            accepted = coherenceMgr_->handleFetchXResp(event, retry);
            break;
        case Command::AckInv:
            accepted = coherenceMgr_->handleAckInv(event, retry);
            break;
        case Command::AckPut:
            accepted = coherenceMgr_->handleAckPut(event, retry);
            break;
        case Command::ForwardFlush:
            accepted = coherenceMgr_->handleForwardFlush(event, retry);
            break;
        case Command::AckFlush:
            accepted = coherenceMgr_->handleAckFlush(event, retry);
            break;
        case Command::UnblockFlush:
            accepted = coherenceMgr_->handleUnblockFlush(event, retry);
            break;
        case Command::NACK:
            accepted = coherenceMgr_->handleNACK(event, retry);
            break;
        case Command::NULLCMD:
            accepted = coherenceMgr_->handleNULLCMD(event, retry);
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

    if (CommandRouteByAddress[(int)event->getCmd()]) { /* These events don't have a destination already */
        if (!(event->queryFlag(MemEvent::F_NORESPONSE))) {
            noncacheableResponseDst_.insert(std::make_pair(event->getID(), event->getSrc()));
        }
        coherenceMgr_->forwardByAddress(event);
    } else {
        std::map<SST::Event::id_type,std::string>::iterator it = noncacheableResponseDst_.find(event->getResponseToID());
        if (it == noncacheableResponseDst_.end()) {
            out_->fatal(CALL_INFO, 01, "%s, Error: noncacheable response received does not match a request. Event: (%s). Time: %" PRIu64 "\n",
                    getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
        event->setDst(it->second);
        coherenceMgr_->forwardByDestination(event);
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
        SimTime_t startTime = getTimeConverter("1ns")->convertFromCoreTime(entry->getStartTime());
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
            linkDown_->sendUntimedData(coherenceMgr_->getInitCoherenceEvent());

        while(MemEventInit *event = linkDown_->recvUntimedData()) {
            if (event->getCmd() == Command::NULLCMD  || ( mem_h_is_debug_event(event) && event->getInitCmd() == MemEventInit::InitCommand::Data)) {
                dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                        getName().c_str(), event->getVerboseString().c_str());
            }

            /* If event is from one of our destinations, update parameters - link only returns events from destinations */
            if (event->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(event);
                processInitCoherenceEvent(eventC, linkDown_->isSource(eventC->getSrc()));
                delete event;
            } else if (event->getInitCmd() == MemEventInit::InitCommand::Endpoint) {
                event->setSrc(getName());
                linkDown_->sendUntimedData(event);
            } else if (event->getInitCmd() == MemEventInit::InitCommand::Data) {
                if (BasicCommandClassArr[(int)event->getCmd()] == BasicCommandClass::Request) {
                    // Forward by addr (mmio likely)
                    if (event->getCmd() == Command::GetS)
                        init_requests_.insert(std::make_pair(event->getID(), event->getSrc()));
                    event->setSrc(getName());
                    linkDown_->sendUntimedData(event, false, true);
                } else {
                    // Forward directly via lookup
                    event->setSrc(getName());
                    event->setDst(init_requests_.find(event->getID())->second);
                    init_requests_.erase(event->getID());
                    linkDown_->sendUntimedData(event, false, false);
                }
            }
        }
        return;
    }

    // Case: 2 links
    linkUp_->init(phase);
    linkDown_->init(phase);

    if (!phase) {
        linkUp_->sendUntimedData(coherenceMgr_->getInitCoherenceEvent());
        linkDown_->sendUntimedData(coherenceMgr_->getInitCoherenceEvent());
    }

    while (MemEventInit * event = linkUp_->recvUntimedData()) {
        if (event->getCmd() == Command::NULLCMD) {
            dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                    getName().c_str(), event->getVerboseString().c_str());
            if (event->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                coherenceMgr_->hasUpperLevelCacheName(event->getSrc());
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(event);
                processInitCoherenceEvent(eventC, true);
                delete event;
            } else if (event->getInitCmd() == MemEventInit::InitCommand::Endpoint ) {
                event->setSrc(getName());
                linkDown_->sendUntimedData(event);
            }
        } else if (event->getInitCmd() == MemEventInit::InitCommand::Data) {
            if (mem_h_is_debug_event((event))) {
                dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                        getName().c_str(), event->getVerboseString().c_str());
            }
            if (BasicCommandClassArr[(int)event->getCmd()] == BasicCommandClass::Request) {
                if (event->getCmd() == Command::GetS)
                    init_requests_.insert(std::make_pair(event->getID(), event->getSrc()));
                event->setSrc(getName());
                linkDown_->sendUntimedData(event, false, true);
            } else {
                event->setSrc(getName());
                event->setDst(init_requests_.find(event->getID())->second);
                init_requests_.erase(event->getID());
                linkDown_->sendUntimedData(event, false, false);
            }
        } else {
            delete event;
        }
    }

    while (MemEventInit * event = linkDown_->recvUntimedData()) {
        if (event->getCmd() == Command::NULLCMD) {
            dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                    getName().c_str(), event->getVerboseString().c_str());
            if (linkDown_->isDest(event->getSrc()) && event->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(event);
                processInitCoherenceEvent(eventC, false);
                delete event;
            } else if (event->getInitCmd() == MemEventInitEndpoint::InitCommand::Endpoint) {
                event->setSrc(getName());
                linkUp_->sendUntimedData(event);
            }
        } else if (event->getInitCmd() == MemEventInit::InitCommand::Data) {
            if (mem_h_is_debug_event((event))) {

                dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                        getName().c_str(), event->getVerboseString().c_str());
            }
            if (BasicCommandClassArr[(int)event->getCmd()] == BasicCommandClass::Request) {
                if (event->getCmd() == Command::GetS)
                    init_requests_.insert(std::make_pair(event->getID(), event->getSrc()));
                event->setSrc(getName());
                linkUp_->sendUntimedData(event, false, true);
            } else {
                event->setSrc(getName());
                event->setDst(init_requests_.find(event->getID())->second);
                init_requests_.erase(event->getID());
                linkUp_->sendUntimedData(event, false, false);
            }
        } else {
            delete event;
        }
    }
}

/* Facilitate sharing InitCoherenceEvents between coherence managers */
void Cache::processInitCoherenceEvent(MemEventInitCoherence* event, bool src) {
    coherenceMgr_->processInitCoherenceEvent(event, src);
}

void Cache::setup() {
    // Check that our sources and destinations exist or configure if needed
    linkUp_->setup();
    if (linkUp_ != linkDown_)
        linkDown_->setup();

    // Enqueue the first wakeup event to check for deadlock
    if (timeout_ != 0)
        timeoutSelfLink_->send(1, nullptr);
    coherenceMgr_->setup();
}

void Cache::complete(unsigned int phase) {
    if ( phase == 0 ) {
        coherenceMgr_->beginCompleteStage();
    }

    // Case: 1 link
    if (linkUp_ == linkDown_) {
        linkDown_->complete(phase);

        while(MemEventInit *event = linkDown_->recvUntimedData()) {
            dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                    getName().c_str(), event->getVerboseString().c_str());
            coherenceMgr_->processCompleteEvent(event, linkUp_, linkDown_);
        }
        return;
    }

    // Case: 2 links
    linkUp_->complete(phase);
    linkDown_->complete(phase);

    while (MemEventInit * event = linkUp_->recvUntimedData()) {
        dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                getName().c_str(), event->getVerboseString().c_str());
        coherenceMgr_->processCompleteEvent(event, linkUp_, linkDown_);
    }

    while (MemEventInit * event = linkDown_->recvUntimedData()) {
        dbg_->debug(_L10_, "U: %-20s   Event:Untimed   (%s)\n",
                getName().c_str(), event->getVerboseString().c_str());
        coherenceMgr_->processCompleteEvent(event, linkUp_, linkDown_);
    }
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

void Cache::serialize_order(SST::Core::Serialization::serializer& ser) {
    SST::Component::serialize_order(ser);

    SST_SER(listeners_);
    SST_SER(linkUp_);
    SST_SER(linkDown_);
    SST_SER(prefetchSelfLink_);
    SST_SER(timeoutSelfLink_);
    SST_SER(mshr_);
    SST_SER(coherenceMgr_);
    SST_SER(init_requests_);
    SST_SER(prefetchDelay_);
    SST_SER(lineSize_);
    SST_SER(allNoncacheableRequests_);
    SST_SER(maxRequestsPerCycle_);
    SST_SER(region_);
    SST_SER(timeout_);
    SST_SER(maxOutstandingPrefetch_);
    SST_SER(banked_);

    SST_SER(clockHandler_);
    SST_SER(defaultTimeBase_);
    SST_SER(clockIsOn_);
    SST_SER(clockUpLink_);
    SST_SER(clockDownLink_);
    SST_SER(lastActiveClockCycle_);

    SST_SER(timestamp_);
    SST_SER(requestsThisCycle_);
    SST_SER(bankStatus_);
    SST_SER(addrsThisCycle_);
    SST_SER(retryBuffer_);
    SST_SER(eventBuffer_);
    SST_SER(prefetchBuffer_);
    SST_SER(noncacheableResponseDst_);

    SST_SER(out_);
    SST_SER(dbg_);
    SST_SER(debug_addr_filter_);

    SST_SER(statMSHROccupancy);
    SST_SER(statBankConflicts);
    SST_SER(statPrefetchDrop);
    SST_SER(statPrefetchRequest);
    SST_SER(statRecvEvents);
    SST_SER(statRetryEvents);
    SST_SER(statUncacheRecv);
    SST_SER(statCacheRecv);

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        coherenceMgr_->registerClockEnableFunction(std::bind(&Cache::turnClockOn, this));
    }
}
