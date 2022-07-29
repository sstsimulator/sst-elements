// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
#include "coherencemgr/Incoherent_L1.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Incoherent Controller
 * -> Essentially coherent for a single thread/single core so will write back dirty blocks but assumes no sharing
 * States:
 * I: not present
 * E: present & clean
 * M: present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/


/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/* Handle GetS (load/read) requests */
bool IncoherentL1::handleGetS(MemEvent* event, bool inMSHR){
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;

    if (is_debug_addr(addr)) {
        eventDI.prefill(event->getID(), Command::GetS, localPrefetch, addr, state);
        eventDI.reason = "hit";
    }

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR); // Attempt to allocate a line if needed

            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cacheArray_->lookup(addr, false);
                recordLatencyType(event->getID(), LatType::MISS);
                eventProfileAndNotify(event, I, NotifyAccessType::READ, NotifyResultType::MISS, true, inMSHR);
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(sendTime);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case E:
        case M:
            eventProfileAndNotify(event, state, NotifyAccessType::READ, NotifyResultType::HIT, inMSHR, inMSHR);
            if (localPrefetch) {
                recordPrefetchResult(line, statPrefetchRedundant);
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(line, statPrefetchHit);
            recordLatencyType(event->getID(), LatType::HIT);

            if (event->isLoadLink())
                line->atomicStart();

            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime-1);
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

    return ((status == MemEventStatus::Reject) ? false : true);
}

/* Handle cacheable GetX requests */
bool IncoherentL1::handleWrite(MemEvent* event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}

/* Handle GetX (store/write) requests
 * GetX may also be a store-conditional or write-unlock
 */
bool IncoherentL1::handleGetX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetX, false, addr, state);

    MemEventStatus status = MemEventStatus::OK;
    bool success = true;
    uint64_t sendTime = 0;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR); // Attempt to allocate a line if needed
            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cacheArray_->lookup(addr, false);

                // Profile
               if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    stat_eventState[(int)Command::GetX][I]->addData(1);
                    stat_miss[1][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                // Handle
                sendTime = forwardMessage(event, lineSize_, 0, nullptr, Command::GetX);
                line->setState(IM);
                line->setTimestamp(sendTime);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case E:
            line->setState(M);
        case M:
            // Profile
            recordPrefetchResult(line, statPrefetchHit);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_eventState[(int)Command::GetX][state]->addData(1);
                stat_hit[1][inMSHR]->addData(1);
                stat_hits->addData(1);
            }

            // Handle
            if (!event->isStoreConditional() || line->isAtomic()) { /* Don't write on a non-atomic SC */
                line->setData(event->getPayload(), event->getAddr() - event->getBaseAddr());
                line->atomicEnd();
                if (is_debug_addr(addr))
                    printDataValue(addr, line->getData(), true);
            } else {
                success = false;
            }
            if (event->queryFlag(MemEvent::F_LOCKED)) {
                line->decLock();
            }

            sendTime = sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), success);
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

    return (status == MemEventStatus::Reject) ? false : true;
}


bool IncoherentL1::handleGetSX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;
    std::vector<uint8_t> data;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetSX, false, addr, state);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR); // Attempt to allocate a line if needed
            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cacheArray_->lookup(addr, false);
                // Profile
                eventProfileAndNotify(event, I, NotifyAccessType::READ, NotifyResultType::MISS, true, inMSHR);
                recordLatencyType(event->getID(), LatType::MISS);
                // Handle
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(sendTime);
                if (is_debug_addr(addr))
                    eventDI.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case E:
            line->setState(M);
        case M:
            // Profile
            eventProfileAndNotify(event, state, NotifyAccessType::READ, NotifyResultType::HIT, inMSHR, inMSHR);
            recordPrefetchResult(line, statPrefetchHit);
            recordLatencyType(event->getID(), LatType::HIT);
            // Handle
            line->incLock();
            std::copy(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr())  + event->getSize(), data.begin());
            sendTime = sendResponseUp(event, &data, inMSHR, line->getTimestamp());
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

    return (status == MemEventStatus::Reject) ? false : true;
}

/* Flush a dirty line from the cache but retain a clean copy */
bool IncoherentL1::handleFlushLine(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    }

    // At this point, state must be stable

    /* Flush fails if line is locked */
    if (state != I && line->isLocked()) {
        if (!inMSHR || !mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLine][state]->addData(1);
        }
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), false);
        recordLatencyType(event->getID(), LatType::MISS);
        cleanUpAfterRequest(event, inMSHR);

        if (is_debug_addr(addr)) {
            if (line)
                eventDI.verboseline = line->getString();
            eventDI.reason = "fail, line locked";
        }

        return true;
    }

    // At this point, we need an MSHR entry
    if (!inMSHR && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    if (!mshr_->getProfiled(addr)) {
        stat_eventState[(int)Command::FlushLine][state]->addData(1);
        mshr_->setProfiled(addr);
    }

    recordLatencyType(event->getID(), LatType::HIT);
    mshr_->setInProgress(addr);

    forwardFlush(event, line, state == M);

    if (line && state != I)
        line->setState(E_B);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


/* Flush a line from cache & invalidate it */
bool IncoherentL1::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);

    if (!inMSHR && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    }

    /* Flush fails if line is locked */
    if (state != I && line->isLocked()) {
        stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
        sendResponseUp(event, nullptr, inMSHR, line->getTimestamp(), false);
        recordLatencyType(event->getID(), LatType::MISS);
        cleanUpAfterRequest(event, inMSHR);
        if (is_debug_addr(addr)) {
            if (line)
                eventDI.verboseline = line->getString();
            eventDI.reason = "fail, line locked";
        }
        return true;
    }

    // At this point we need an MSHR entry
    if (!inMSHR && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    mshr_->setInProgress(addr);
    recordLatencyType(event->getID(), LatType::HIT);
    if (!mshr_->getProfiled(addr)) {
        stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
        if (line)
            recordPrefetchResult(line, statPrefetchEvict);
        mshr_->setProfiled(addr);
    }

    forwardFlush(event, line, state != I);
    if (state != I)
        line->setState(I_B);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool IncoherentL1::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_eventState[(int)(event->getCmd())][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);

   if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetSResp, localPrefetch, addr, state);

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(E);
    if (is_debug_addr(addr))
        printDataValue(addr, line->getData(), false);
    
    if (req->isLoadLink())
        line->atomicStart();

    if (is_debug_addr(addr))
        printDataValue(addr, line->getData(), true);

    // Notify processor or set prefetch so we can track prefetch results
    if (localPrefetch) {
        line->setPrefetch(true);
        recordPrefetchLatency(req->getID(), LatType::MISS);
        if (is_debug_addr(addr))
            eventDI.action = "Done";
    } else {
        req->setMemFlags(event->getMemFlags());
        Addr offset = req->getAddr() - req->getBaseAddr();
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


bool IncoherentL1::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    if (req->getCmd() == Command::GetS) {
        return handleGetSResp(event, inMSHR);
    }
    
    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetXResp, false, addr, state);

    req->setMemFlags(event->getMemFlags());

    // Set line data
    line->setData(event->getPayload(), 0);
    if (is_debug_addr(line->getAddr()))
        printDataValue(line->getAddr(), line->getData(), true);


    line->setState(M);

    /* Execute write */
    Addr offset = req->getAddr() - req->getBaseAddr();
    std::vector<uint8_t> data;
    bool success = true;
    if (req->getCmd() == Command::GetX || req->getCmd() == Command::Write) {
        if (!req->isStoreConditional() || line->isAtomic()) {
            line->setData(req->getPayload(), offset);
            if (is_debug_addr(line->getAddr()))
                printDataValue(line->getAddr(), line->getData(), true);
            line->atomicEnd();
        } else {
            success = false;
        }

        if (req->queryFlag(MemEventBase::F_LOCKED)) {
            line->decLock();
        }

    } else { // Read-lock
        line->incLock();
    }

    // Return response
    data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + req->getSize());
    uint64_t sendTime = sendResponseUp(req, &data, true, line->getTimestamp(), success);
    line->setTimestamp(sendTime-1);

    stat_eventState[(int)Command::GetXResp][state]->addData(1);
    if (is_debug_addr(addr)) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool IncoherentL1::handleFlushLineResp(MemEvent * event, bool inMSHR) {
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
        case E_B:
            line->setState(E);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
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

/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool IncoherentL1::handleNULLCMD(MemEvent* event, bool inMSHR) {
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
        retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
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

bool IncoherentL1::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    L1CacheLine * line = cacheArray_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(event->getBaseAddr()))
        eventDI.prefill(event->getID(), Command::NACK, false, event->getBaseAddr(), state);

    delete event;
    resendEvent(nackedEvent, false); // resend this down (since we're an L1)

    return true;
}

/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

/**
 * Handle a cache miss
 */
MemEventStatus IncoherentL1::processCacheMiss(MemEvent * event, L1CacheLine * line, bool inMSHR) {
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (status == MemEventStatus::OK && !line) { // Need a cache line too
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}


/**
 * Allocate a new MSHR entry
 */
MemEventStatus IncoherentL1::allocateMSHR(MemEvent * event, bool fwdReq, int pos) {
    int insert_pos = mshr_->insertEvent(event->getBaseAddr(), event, pos, fwdReq, false);
    if (insert_pos == -1)
        return MemEventStatus::Reject; // MSHR is full
    else if (insert_pos != 0)
        return MemEventStatus::Stall;

    return MemEventStatus::OK;
}


/**
 * Allocate a new cache line
 */
L1CacheLine* IncoherentL1::allocateLine(MemEvent * event, L1CacheLine * line) {
    bool evicted = handleEviction(event->getBaseAddr(), line);
    if (evicted) {
        if (is_debug_event(event) || is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        return line;
    } else {
        if (is_debug_event(event) || is_debug_addr(line->getAddr())) {
            eventDI.action = "Stall";
            std::stringstream st;
            st << "evict 0x" << std::hex << line->getAddr() << ", ";
            eventDI.reason = st.str();
        }
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        return nullptr;
    }
}

bool IncoherentL1::handleEviction(Addr addr, L1CacheLine*& line) {
    if (!line)
        line = cacheArray_->findReplacementCandidate(addr);
    State state = line->getState();

    if (is_debug_addr(addr))
        evictDI.oldst = line->getState();

    /* L1s can have locked cache lines */
    if (line->isLocked()) {
        if (is_debug_addr(line->getAddr()))
            printDebugAlloc(false, line->getAddr(), "InProg, line locked");
        return false;
    }

    stat_evict[state]->addData(1);

    switch (state) {
        case I:
            if (is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            return true;
        case E:
            if (is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            line->setState(I);
            break;
        case M:
            if (is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Writeback");
            sendWriteback(Command::PutM, line, true);
            cacheArray_->deallocate(line);
            line->setState(I);
            break;
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


void IncoherentL1::cleanUpAfterRequest(MemEvent* event, bool inMSHR) {
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
            if (!mshr_->getInProgress(addr)) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting for this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}

void IncoherentL1::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    mshr_->removeFront(addr); // delete req after this since debug might print the event it's removing
    delete event;
    if (req)
        delete req;

    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr)) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            }
        } else {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                retryBuffer_.push_back(ev);
            }
        }
    }
}

void IncoherentL1::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retryBuffer_.push_back(mshr_->getFrontEvent(addr));
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                retryBuffer_.push_back(ev);
            }
        }
    }
}


/***********************************************************************************************************
 * Protocol helper functions
 ***********************************************************************************************************/

uint64_t IncoherentL1::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, uint64_t time, bool success) {
    Command cmd = event->getCmd();
    MemEvent * responseEvent = event->makeResponse();

    /* Only return the desired word */
    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (is_debug_event(event)) {
            printDataValue(event->getAddr(), data, false);
        }
    }

    if (!success)
        responseEvent->setFail();

    // Compute latency, accounting for serialization of requests to the address
    if (time < timestamp_) time = timestamp_;
    uint64_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    forwardByDestination(responseEvent, deliveryTime);

    // Debugging
    if (is_debug_event(responseEvent))
        eventDI.action = "Respond";

    return deliveryTime;
}

void IncoherentL1::sendResponseDown(MemEvent * event, L1CacheLine * line, bool data) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*line->getData());
        if (line->getState() == M)
            responseEvent->setDirty(true);
    }

    responseEvent->setSize(lineSize_);

    uint64_t deliverTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    forwardByDestination(responseEvent, deliverTime);

    if (is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
    }
}


void IncoherentL1::forwardFlush(MemEvent * event, L1CacheLine * line, bool evict) {
    MemEvent * flush = new MemEvent(*event);

    uint64_t latency = tagLatency_;
    if (evict) {
        flush->setEvict(true);
        // TODO only send payload when needed
        flush->setPayload(*(line->getData()));
        flush->setDirty(line->getState() == M);
        latency = accessLatency_;
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
    if (is_debug_addr(event->getBaseAddr()))
        eventDI.action = "forward";
}

/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/

/*********************************************
 *  Methods for sending & receiving messages
 *********************************************/

/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void IncoherentL1::sendWriteback(Command cmd, L1CacheLine* line, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd, getCurrentSimTimeNano());
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    /* Writeback data */
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*line->getData());
        writeback->setDirty(dirty);

        if (is_debug_addr(line->getAddr())) {
            printDataValue(line->getAddr(), line->getData(), false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t baseTime = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    forwardByAddress(writeback, deliveryTime);
    line->setTimestamp(deliveryTime-1);

    if (is_debug_addr(line->getAddr()))
        debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", deliveryTime, CommandString[(int)cmd], ((cmd == Command::PutM || writebackCleanBlocks_) ? "" : "out"));
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void IncoherentL1::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void IncoherentL1::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}

/********************
 * Helper functions
 ********************/

MemEventInitCoherence* IncoherentL1::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true, false, false, lineSize_, true);
}

void IncoherentL1::printLine(Addr addr) {
    /*if (!is_debug_addr(addr)) return;
    L1CacheLine * line = cacheArray_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());*/
}

/* Record the result of a prefetch. important: assumes line is not null */
void IncoherentL1::recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t> * stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}

void IncoherentL1::eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool inMSHR, bool stalled) {
    if (!inMSHR || !mshr_->getProfiled(event->getBaseAddr())) {
        stat_eventState[(int)event->getCmd()][state]->addData(1); // profile
        if (result == NotifyResultType::MISS) {
            stat_misses->addData(1);
            stat_miss[(int)event->getCmd()][(int)stalled]->addData(1);
        } else {
            stat_hits->addData(1);
            stat_hit[(int)event->getCmd()][(int)stalled]->addData(1);
        }
        notifyListenerOfAccess(event, type, result);
        if (inMSHR)
            mshr_->setProfiled(event->getBaseAddr());
    }
}

void IncoherentL1::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

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

