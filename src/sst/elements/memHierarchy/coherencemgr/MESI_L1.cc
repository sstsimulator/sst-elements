// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
#include <vector>
#include "coherencemgr/MESI_L1.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Coherence Controller
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/*
 * Handle GetS (load/read) request
 * GetS may be the start of an LLSC
 */
bool MESIL1::handleGetS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ?  line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (is_debug_addr(addr)) {
        eventDI.prefill(event->getID(), Command::GetS, localPrefetch, addr, state);
        eventDI.reason = "hit";
    }

    switch (state) {
        case I: /* Miss */
            status = processCacheMiss(event, line, inMSHR); // Attempt to allocate an MSHR entry and/or line

            if (status == MemEventStatus::OK) {
                line = cacheArray_->lookup(addr, false);
                //eventProfileAndNotify(event, I, NotifyAccessType::READ, NotifyResultType::MISS, true, LatType::MISS);
                if (!mshr_->getProfiled(addr)) {
                    recordLatencyType(event->getID(), LatType::MISS);
                    stat_eventState[(int)Command::GetS][I]->addData(1);
                    stat_miss[0][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IS);
                line->setTimestamp(sendTime);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case S: /* Hit */
        case E:
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                recordLatencyType(event->getID(), LatType::HIT);
                stat_eventState[(int)Command::GetS][state]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            }

            if (localPrefetch) {
                statPrefetchRedundant->addData(1); // Unneccessary prefetch
                recordPrefetchLatency(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(line, statPrefetchHit);

            if (event->isLoadLink())
                line->atomicStart();
            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime - 1);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (is_debug_event(event)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false : true;
}


/*
 * Handle GetX (store/write) requests
 * GetX may also be a store-conditional or write-unlock
 */
bool MESIL1::handleGetX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetX, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        line->setState(M);
    }

    MemEventStatus status = MemEventStatus::OK;
    bool atomic = true;
    uint64_t sendTime = 0;

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR);

            if (status == MemEventStatus::OK) {
                line = cacheArray_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    stat_eventState[(int)Command::GetX][I]->addData(1);
                    stat_miss[1][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
            } else {
                recordMiss(event->getID());
            }
            break;
        case S:
            status = processCacheMiss(event, line, inMSHR); // Just acquire an MSHR entry
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    stat_eventState[(int)Command::GetX][S]->addData(1);
                    stat_miss[1][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordPrefetchResult(line, statPrefetchUpgradeMiss);

                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(SM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
            }
            break;
        case E:
            line->setState(M);
        case M:
            recordPrefetchResult(line, statPrefetchHit);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_eventState[(int)Command::GetX][state]->addData(1);
                stat_hit[1][inMSHR]->addData(1);
                stat_hits->addData(1);
            }

            if (!event->isStoreConditional() || line->isAtomic()) { // Don't write on a non-atomic SC
                line->setData(event->getPayload(), event->getAddr() - event->getBaseAddr());
                atomic = line->isAtomic();
                line->atomicEnd();
            }
            if (event->queryFlag(MemEvent::F_LOCKED)) {
                line->decLock();
            }

            sendTime = sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), atomic);
            line->setTimestamp(sendTime-1);
            if (is_debug_addr(addr))
                eventDI.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (is_debug_addr(addr)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}

/*
 * Handle GetSX (read-exclusive) request
 * GetSX acquires a line in exclusive state and locks it until any future GetX arrives
 */
bool MESIL1::handleGetSX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetSX, false, addr, state);

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        line->setState(M);
    }

    MemEventStatus status = MemEventStatus::OK;
    bool atomic = true;
    uint64_t sendTime = 0;
    vector<uint8_t> data;

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR);

            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    stat_eventState[(int)Command::GetSX][I]->addData(1);
                    stat_miss[2][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
            } else {
                recordMiss(event->getID());
            }
            break;
        case S:
            status = processCacheMiss(event, line, inMSHR); // Just acquire an MSHR entry
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    stat_eventState[(int)Command::GetSX][S]->addData(1);
                    stat_miss[2][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordPrefetchResult(line, statPrefetchUpgradeMiss);

                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(SM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (is_debug_addr(addr))
                    eventDI.reason = true;
            }
            break;
        case E:
            line->setState(M);
        case M:
            recordPrefetchResult(line, statPrefetchHit);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_eventState[(int)Command::GetSX][state]->addData(1);
                stat_hit[2][inMSHR]->addData(1);
                stat_hits->addData(1);
            }
            line->incLock();
            std::copy(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr()) + event->getSize(), data.begin());
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime-1);
            cleanUpAfterRequest(event, inMSHR);
            if (is_debug_addr(addr))
                eventDI.reason = "hit";
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (is_debug_addr(addr)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}

bool MESIL1::handleFlushLine(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    printLine(addr);

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    } else if (inMSHR) {
        mshr_->removePendingRetry(addr);
    }

    /* If we're here, state is stable */

    /* Flush fails if line is locked */
    if (state != I && line->isLocked()) {
        if (!inMSHR || !mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLine][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp());
        cleanUpAfterRequest(event, inMSHR);

        if (is_debug_addr(addr)) {
            if (line)
                eventDI.verboseline = line->getString();
            eventDI.reason = "fail, line locked";
        }

        return true;
    }

    /* If we're here, we need an MSHR entry if we don't already have one */
    if (!inMSHR && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    if (!mshr_->getProfiled(addr)) {
        stat_eventState[(int)Command::FlushLine][state]->addData(1);
        recordLatencyType(event->getID(), LatType::HIT);
        mshr_->setProfiled(addr);
    }

    mshr_->setInProgress(addr);

    bool downgrade = (state == E || state == M);
    forwardFlush(event, line, downgrade);

    if (line && state != I)
        line->setState(S_B);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}

bool MESIL1::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);

    printLine(addr);

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) != MemEventStatus::Reject);
    } else if (inMSHR) {
        mshr_->removePendingRetry(addr);

        if (!mshr_->getFrontEvent(addr) || mshr_->getFrontEvent(addr) != event) {
            if (is_debug_event(event))
                eventDI.action = "Stall";
            return true;
        }
    }
    /* Flush fails if line is locked */
    if (state != I && line->isLocked()) {
        if (!inMSHR || !mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp());
        cleanUpAfterRequest(event, inMSHR);

        if (is_debug_addr(addr)) {
            if (line)
                eventDI.verboseline = line->getString();
            eventDI.reason = "fail, line locked";
        }
        return true;
    }

    /* If we're here, we need an MSHR entry if we don't already have one */
    if (!inMSHR && (allocateMSHR(event, false) == MemEventStatus::Reject)) {
        return false;
    }

    mshr_->setInProgress(addr);
    if (!mshr_->getProfiled(addr)) {
        stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
        if (line)
            recordPrefetchResult(line, statPrefetchEvict);
        mshr_->setProfiled(addr);
    }

    forwardFlush(event, line, state != I);
    if (state != I)
        line->setState(I_B);

    recordLatencyType(event->getID(), LatType::HIT);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    return true;
}

bool MESIL1::handleFetch(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    snoopInvalidation(event, line); // Let core know that line is being accessed elsewhere (for ARM/gem5)

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Fetch, false, event->getBaseAddr(), state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case S:
        case SM:
        case S_B:
            sendResponseDown(event, line, true);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
            /* In these cases, an eviction raced with this request; ignore and wait for a response to our request */
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    cachename_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::Fetch][state]->addData(1);

    delete event;
    return true;
}

bool MESIL1::handleInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Inv, false, event->getBaseAddr(), state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    /* Note - not possible to receive an inv when the line is locked (locked implies state = E or M) */

    stat_eventState[(int)Command::Inv][state]->addData(1);
    if (line)
        recordPrefetchResult(line, statPrefetchInv);

    switch (state) {
        case S:
        case S_B: // Inv raced with our FlushLine; ordering Inv before FlushLine
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(I);
            break;
        case SM: // Inv raced with our upgrade; ordering Inv before upgrade
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(IM);
            break;
        case I_B:
            line->setState(I);
        case IS:
        case IM:
        case I:
            // Raced with Put* or FlushLineInv; will be resolved by arrival of our request
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received Inv in unhandled state '%s'. Event: %s, Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;
    return true;
}

bool MESIL1::handleForceInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::ForceInv, false, event->getBaseAddr(), state);

    switch (state) {
        case S:
        case E:
        case M:
        case S_B:
            if (line->isLocked()) {
                if (!inMSHR && allocateMSHR(event, true, 0) == MemEventStatus::Reject)
                    return false;
                else {
                    stat_eventStalledForLock->addData(1);
                    if (is_debug_event(event)) {
                        eventDI.action = "Stall";
                        eventDI.reason = "line locked";
                    }
                    return true;
                }
            }
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(I);
            break;
        case I_B:
            line->setState(I);
        case I:
        case IS:
        case IM:
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        case SM:
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(IM);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Tme = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::ForceInv][state]->addData(1);
    if (line) {
        recordPrefetchResult(line, statPrefetchInv);

        if (is_debug_event(event)) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    return true;
}

bool MESIL1::handleFetchInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInv, false, event->getBaseAddr(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            stat_eventState[(int)Command::FetchInv][state]->addData(1);
            delete event;
            return true;
        case M:
            if (line->isLocked()) {
                if (!inMSHR && (allocateMSHR(event, true, 0) == MemEventStatus::Reject))
                    return false;
                else {
                    stat_eventStalledForLock->addData(1);
                    if (is_debug_event(event)) {
                        eventDI.action = "Stall";
                        eventDI.reason = "line locked";
                    }
                    return true;
                }
            }
        case S:
        case E:
        case S_B:
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(I);
            break;
        case I_B:
            line->setState(I);
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        case SM:
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(IM);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::FetchInv][state]->addData(1);

    if (line) {
        recordPrefetchResult(line, statPrefetchInv);

        if (is_debug_event(event)) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}

bool MESIL1::handleFetchInvX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInvX, false, event->getBaseAddr(), state);

    switch (state) {
        case E:
        case M:
            if (line->isLocked()) {
                if (!inMSHR && (allocateMSHR(event, true, 0) == MemEventStatus::Reject)) {
                    return false;
                } else {
                    stat_eventStalledForLock->addData(1);
                    if (is_debug_event(event)) {
                        eventDI.action = "Stall";
                        eventDI.reason = "line locked";
                    }
                    return true;
                }
            }
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(S);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
        case S_B:
            if (is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::FetchInvX][state]->addData(1);

    if (is_debug_addr(event->getBaseAddr()) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    delete event;
    return true;
}

bool MESIL1::handleGetSResp(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::GetSResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetSResp, localPrefetch, addr, state);

    req->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(S);

    if (req->isLoadLink())
        line->atomicStart();

    if (is_debug_addr(addr))
        printData(line->getData(), true);

    if (localPrefetch) {
        line->setPrefetch(true);
        recordPrefetchLatency(req->getID(), LatType::MISS);
        if (is_debug_addr(addr))
            eventDI.action = "Done";
    } else {
        req->setMemFlags(event->getMemFlags());
        Addr offset = req->getAddr() - addr;
        vector<uint8_t> data(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
        uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp());
        line->setTimestamp(sendTime-1);
    }

    if (is_debug_addr(addr)) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool MESIL1::handleGetXResp(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::GetXResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetXResp, localPrefetch, addr, state);

    req->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - addr;

    switch (state) {
        case IS:
            {
                line->setData(event->getPayload(), 0);
                if (is_debug_addr(addr))
                    printData(line->getData(), true);

                if (event->getDirty()) {
                    line->setState(M); // Sometimes get dirty data from a noninclusive cache
                } else {
                    line->setState(protocolState_); // E (MESI) or S (MSI)
                }

                if (req->isLoadLink())
                    line->atomicStart();

                if (localPrefetch) {
                    line->setPrefetch(true);
                    recordPrefetchLatency(req->getID(), LatType::MISS);
                } else {
                    data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
                    uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp());
                    line->setTimestamp(sendTime - 1);
                }
                break;
            }
        case IM:
            line->setData(event->getPayload(), 0);
            if (is_debug_addr(addr))
                printData(line->getData(), true);
        case SM:
            {
                line->setState(M);

                if (req->getCmd() == Command::GetX) {
                    if (!req->isStoreConditional() || line->isAtomic()) { // Normal or successful store-conditional
                        line->setData(req->getPayload(), offset);

                        if (is_debug_addr(addr))
                            printData(line->getData(), true);

                        line->atomicEnd(); // Any write causes a future SC to fail 
                    }

                    if (req->queryFlag(MemEventBase::F_LOCKED)) {
                        line->decLock();
                    }
                } else { // Read lock/GetSX
                    line->incLock();
                }
                data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
                uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp(), false);
                line->setTimestamp(sendTime-1);
                break;
            }
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    cleanUpAfterResponse(event, inMSHR);

    if (is_debug_addr(addr)) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIL1::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::FlushLineResp][state]->addData(1);

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLineResp, false, addr, state);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            line->atomicEnd();
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, timestamp_, event->success());

    if (is_debug_event(event)) {
        eventDI.action = "Respond";
        if (line) {
            eventDI.verboseline = line->getString();
            eventDI.newst = line->getState();
        }
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool MESIL1::handleAckPut(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::AckPut][state]->addData(1);

    if (is_debug_addr(addr)) {
        eventDI.prefill(event->getID(), Command::AckPut, false, addr, state);
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in thsi case an eviction */
bool MESIL1::handleNULLCMD(MemEvent * event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    L1CacheLine * line = cacheArray_->lookup(oldAddr, false);

    bool evicted = handleEviction(newAddr, line);

    if (is_debug_addr(newAddr)) {
        eventDI.prefill(event->getID(), Command::NULLCMD, false, line->getAddr(), evictDI.oldst);
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->deallocate(line);
        retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
        mshr_->addPendingRetry(newAddr);
        if (mshr_->removeEvictPointer(oldAddr, newAddr))
            retry(oldAddr);
        if (is_debug_addr(newAddr)) {
            eventDI.action = "Retry";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr;
            eventDI.reason = reason.str();
        }
    } else { // Could be stalling for a new address or locked line
        if (is_debug_addr(newAddr)) {
            eventDI.action = "Stall";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr << ", evict failed";
                eventDI.reason = reason.str();
        }
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (is_debug_addr(line->getAddr()) || is_debug_event(event)) {
                eventDI.action = "Stall";
                std::stringstream st;
                st << "0x" << std::hex << newAddr << ", new targ";
                eventDI.reason = st.str();
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    delete event;
    return true;
}


/* Handle a NACKed event */
bool MESIL1::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    L1CacheLine* line = cacheArray_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(event->getBaseAddr()))
        eventDI.prefill(event->getID(), Command::NACK, false, event->getBaseAddr(), state);

    delete event;
    resendEvent(nackedEvent, false); // An L1 always resends NACKed events and they are always sent down

    return true;
}


/***********************************************************************************************************
 * MSHR and CacheArray management
 ***********************************************************************************************************/

/*
 * Handle a cache miss
 */
MemEventStatus MESIL1::processCacheMiss(MemEvent* event, L1CacheLine * line, bool inMSHR) {
    MemEventStatus status = MemEventStatus::OK;
    // Allocate an MSHR entry
    if (inMSHR) {
        status = MemEventStatus::Stall;
        if (mshr_->getFrontEvent(event->getBaseAddr())) {
            MemEvent* front = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
            status = (front == event) ? MemEventStatus::OK : MemEventStatus::Stall;
        }
    } else {
        status = allocateMSHR(event, false);
    }

    // Allocate a cache line
    if (status == MemEventStatus::OK && !line) {
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}

/*
 * Allocate a new cache line
 */
L1CacheLine* MESIL1::allocateLine(MemEvent* event, L1CacheLine* line) {
    bool evicted = handleEviction(event->getBaseAddr(), line);

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return line;
    } else {
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            std::stringstream st;
            st << "evict 0x" << std::hex << line->getAddr() << ", ";
            eventDI.reason = st.str();
        }
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        return nullptr;
    }
}

/*
 * Evict a cacheline
 * Return whether successful and return line pointer via input parameters
 */
bool MESIL1::handleEviction(Addr addr, L1CacheLine*& line) {
    if (!line) {
        line = cacheArray_->findReplacementCandidate(addr);
    }
    State state = line->getState();

    if (is_debug_addr(addr))
        evictDI.oldst = line->getState();

    /* L1s can have locked cache lines which prevents replacement */
    if (line->isLocked()) {
        stat_eventStalledForLock->addData(1);
        if (is_debug_addr(line->getAddr()))
            printDebugAlloc(false, line->getAddr(), "InProg, line locked");
        return false;
    }

    stat_evict[state]->addData(1);

    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!silentEvictClean_) {
                    sendWriteback(Command::PutS, line, false);
                    if (recvWritebackAck_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!silentEvictClean_) {
                    sendWriteback(Command::PutE, line, false);
                    if (recvWritebackAck_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                line->setState(I);
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                sendWriteback(Command::PutM, line, true);
                if (recvWritebackAck_)
                    mshr_->insertWriteback(line->getAddr(), false);
                if (is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Writeback");
                line->setState(I);
                break;
            }
        default:
            if (is_debug_addr(line->getAddr())) {
                stringstream note;
                if (mshr_->getPendingRetries(addr) != 0) {
                    note << "PendRetry, " << StateString[state];
                } else {
                    note << "InProg, " << StateString[state];
                }
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    line->atomicEnd();
    recordPrefetchResult(line, statPrefetchEvict);
    return true;
}


/* Clean up MSHR state, events, etc. after a request. Also trigger any retries */
void MESIL1::cleanUpAfterRequest(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (inMSHR) {
        mshr_->removeFront(addr);
    }

    delete event;

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr))
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting to evict this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}


/* Clean up MSHR state, events, etc. after a response. Also trigger any retries */
void MESIL1::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    mshr_->removeFront(addr); // delete req after this since debug might print the event it's removing
    delete event;
    if (req)
        delete req;

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr)) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer to an eviction
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
        }
    }
}


/* Retry the next event at address 'addr' */
void MESIL1::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
        }
    }
}


/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

/*
 * Send a response event up towards core
 * Args:
 *  event: The request event that triggered this response
 *  data:  The data to be returned. **L1's return only the desired word so this should be ONLY the requested bytes
 *  inMSHR: Whether 'event' is in the MSHR (and required an MSHR access to look it up)
 *  time: The earliest time at which the requested cacheline can be accessed
 *  success: for requests that can fail (e.g., LLSC), whether the request was successful or not (default false)
 *
 *  Return: time that the requested cacheline can again be accessed
 */
uint64_t MESIL1::sendResponseUp(MemEvent* event, vector<uint8_t>* data, bool inMSHR, uint64_t time, bool success) {
    Command cmd = event->getCmd();
    MemEvent * responseEvent = event->makeResponse();

    uint64_t latency = inMSHR ? mshrLatency_ : tagLatency_;
    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size());
        if (is_debug_event(event)) {
            printData(data, false);
        }
        latency = accessLatency_;
    }

    if (success)
        responseEvent->setSuccess(true);

    // Compute latency, accounting for serialization of requests to the address
    uint64_t deliveryTime = latency;
    if (time < timestamp_)
        deliveryTime += timestamp_;
    else deliveryTime += time;
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);

    // Debugging
    if (is_debug_event(responseEvent))
        eventDI.action = "Respond";

    return deliveryTime;
}


/*
 * Send a response to a lower cache/directory/memory/etc.
 * E.g., AckInv, FetchResp, FetchXResp, etc.
 */
void MESIL1::sendResponseDown(MemEvent * event, L1CacheLine * line, bool data) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*line->getData());
        if (line->getState() == M)
            responseEvent->setDirty(true);
    }

    responseEvent->setSize(lineSize_);

    uint64_t deliverTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    Response resp  = {responseEvent, deliverTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueue(resp);

    if (is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
    }
}


/* Forward a flush to the next cache/directory/memory */
void MESIL1::forwardFlush(MemEvent* event, L1CacheLine* line, bool evict) {
    MemEvent* flush = new MemEvent(*event);

    flush->setSrc(cachename_);
    flush->setDst(getDestination(event->getBaseAddr()));

    uint64_t latency = tagLatency_; // Check coherence state/hitVmiss
    if (evict) {
        flush->setEvict(true);
        flush->setPayload(*(line->getData()));
        flush->setDirty(line->getState() == M);
        latency = accessLatency_; // Time to check coherence & access data (in parallel)
    } else {
        flush->setPayload(0, nullptr);
    }
    bool payload = false;

    uint64_t baseTime = timestamp_;
    if (line && line->getTimestamp() > baseTime) baseTime = line->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (line)
        line->setTimestamp(deliveryTime-1);

    if (is_debug_addr(event->getBaseAddr()))
        eventDI.action = "forward";
}


/*
 * Send a writeback
 * Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIL1::sendWriteback(Command cmd, L1CacheLine * line, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setDst(getDestination(line->getAddr()));
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*line->getData());
        writeback->setDirty(dirty);

        if (is_debug_addr(line->getAddr())) {
            printData(line->getData(), false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t baseTime = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    line->setTimestamp(deliveryTime-1);
}


/* Send notification to the core that a line we have might have been lost */
void MESIL1::snoopInvalidation(MemEvent * event, L1CacheLine * line) {
    if (snoopL1Invs_ && line) {
        MemEvent * snoop = new MemEvent(cachename_, event->getAddr(), event->getBaseAddr(), Command::Inv);
        uint64_t baseTime = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
        uint64_t deliveryTime = baseTime + tagLatency_;
        Response resp = {snoop, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIL1::addToOutgoingQueue(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueue(resp);
}

void MESIL1::addToOutgoingQueueUp(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueueUp(resp);
}


/***********************************************************************************************************
 * Statistics and listeners
 ***********************************************************************************************************/

/* Record result of a prefetch. Important: assumes line is not null */
void MESIL1::recordPrefetchResult(L1CacheLine* line, Statistic<uint64_t>* stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}


void MESIL1::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return; // Never set a hit/miss status
    switch (cmd) {
        case Command::GetS:
            stat_latencyGetS[type]->addData(latency);
            break;
        case Command::GetX:
            stat_latencyGetX[type]->addData(latency);
            break;
        case Command::GetSX:
            stat_latencyGetSX[type]->addData(latency);
            break;
        case Command::FlushLine:
            stat_latencyFlushLine[type]->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latencyFlushLineInv[type]->addData(latency);
            break;
        default:
            break;
    }
}


void MESIL1::eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool inMSHR) {
    if (!inMSHR || !mshr_->getProfiled(event->getBaseAddr())) {
        stat_eventState[(int)event->getCmd()][state]->addData(1); // Profile event receive
        notifyListenerOfAccess(event, type, result);
        if (inMSHR)
            mshr_->setProfiled(event->getBaseAddr());
    }
}


/***********************************************************************************************************
 * Miscellaneous
 ***********************************************************************************************************/
MemEventInitCoherence* MESIL1::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true, false, false, lineSize_, true);
}

std::set<Command> MESIL1::getValidReceiveEvents() {
    std::set<Command> cmds;
    cmds.insert(Command::GetS);
    cmds.insert(Command::GetX);
    cmds.insert(Command::GetSX);
    cmds.insert(Command::FlushLine);
    cmds.insert(Command::FlushLineInv);
    cmds.insert(Command::Inv);
    cmds.insert(Command::ForceInv);
    cmds.insert(Command::Fetch);
    cmds.insert(Command::FetchInv);
    cmds.insert(Command::FetchInvX);
    cmds.insert(Command::NULLCMD);
    cmds.insert(Command::GetSResp);
    cmds.insert(Command::GetXResp);
    cmds.insert(Command::FlushLineResp);
    cmds.insert(Command::AckPut);
    cmds.insert(Command::NACK );

    return cmds;
}

void MESIL1::setSliceAware(uint64_t interleaveSize, uint64_t interleaveStep) {
    cacheArray_->setSliceAware(interleaveSize, interleaveStep);
}

Addr MESIL1::getBank(Addr addr) {
    return cacheArray_->getBank(addr);
}

void MESIL1::printLine(Addr addr) { }
void MESIL1::printData(Addr addr) { }
void MESIL1::printData(vector<uint8_t>* data, bool set) { }

void MESIL1::printStatus(Output &out) {
    cacheArray_->printCacheArray(out);
}

