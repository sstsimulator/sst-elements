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

#include <sst_config.h>
#include <vector>
#include "coherencemgr/MESI_L1.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

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

    if (mem_h_is_debug_addr(addr)) {
        eventDI.prefill(event->getID(), event->getThreadID(), Command::GetS, (localPrefetch ? "-pref" : "" ), addr, state);
        eventDI.reason = "hit";
    }

    if (isFlushing_ && !inMSHR) {
        eventDI.action = "Reject";
        eventDI.reason = "Cache flush in progress";
        return false;
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
                if (mem_h_is_debug_addr(addr))
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

            if (event->isLoadLink()) {
                line->atomicStart(timestamp_ + llscBlockCycles_, event->getThreadID());
            }
            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime - 1);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_event(event)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false : true;
}


/*
 * Handle cacheable Write requests
 * May also be a store-conditional or write-unlock
 */
bool MESIL1::handleWrite(MemEvent* event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}

/*
 * Handle cacheable GetX/Write requests
 * May also be store-conditional or write-unlock
 */
bool MESIL1::handleGetX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), event->getThreadID(), Command::GetX, (event->isStoreConditional() ? "-SC" : ""), addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    // Any event other than an unlock or store-conditional needs to stall for a cache flush
    if (isFlushing_ && !inMSHR && !event->isStoreConditional() && !event->queryFlag(MemEvent::F_LOCKED)) {
        eventDI.action = "Reject";
        eventDI.reason = "Cache flush in progress";
        return false;
    }

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        line->setState(M);
    }

    MemEventStatus status = MemEventStatus::OK;
    bool success = true;
    uint64_t sendTime = 0;

    switch (state) {
        case I:
            if (event->isStoreConditional()) { /* Check if we can process the event, if so, it failed */
                status = checkMSHRCollision(event, inMSHR);
                if (status == MemEventStatus::OK) {
                    sendTime = sendResponseUp(event, nullptr, inMSHR, 0, false);
                    if (mem_h_is_debug_addr(addr))
                        eventDI.reason = "hit/fail";
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                }
                break;
            }

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

                sendTime = forwardMessage(event, lineSize_, 0, nullptr, Command::GetX);
                line->setState(IM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
                    eventDI.reason = "miss";
            } else {
                recordMiss(event->getID());
            }
            break;
        case S:
            if (event->isStoreConditional()) { /* Definitely failed */
                sendTime = sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), false);
                line->setTimestamp(sendTime-1);
                if (mem_h_is_debug_addr(addr))
                    eventDI.reason = "hit/fail";
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

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

                sendTime = forwardMessage(event, lineSize_, 0, nullptr, Command::GetX);
                line->setState(SM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
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

            if (!event->isStoreConditional() || line->isAtomic(event->getThreadID())) { // Don't write on a non-atomic SC
                line->setData(event->getPayload(), event->getAddr() - event->getBaseAddr());
                line->atomicEnd();
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, line->getData(), true);
            } else {
                success = false;
            }
            if (event->queryFlag(MemEvent::F_LOCKED)) {
                line->decLock();
            }

            sendTime = sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), success);
            line->setTimestamp(sendTime-1);
            if (mem_h_is_debug_addr(addr)) {
                if (success) eventDI.reason = "hit";
                else eventDI.reason = "hit/fail";
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_addr(addr)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}


/*
 * Handle GetSX (read-exclusive) request
 * GetSX acquires a line in exclusive or modified state
 *  F_LOCKED: Lock line until future GetX arrives. Line can preemptively be put into M (dirty) state
 *  F_LLSC: Flag line as atomic and watch for accesses. Line should be put in E state if possible, otherwise M.
 */
bool MESIL1::handleGetSX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), event->getThreadID(), Command::GetSX, (event->isLoadLink() ? "-LL" : ""), addr, state);

    if (isFlushing_ && !inMSHR) {
        eventDI.action = "Reject";
        eventDI.reason = "Cache flush in progress";
        return false;
    }

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = protocolExclState_;
        line->setState(protocolExclState_);
    }

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;
    vector<uint8_t> data;

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR);

            if (status == MemEventStatus::OK) {
                line = cacheArray_->lookup(addr, false);
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
                if (mem_h_is_debug_addr(addr))
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
                if (mem_h_is_debug_addr(addr))
                    eventDI.reason = "miss";
            }
            break;
        case E:
        case M:
            recordPrefetchResult(line, statPrefetchHit);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_eventState[(int)Command::GetSX][state]->addData(1);
                stat_hit[2][inMSHR]->addData(1);
                stat_hits->addData(1);
            }
            if (event->isLoadLink()) {
                line->atomicStart(timestamp_ + llscBlockCycles_, event->getThreadID());
            }
            else
                line->incLock();
            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime - 1);
            cleanUpAfterRequest(event, inMSHR);
            if (mem_h_is_debug_addr(addr))
                eventDI.reason = "hit";
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_addr(addr)) {
                eventDI.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}

bool MESIL1::handleFlushLine(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), event->getThreadID(), Command::FlushLine, "", addr, state);

    if (isFlushing_ && !inMSHR) {
        eventDI.action = "Reject";
        eventDI.reason = "Cache flush in progress";
        return false;
    }

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    } else if (inMSHR) {
        mshr_->removePendingRetry(addr);
    }

    /* If we're here, state is stable */

    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        if (!inMSHR || !mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLine][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), false);
        cleanUpAfterRequest(event, inMSHR);

        if (mem_h_is_debug_addr(addr)) {
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

    if (mem_h_is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}

bool MESIL1::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), event->getThreadID(), Command::FlushLineInv, "", addr, state);

    if (isFlushing_ && !inMSHR) {
        eventDI.action = "Reject";
        eventDI.reason = "Cache flush in progress";
        return false;
    }

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) != MemEventStatus::Reject);
    } else if (inMSHR) {
        mshr_->removePendingRetry(addr);

        if (!mshr_->getFrontEvent(addr) || mshr_->getFrontEvent(addr) != event) {
            if (mem_h_is_debug_event(event))
                eventDI.action = "Stall";
            return true;
        }
    }
    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        if (!inMSHR || !mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), false);
        cleanUpAfterRequest(event, inMSHR);

        if (mem_h_is_debug_addr(addr)) {
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

    if (mem_h_is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    return true;
}


bool MESIL1::handleFlushAll(MemEvent* event, bool inMSHR) {
    eventDI.prefill(event->getID(), Command::FlushAll, "", 0, State::NP);

    // A core shouldn't send another FlushAll while one is outstanding but just in case
    if (!inMSHR) {
        if (isFlushing_) {
            eventDI.action = "Reject";
            eventDI.reason = "Flush in progress";
            return false;
        }

        if (mshr_->insertFlush(event, false) == MemEventStatus::Reject) {
            if (mem_h_is_debug_event(event)) {
                eventDI.action = "Reject";
                eventDI.reason = "MSHR full";
            }
            return false;
        }

        isFlushing_ = true;
        if (!flush_manager_) {
            // Forward flush to flush manager
            MemEvent* flush = new MemEvent(*event); // Copy event for forwarding
            flush->setDst(flush_dest_);
            forwardByDestination(flush, timestamp_ + mshrLatency_); // Time to insert event in MSHR
            eventDI.action = "forward";
            return true;
        }
    }

    if (!flush_manager_) {
        debug->fatal(CALL_INFO, -1, "%s, ERROR: Trying to retry a flushall but not the flush manager...\n", getName().c_str());
    }

    if (mshr_->getFlushSize() != mshr_->getSize()) { /* Wait for MSHR to drain */
        eventDI.action = "Drain MSHR";
        flushDrain_ = true;
        return true;
    }

    flushDrain_ = false;

    bool success = true;
    bool evictionNeeded = false;
    for (auto it : *cacheArray_) {
        if (it->isLocked(timestamp_)) {
            success = false; // No good, should not have issued a FlushAll between a lock/unlock!
            evictionNeeded = false;
            break;
        }

        switch (it->getState()) {
            case I:
                break;
            case S:
            case E:
            case M:
                {
                MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                retryBuffer_.push_back(ev);
                evictionNeeded = true;
                mshr_->incrementFlushCount();
                break;
                }
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
            break;
        }
    }

    if (!evictionNeeded) {
        isFlushing_ = false;
        sendResponseUp(event, nullptr, true, timestamp_, success);
        cleanUpAfterFlush(event); // Remove FlushAll
        eventDI.action = "Respond";
    } else {
        eventDI.action = "Flush";
    }

    return true;
}


bool MESIL1::handleForwardFlush(MemEvent* event, bool inMSHR) {
    eventDI.prefill(event->getID(), Command::ForwardFlush, "", 0, State::NP);
    MemEventStatus status = MemEventStatus::OK;

    if (!inMSHR) {
        status = mshr_->insertFlush(event, true);
        if (status == MemEventStatus::Reject) {
            if (mem_h_is_debug_event(event)) {
                eventDI.action = "Reject";
                eventDI.reason = "MSHR full";
            }
            return false; /* No room for flush in MSHR */
        } else if (status == MemEventStatus::Stall) {
            eventDI.action = "Stall";
            eventDI.reason = "Flush in progress";
            return true;
        }
    }
    isFlushing_ = true;

    if (mshr_->getFlushSize() != mshr_->getSize()) { /* Wait for MSHR to drain */
        eventDI.action = "Drain MSHR";
        flushDrain_ = true;
        return true;
    }

    flushDrain_ = false;

    bool success = true;
    bool evictionNeeded = false;
    for (auto it : *cacheArray_) {
        if (it->isLocked(timestamp_)) {
            // Retry in a few cycles
            success = false;
            continue;
        }

        switch (it->getState()) {
            case I:
                break;
            case S:
            case E:
            case M:
                {
                MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                retryBuffer_.push_back(ev);
                evictionNeeded = true;
                mshr_->incrementFlushCount();
                break;
                }
            default:
                debug->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s, Time: %" PRIu64 "ns\n",
                    cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                break;
        }
    }

    if (evictionNeeded) {
        // if unsuccessful that is OK, we'll retry when the evictions complete
        eventDI.action = "Flush";
    } else if (!success) { // No choice but to keep retrying until the unlock occurs
        retryBuffer_.push_back(event);
        eventDI.action = "Retry";
    } else {
        MemEvent * responseEvent = event->makeResponse();
        uint64_t deliverTime = std::max(timestamp_ + tagLatency_, flush_complete_timestamp_ + 1);
        forwardByDestination(responseEvent, deliverTime);
        eventDI.action = "Respond";

        mshr_->removeFlush(); // Remove ForwardFlush
        delete event;

        MemEventBase* next_flush = mshr_->getFlush();
        if (next_flush != nullptr && next_flush->getCmd() == Command::ForwardFlush) {
            retryBuffer_.push_back(next_flush);
        }
    }

    return true;
}


bool MESIL1::handleUnblockFlush(MemEvent* event, bool inMSHR) {
    eventDI.prefill(event->getID(), Command::UnblockFlush, "", 0, State::NP);

    if (mshr_->getFlush() == nullptr)
        isFlushing_ = false;
    delete event;

    return true;
}


bool MESIL1::handleFetch(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    snoopInvalidation(event, line); // Let core know that line is being accessed elsewhere (for ARM/gem5)

    if (mem_h_is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Fetch, "", event->getBaseAddr(), state);

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
            if (mem_h_is_debug_event(event))
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

    if (mem_h_is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Inv, "", event->getBaseAddr(), state);

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
            if (mem_h_is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received Inv in unhandled state '%s'. Event: %s, Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_event(event) && line) {
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

    if (mem_h_is_debug_event(event))
        eventDI.prefill(event->getID(), Command::ForceInv, "", event->getBaseAddr(), state);

    switch (state) {
        case S:
        case E:
        case M:
        case S_B:
            if (line->isLocked(timestamp_)) {
                if (!inMSHR && allocateMSHR(event, true, 0) == MemEventStatus::Reject)
                    return false;
                else {
                    stat_eventStalledForLock->addData(1);
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llscTimeoutSelfLink_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    if (mem_h_is_debug_event(event)) {
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
            if (mem_h_is_debug_event(event))
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

        if (mem_h_is_debug_event(event)) {
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

    if (mem_h_is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInv, "", event->getBaseAddr(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
            if (mem_h_is_debug_event(event))
                eventDI.action = "Ignore";
            stat_eventState[(int)Command::FetchInv][state]->addData(1);
            delete event;
            return true;
        case E:
        case M:
            if (line->isLocked(timestamp_)) {
                if (!inMSHR && (allocateMSHR(event, true, 0) == MemEventStatus::Reject))
                    return false; /* Unable to allocate MSHR, must NACK */
                else {
                    stat_eventStalledForLock->addData(1);
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llscTimeoutSelfLink_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    if (mem_h_is_debug_event(event)) {
                        eventDI.action = "Stall";
                        eventDI.reason = "line locked";
                    }
                    return true;
                }
            }
        case S:
        case S_B:
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(I);
            break;
        case I_B:
            line->setState(I);
            if (mem_h_is_debug_event(event))
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

        if (mem_h_is_debug_event(event)) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    if (inMSHR)
        cleanUpAfterRequest(event, inMSHR);
    else
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

    if (mem_h_is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInvX, "", event->getBaseAddr(), state);

    switch (state) {
        case E:
        case M:
            if (line->isLocked(timestamp_)) {
                if (!inMSHR && (allocateMSHR(event, true, 0) == MemEventStatus::Reject)) {
                    return false;
                } else {
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llscTimeoutSelfLink_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    stat_eventStalledForLock->addData(1);
                    if (mem_h_is_debug_event(event)) {
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
            if (mem_h_is_debug_event(event))
                eventDI.action = "Ignore";
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::FetchInvX][state]->addData(1);

    if (mem_h_is_debug_addr(event->getBaseAddr()) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    if (inMSHR)
        cleanUpAfterRequest(event, inMSHR);
    else
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

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), req->getThreadID(), Command::GetSResp, (localPrefetch ? "-pref" : ""), addr, state);

    req->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(S);
    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), false);

    if (req->isLoadLink()) {
        line->atomicStart(timestamp_ + llscBlockCycles_, req->getThreadID());
    }

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), true);

    if (localPrefetch) {
        line->setPrefetch(true);
        recordPrefetchLatency(req->getID(), LatType::MISS);
        if (mem_h_is_debug_addr(addr))
            eventDI.action = "Done";
    } else {
        req->setMemFlags(event->getMemFlags());
        Addr offset = req->getAddr() - addr;
        vector<uint8_t> data(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
        uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp());
        line->setTimestamp(sendTime-1);
    }

    if (mem_h_is_debug_addr(addr)) {
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

    if (mem_h_is_debug_addr(addr)) {
        std::string mod = localPrefetch ? "-pref" : (req->isLoadLink() ? "-LL" : (req->isStoreConditional() ? "-SC" : ""));
        eventDI.prefill(event->getID(), req->getThreadID(), Command::GetXResp, mod, addr, state);
    }
    req->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - addr;
    bool success = true;

    switch (state) {
        case IS:
            {
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, line->getData(), true);

                if (event->getDirty()) {
                    line->setState(M); // Sometimes get dirty data from a noninclusive cache
                } else {
                    line->setState(protocolReadState_); // E (MESI) or S (MSI)
                }

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
            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, line->getData(), true);
        case SM:
            {
                line->setState(M);

                if (req->getCmd() == Command::Write || req->getCmd() == Command::GetX) {
                    if (!req->isStoreConditional() || line->isAtomic(req->getThreadID())) { // Normal or successful store-conditional
                        line->setData(req->getPayload(), offset);

                        if (mem_h_is_debug_addr(addr))
                            printDataValue(addr, line->getData(), true);
                        line->atomicEnd(); // Any write causes a future SC to fail
                    } else {
                        success = false;
                    }

                    if (req->queryFlag(MemEventBase::F_LOCKED)) {
                        line->decLock();
                    }
                } else if (req->isLoadLink()) {
                    if (!event->getDirty()) {
                        line->setState(protocolExclState_);
                    }
                    line->atomicStart(timestamp_ + llscBlockCycles_, req->getThreadID());
                } else { // ReadLock
                    line->incLock();
                }
                data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
                uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp(), success);
                line->setTimestamp(sendTime-1);
                break;
            }
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    cleanUpAfterResponse(event, inMSHR);

    if (mem_h_is_debug_addr(addr)) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
        if (!success)
            eventDI.reason = "hit/fail";
    }

    return true;
}


bool MESIL1::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    if (mem_h_is_debug_addr(addr))
        eventDI.prefill(event->getID(), req->getThreadID(), Command::FlushLineResp, "", addr, state);

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

    if (mem_h_is_debug_event(event)) {
        eventDI.action = "Respond";
        if (line) {
            eventDI.verboseline = line->getString();
            eventDI.newst = line->getState();
        }
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool MESIL1::handleFlushAllResp(MemEvent * event, bool inMSHR) {
    MemEvent* flush_req = static_cast<MemEvent*>(mshr_->getFlush());
    mshr_->removeFlush(); // Remove FlushAll

    eventDI.prefill(event->getID(), flush_req->getThreadID(), Command::FlushAllResp, "", 0, State::NP);
    eventDI.action = "Respond";

    sendResponseUp(flush_req, nullptr, true, timestamp_);

    delete flush_req;
    delete event;

    if (mshr_->getFlush() != nullptr) {
        retryBuffer_.push_back(mshr_->getFlush());
    }

    return true;
}


bool MESIL1::handleAckPut(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)Command::AckPut][state]->addData(1);

    if (mem_h_is_debug_addr(addr)) {
        eventDI.prefill(event->getID(), Command::AckPut, "", addr, state);
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIL1::handleNULLCMD(MemEvent * event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    L1CacheLine * line = cacheArray_->lookup(oldAddr, false);

    bool evicted = handleEviction(newAddr, line, oldAddr == newAddr);

    if (mem_h_is_debug_addr(newAddr)) {
        eventDI.prefill(event->getID(), Command::NULLCMD, "", line->getAddr(), evictDI.oldst);
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->deallocate(line);

        if (oldAddr != newAddr) { /* Reallocating a line to a new address */
            retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
            mshr_->addPendingRetry(newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
            if (mem_h_is_debug_addr(newAddr)) {
                eventDI.action = "Retry";
                std::stringstream reason;
                reason << "0x" << std::hex << newAddr;
                eventDI.reason = reason.str();
            }
        } else { /* Deallocating a line for a cache flush */
            mshr_->decrementFlushCount();
            if (mshr_->getFlushCount() == 0) {
                retryBuffer_.push_back(mshr_->getFlush());
            }
        }
    } else { // Could be stalling for a new address or locked line
        if (mem_h_is_debug_addr(newAddr)) {
            eventDI.action = "Stall";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr << ", evict failed";
                eventDI.reason = reason.str();
        }
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (mem_h_is_debug_addr(line->getAddr()) || mem_h_is_debug_event(event)) {
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

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        eventDI.prefill(event->getID(), Command::NACK, "", event->getBaseAddr(), state);

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
 * Check for an MSHR collision.
 * If one exists, put event into MSHR
 * Otherwise do nothing
 */
MemEventStatus MESIL1::checkMSHRCollision(MemEvent* event, bool inMSHR) {
    MemEventStatus status = MemEventStatus::OK;
    if (inMSHR) {
        status = MemEventStatus::Stall;
        if (mshr_->getFrontEvent(event->getBaseAddr())) {
            MemEvent* front = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
            status = (front == event) ? MemEventStatus::OK : MemEventStatus::Stall;
        }
    } else {
        int ins = mshr_->insertEventIfConflict(event->getBaseAddr(), event);
        if (ins == -1)
            status = MemEventStatus::Reject;
        else if (ins != 0)
            status = MemEventStatus::Stall;
    }
    return status;
}

/*
 * Allocate a new cache line
 */
L1CacheLine* MESIL1::allocateLine(MemEvent* event, L1CacheLine* line) {
    bool evicted = handleEviction(event->getBaseAddr(), line, false);

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (mem_h_is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return line;
    } else {
        if (mem_h_is_debug_event(event)) {
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
bool MESIL1::handleEviction(Addr addr, L1CacheLine*& line, bool flush) {
    if (!line) {
        line = cacheArray_->findReplacementCandidate(addr);
    }
    State state = line->getState();

    if (mem_h_is_debug_addr(addr))
        evictDI.oldst = line->getState();

    /* L1s can have locked cache lines which prevents replacement */
    if (line->isLocked(timestamp_)) {
        stat_eventStalledForLock->addData(1);
        if (mem_h_is_debug_addr(line->getAddr()))
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
                    sendWriteback(Command::PutS, line, false, flush);
                    if (recvWritebackAck_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!silentEvictClean_) {
                    sendWriteback(Command::PutE, line, false, flush);
                    if (recvWritebackAck_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                sendWriteback(Command::PutM, line, true, flush);
                if (recvWritebackAck_)
                    mshr_->insertWriteback(line->getAddr(), false);
                if (mem_h_is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Writeback");
                line->setState(I);
                break;
            }
        default:
            if (mem_h_is_debug_addr(line->getAddr())) {
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
        if (event->isPrefetch() && event->getRqstr() == cachename_) outstandingPrefetches_--;
        mshr_->removeFront(addr);
    }

    delete event;

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getPendingRetries(addr) == 0) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting to evict this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    } else if (isFlushing_ && flushDrain_ &&  mshr_->getSize() == mshr_->getFlushSize()) {
        retryBuffer_.push_back(mshr_->getFlush());
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
    if (req) {
        if (req->isPrefetch() && req->getRqstr() == cachename_) outstandingPrefetches_--;
        delete req;
    }

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
    } else if (isFlushing_ && flushDrain_ && mshr_->getSize() == mshr_->getFlushSize()) {
        retryBuffer_.push_back(mshr_->getFlush());
    }
}


/* Clean up MSHR state after a FlushAll or ForwardFlush completes */
void MESIL1::cleanUpAfterFlush(MemEvent* req, MemEvent* resp, bool inMSHR) {
    if (inMSHR) mshr_->removeFlush();

    delete req;
    if (resp) delete resp;

    if (mshr_->getFlush() != nullptr) {
        retryBuffer_.push_back(mshr_->getFlush());
    } else {
        isFlushing_ = false;
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

void MESIL1::handleLoadLinkExpiration(SST::Event* ev) {
    LoadLinkWakeup* ll = static_cast<LoadLinkWakeup*>(ev);

    if (mshr_->exists(ll->addr_) && (mshr_->getFrontType(ll->addr_) == MSHREntryType::Event) &&
            mshr_->getFrontEvent(ll->addr_)->getID() == ll->id_ && !mshr_->getInProgress(ll->addr_) &&
            (mshr_->getPendingRetries(ll->addr_) == 0)) {

        retryBuffer_.push_back(mshr_->getFrontEvent(ll->addr_));
        mshr_->addPendingRetry(ll->addr_);
        reenableClock_();
    }

    fflush(stdout);
    delete ev;
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
        if (mem_h_is_debug_event(event)) {
            printDataValue(event->getAddr(), data, false);
        }
        latency = accessLatency_;
    }

    if (!success)
        responseEvent->setFail();

    // Compute latency, accounting for serialization of requests to the address
    uint64_t deliveryTime = latency;
    if (time < timestamp_)
        deliveryTime += timestamp_;
    else deliveryTime += time;
    forwardByDestination(responseEvent, deliveryTime);

    // Debugging
    if (mem_h_is_debug_event(responseEvent))
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
    forwardByDestination(responseEvent, deliverTime);

    if (mem_h_is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
    }
}


/* Forward a flush to the next cache/directory/memory */
void MESIL1::forwardFlush(MemEvent* event, L1CacheLine* line, bool evict) {
    MemEvent* flush = new MemEvent(*event);

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
    forwardByAddress(flush, deliveryTime);
    if (line)
        line->setTimestamp(deliveryTime-1);

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        eventDI.action = "forward";
}


/*
 * Send a writeback
 * Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIL1::sendWriteback(Command cmd, L1CacheLine * line, bool dirty, bool flush) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*line->getData());
        writeback->setDirty(dirty);

        if (mem_h_is_debug_addr(line->getAddr())) {
            printDataValue(line->getAddr(), line->getData(), false);
        }

        latency = accessLatency_;
    }


    writeback->setRqstr(cachename_);

    uint64_t baseTime = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    forwardByAddress(writeback, deliveryTime);
    line->setTimestamp(deliveryTime-1);

    // If a full cache flush, we need to order flush w.r.t. *all* evictions
    if (flush && deliveryTime > flush_complete_timestamp_) {
        flush_complete_timestamp_ = deliveryTime;
    }


}


/* Send notification to the core that a line we have might have been lost */
void MESIL1::snoopInvalidation(MemEvent * event, L1CacheLine * line) {
    if (snoopL1Invs_ && line) {
        for (auto it = cpus.begin(); it != cpus.end(); it++) {
            MemEvent * snoop = new MemEvent(cachename_, event->getAddr(), event->getBaseAddr(), Command::Inv);
            uint64_t baseTime = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
            uint64_t deliveryTime = baseTime + tagLatency_;
            snoop->setDst(*it);
            forwardByDestination(snoop, deliveryTime);
        }
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIL1::forwardByAddress(MemEventBase* ev, Cycle_t ts) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, ts);
}

void MESIL1::forwardByDestination(MemEventBase* ev, Cycle_t ts) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, ts);
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
        case Command::Write:
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
    cmds.insert(Command::Write);
    cmds.insert(Command::GetSX);
    cmds.insert(Command::FlushLine);
    cmds.insert(Command::FlushLineInv);
    cmds.insert(Command::FlushAll);
    cmds.insert(Command::ForwardFlush);
    cmds.insert(Command::Inv);
    cmds.insert(Command::ForceInv);
    cmds.insert(Command::Fetch);
    cmds.insert(Command::FetchInv);
    cmds.insert(Command::FetchInvX);
    cmds.insert(Command::NULLCMD);
    cmds.insert(Command::GetSResp);
    cmds.insert(Command::GetXResp);
    cmds.insert(Command::FlushLineResp);
    cmds.insert(Command::FlushAllResp);
    cmds.insert(Command::UnblockFlush);
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

void MESIL1::printStatus(Output &out) {
    cacheArray_->printCacheArray(out);
}

void MESIL1::beginCompleteStage() {
    isFlushing_ = false;
}

void MESIL1::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
    if (event->getInitCmd() == MemEventInit::InitCommand::Flush) {
        if (isFlushing_) { // Already in progress, ignore
            delete event;
            return;
        }

        isFlushing_ = true;
        for (auto it : *cacheArray_) {
            // Only flush dirty data, no need to fix coherence state
            switch (it->getState()) {
                case I:
                case S:
                case E:
                    break;
                case M:
                    {
                    MemEventInit * ev = new MemEventInit(cachename_, Command::Write, it->getAddr(), *(it->getData()));
                    lowlink->sendUntimedData(ev, false, true); // Don't broadcast, route by addr
                    break;
                    }
            default:
                // TODO handle case where sim ends and state is not stable
                std::string valstr = getDataString(it->getData());
                debug->output("%s, NOTICE: Unable to flush cache line that is in transient state '%s'. Addr = 0x%" PRIx64 ". Value = %s\n",
                    cachename_.c_str(), StateString[it->getState()], it->getAddr(), valstr.c_str());
                break;
            };
        }

        if (!flush_manager_) { /* There's another cache/dir level below us; let it know we're done */
            MemEventUntimedFlush* response = new MemEventUntimedFlush(getName(), false);
            lowlink->sendUntimedData(response, true);
        }
    }
    delete event; // Nothing for now
}