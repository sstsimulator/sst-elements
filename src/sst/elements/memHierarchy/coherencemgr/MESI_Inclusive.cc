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

// General
#include <vector>

// SST-Core
#include <sst_config.h>

// SST-Elements
#include "coherencemgr/MESI_Inclusive.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * MESI Coherence Controller Implementation
 * Provides MESI & MSI coherence for inclusive, non-L1 caches
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

bool MESIInclusive::handleGetS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, true);
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;
    vector<uint8_t>* data;
    Command respcmd;

    if (is_debug_addr(addr))
        eventDI.prefill(event->getID(), Command::GetS, localPrefetch, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR);

            if (status == MemEventStatus::OK) { // Both MSHR insert and cache line allocation succeeded and there's no MSHR conflict
                line = cacheArray_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][I]->addData(1);
                    stat_miss[0][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, event->getSize(), 0, nullptr);
                line->setState(IS);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr);

                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case S:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::GetS][S]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                if (localPrefetch) {
                    statPrefetchRedundant->addData(1);
                    recordPrefetchLatency(event->getID(), LatType::HIT);
                } else {
                    recordLatencyType(event->getID(), LatType::HIT);
                }
            }

            if (is_debug_event(event))
                eventDI.reason = "hit";

            if (localPrefetch) {
                if (is_debug_event(event))
                    eventDI.action = "Done";
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(line, statPrefetchHit);
            line->addSharer(event->getSrc());

            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime - 1);
            cleanUpAfterRequest(event, inMSHR);

            break;
        case E:
        case M:
            if (is_debug_event(event))
                eventDI.reason = "hit";

            // Local prefetch -> drop
            if (localPrefetch) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][state]->addData(1);
                    stat_hit[0][inMSHR]->addData(1);
                    stat_hits->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::PREFETCH, NotifyResultType::HIT);
                    statPrefetchRedundant->addData(1);
                    recordPrefetchLatency(event->getID(), LatType::HIT);
                }
                if (is_debug_event(event))
                    eventDI.action = "Done";
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(line, statPrefetchHit); // Accessed a prefetched line

            if (line->hasOwner()) {
                if (!inMSHR)
                    status = allocateMSHR(event, false);

                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_eventState[(int)Command::GetS][state]->addData(1);
                        stat_hit[0][inMSHR]->addData(1);
                        stat_hits->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                        recordLatencyType(event->getID(), LatType::INV);
                        mshr_->setProfiled(addr);
                    }
                    downgradeOwner(event, line, inMSHR);
                    if (state == E)
                        line->setState(E_InvX);
                    else
                        line->setState(M_InvX);
                    mshr_->setInProgress(addr);
                }
                break;
            } else {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][state]->addData(1);
                    stat_hit[0][inMSHR]->addData(1);
                    stat_hits->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                    recordLatencyType(event->getID(), LatType::HIT);
                    if (inMSHR) mshr_->setProfiled(addr);
                }
                if (!line->hasSharers() && protocol_) {
                    line->setOwner(event->getSrc());
                    respcmd = Command::GetXResp;
                } else {
                    line->addSharer(event->getSrc());
                    respcmd = Command::GetSResp;
                }
            }

            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp(), respcmd);
            line->setTimestamp(sendTime);
            cleanUpAfterRequest(event, inMSHR);

            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            }
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject) {
        if (!localPrefetch)
            sendNACK(event);
        else
            return false;
    }

    return true;
}


bool MESIInclusive::handleGetX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), event->getCmd(), false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, inMSHR);
            if (status == MemEventStatus::OK) {
                line = cacheArray_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    recordMiss(event->getID());
                    recordLatencyType(event->getID(), LatType::MISS);
                    stat_eventState[(int)event->getCmd()][I]->addData(1);
                    stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(sendTime);
                mshr_->setInProgress(addr); // Keeps us from retrying too early due to certain race conditions between events
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case S:
            if (!lastLevel_) { // Otherwise can silently upgrade to M by falling through to next case
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_eventState[(int)event->getCmd()][state]->addData(1);
                        stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                        stat_misses->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                        mshr_->setProfiled(addr);
                    }
                    recordPrefetchResult(line, statPrefetchUpgradeMiss);
                    recordLatencyType(event->getID(), LatType::UPGRADE);

                    sendTime = forwardMessage(event, lineSize_, 0, nullptr);

                    if (invalidateExceptRequestor(event, line, inMSHR)) {
                        line->setState(SM_Inv);
                    } else {
                        line->setState(SM);
                        line->setTimestamp(sendTime);
                        if (is_debug_event(event))
                            eventDI.reason = "miss";
                    }
                    mshr_->setInProgress(addr);
                }
                break;
            }
        case E:
            line->setState(M);
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                stat_eventState[(int)event->getCmd()][state]->addData(1);
                stat_hit[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                stat_hits->addData(1);
                if (inMSHR)
                    mshr_->setProfiled(addr);
            }

            recordPrefetchResult(line, statPrefetchHit);

            if (line->hasOtherSharers(event->getSrc())) {
                if (!inMSHR)
                    status = allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setProfiled(addr);
                    invalidateExceptRequestor(event, line, inMSHR);
                    line->setState(M_Inv);
                    mshr_->setInProgress(addr);
                }
                break;
            } else if (line->hasOwner()) {
                if (!inMSHR)
                    status = allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    recordLatencyType(event->getID(), LatType::INV);
                    invalidateOwner(event, line, inMSHR, Command::FetchInv);
                    line->setState(M_Inv);
                    mshr_->setInProgress(addr);
                }
                break;
            }

            line->setOwner(event->getSrc());
            if (line->isSharer(event->getSrc()))
                line->removeSharer(event->getSrc());
            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime);

            if (is_debug_event(event))
                eventDI.reason = "hit";

            if (!inMSHR || !mshr_->getProfiled(addr))
                recordLatencyType(event->getID(), LatType::HIT);

            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleGetSX(MemEvent * event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


bool MESIInclusive::handleFlushLine(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    // Always need an MSHR for a flush
    MemEventStatus status = MemEventStatus::OK;
    if (!inMSHR) {
        status = allocateMSHR(event, false);
    } else {
        mshr_->removePendingRetry(addr);
    }

    bool ack = false;

    /*
     * To avoid races with Invs, we're going to strip the
     * evict part of a FlushLine out before we handle it
     * Then if it gets NACKed, we don't have to worry about
     * whether to retry the eviction too
     */
    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        state = doEviction(event, line, state);
        line->addSharer(event->getSrc());
        ack = true;
    }

    switch (state) {
        case I:
        case S:
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK && line->hasOwner()) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                downgradeOwner(event, line, inMSHR);
                state == M ? line->setState(M_InvX) : line->setState(E_InvX);
                status = MemEventStatus::Stall;
            }
            break;
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
                MemEvent * headEvent = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                if (headEvent->getCmd() == Command::FetchInvX && !line->hasOwner()) { // Resolve race between downgrade request & this flush
                    responses.find(addr)->second.erase(event->getSrc());
                    if (responses.find(addr)->second.empty()) responses.erase(addr);
                    retry(addr);
                }
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case E_InvX:
        case M_InvX:
            if (ack) {
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                mshr_->decrementAcksNeeded(addr);
                state == E_InvX ? line->setState(E) : line->setState(M);
                retry(addr);
            }
            // Fall through (stall the Flush/replay the waiting request)
        default:
            if (status == MemEventStatus::OK)
               status = MemEventStatus::Stall;
            break;
    }

    if (status == MemEventStatus::OK) {
        if (!mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLine][state]->addData(1);
            mshr_->setProfiled(addr);
        }
        bool downgrade = (state == E || state == M);
        forwardFlush(event, line, downgrade);
        if (line) {
            recordPrefetchResult(line, statPrefetchEvict);
            if (state != I)
                line->setState(S_B);
        }
        mshr_->setInProgress(addr);
    }

    //printLine(addr);
    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFlushLineInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);
    //printLine(addr);

    // Always need an MSHR entry
    if (!inMSHR) {
        status = allocateMSHR(event, false);
    } else {
        mshr_->removePendingRetry(addr);
    }

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (event->getEvict()) {
        state = doEviction(event, line, state);
        if (responses.find(addr) != responses.end() && responses.find(addr)->second.find(event->getSrc()) != responses.find(addr)->second.end()) {
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
        }
        if (!done) {
            done = mshr_->decrementAcksNeeded(addr);
        }
    }

    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            break;
        case S:
            if (status == MemEventStatus::OK && invalidateAll(event, line, inMSHR)) {
                line->setState(S_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case E:
            if (status == MemEventStatus::OK && invalidateAll(event, line, inMSHR)) {
                line->setState(E_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case M:
            if (status == MemEventStatus::OK && invalidateAll(event, line, inMSHR)) {
                line->setState(M_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case S_Inv:
            if (done) {
                line->setState(S);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case SB_Inv:
            if (done) {
                line->setState(S_B);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case SM_Inv:
            if (done) {
                line->setState(SM);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        default:
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
    }

    if (status == MemEventStatus::OK) {
        if (!mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
            mshr_->setProfiled(addr);
        }
        mshr_->setInProgress(addr);
        if (line)
            recordPrefetchResult(line, statPrefetchEvict);
        forwardFlush(event, line, state != I);

        if (state != I)
            line->setState(I_B);
    }

    //printLine(addr);
    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handlePutS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::PutS, false, addr, state);
        eventDI.action = "Done";
    }

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    state = doEviction(event, line, state);
    stat_eventState[(int)Command::PutS][state]->addData(1);

    if (responses.find(addr) != responses.end() && responses.find(addr)->second.find(event->getSrc()) != responses.find(addr)->second.end()) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty()) responses.erase(addr);
    }

    if (sendWritebackAck_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case S:
        case E:
        case M:
            break;
        case S_Inv:
            if (done) {
                line->setState(S);
                retry(addr);
            }
            break;
        case E_Inv:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        case SM_Inv:
            if (done) {
                line->setState(SM);
                if (!mshr_->getInProgress(addr))
                    retry(addr);    // An Inv raced with a GetX, replay the Inv
            }
            break;
        case SB_Inv:
            if (done) {
                line->setState(S_B);
                retry(addr);
            }
            break;
        case S_B:
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::PutE, false, addr, state);
        eventDI.action = "Done";
    }

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    stat_eventState[(int)Command::PutE][state]->addData(1);

    state = doEviction(event, line, state);
    if (responses.find(addr) != responses.end() && responses.find(addr)->second.find(event->getSrc()) != responses.find(addr)->second.end()) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty()) responses.erase(addr);
    }


    if (sendWritebackAck_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::PutM, false, addr, state);
        eventDI.action = "Done";
    }

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    stat_eventState[(int)Command::PutM][state]->addData(1);

    state = doEviction(event, line, state);
    if (responses.find(addr) != responses.end() && responses.find(addr)->second.find(event->getSrc()) != responses.find(addr)->second.end()) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty()) responses.erase(addr);
    }

    if (sendWritebackAck_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutX, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    stat_eventState[(int)Command::PutX][state]->addData(1);

    state = doEviction(event, line, state);
    line->addSharer(event->getSrc());

    if (sendWritebackAck_)
       sendAckPut(event);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty()) responses.erase(addr);
                retry(addr);
            }
            break;
        case E_InvX:
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
            if (mshr_->getAcksNeeded(addr) && mshr_->decrementAcksNeeded(addr)) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_InvX:
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty()) responses.erase(addr);
            if (mshr_->getAcksNeeded(addr) && mshr_->decrementAcksNeeded(addr)) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;
    return true;
}


bool MESIInclusive::handleFetch(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Fetch, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    stat_eventState[(int)Command::Fetch][state]->addData(1);

    switch (state) {
        case S:
        case SM:
        case S_B:
        case S_Inv:
        case SM_Inv:
            sendResponseDown(event, line, true, false);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    cleanUpEvent(event, inMSHR); // No replay since state doesn't change
    return true;
}


bool MESIInclusive::handleInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Inv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    bool handle = false;
    State state1, state2;
    switch (state) {
        case S:     // If sharers -> S_Inv & stall, if no sharers -> I & respond (done).
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B:   // If sharers -> SB_Inv & stall, if no sharers -> I & respond (done)
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:    // If requestor is sharer, send inv -> SM_Inv, & stall. If no sharers, -> IM & respond(done)
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_Inv:
        case SM_Inv:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            break;
        case I_B:
            line->setState(I);
        case I:
        case IS:
        case IM:
            if (is_debug_event(event))
                eventDI.action = "Drop";
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            stat_eventState[(int)Command::Inv][state]->addData(1);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (!inMSHR || mshr_->getProfiled(addr)) {
            stat_eventState[(int)Command::Inv][state]->addData(1);
            recordPrefetchResult(line, statPrefetchInv);
            if (inMSHR) mshr_->setProfiled(addr);
        }
        if (line->hasSharers() && !inMSHR)
            status = allocateMSHR(event, true, 0);
        if (status != MemEventStatus::Reject) {
            if (invalidateAll(event, line, inMSHR)) {
                line->setState(state1);
                status = MemEventStatus::Stall;
            } else {
                sendResponseDown(event, line, false, true);
                line->setState(state2);
                cleanUpAfterRequest(event, inMSHR);
            }
        }
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleForceInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::ForceInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    bool handle = false;
    bool profile = false;
    State state1, state2;
    switch (state) {
        case S: // Ack if unshared, else forward ForceInv and S_Inv
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case E: // Ack if unshared/owned. Else forward ForceInv and E_Inv
            state1 = E_Inv;
            state2 = I;
            handle = true;
            break;
        case M: // Ack if unshared/owned. Else forward ForceInv and M_Inv
            state1 = M_Inv;
            state2 = I;
            handle = true;
            break;
        case SM: // ForceInv if un-inv'd sharer & SM_Inv, else IM & ack
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_B: // if shared, forward & SB_Inv, else ack & I
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case S_Inv: // in mshr & stall
        case E_Inv: // in mshr & stall
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                status = MemEventStatus::Stall;
            }
            break;
        case M_Inv: // in mshr & stall
        case E_InvX: // in mshr & stall
        case M_InvX: // in mshr & stall
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX)) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                profile = true;
            }
            if (status != MemEventStatus::Reject) {
                status = MemEventStatus::Stall;
            } else profile = false;
            break;
        case I_B:
            line->setState(I);
        case IS:
        case IM:
        case I:
            stat_eventState[(int)Command::ForceInv][state]->addData(1);
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        case SM_Inv: { // ForceInv if there's an un-inv'd sharer, else in mshr & stall
            std::string src = mshr_->getFrontEvent(addr)->getSrc();
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                if (line->isSharer(src)) {
                    line->setTimestamp(invalidateSharer(src, event, line, inMSHR, Command::ForceInv));
                }
                status = MemEventStatus::Stall;
            }
            break;
            }
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if ((handle || profile) && (!inMSHR || !mshr_->getProfiled(addr))) {
        stat_eventState[(int)Command::ForceInv][state]->addData(1);
        recordPrefetchResult(line, statPrefetchInv);
        if (inMSHR || profile) mshr_->setProfiled(addr);
    }

    if (handle) {
        if (line->hasSharers() || line->hasOwner()) {
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                invalidateAll(event, line, inMSHR, Command::ForceInv);
                line->setState(state1);
                status = MemEventStatus::Stall;
                mshr_->setProfiled(addr);
            }
        } else {
            sendResponseDown(event, line, false, true);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFetchInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    State state1, state2;
    bool handle = false;
    bool profile = false;
    switch (state) {
        case I_B:
            line->setState(I);
        case I: // Drop
        case IS:
        case IM:
            if (is_debug_event(event))
                eventDI.action = "Drop";
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            stat_eventState[(int)Command::FetchInv][state]->addData(1);
            break;
        case S:
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case E:
            state1 = E_Inv;
            state2 = I;
            handle = true;
            break;
        case M:
            state1 = M_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_B:
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case S_Inv:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            if (status != MemEventStatus::Reject)
                status = MemEventStatus::Stall;
            break;
        case E_Inv:
        case M_Inv:
        case E_InvX:
        case M_InvX:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX)) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                profile = true;
            }
            if (status != MemEventStatus::Reject)
                status = MemEventStatus::Stall;
            else profile = false;
            break;
        case SM_Inv:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                std::string shr = mshr_->getFrontEvent(addr)->getSrc();
                if (line->isSharer(shr)) {
                    invalidateSharer(shr, event, line, inMSHR);
                }
                status = MemEventStatus::Stall;
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if ((handle || profile) && (!inMSHR || !mshr_->getProfiled(addr))) {
        stat_eventState[(int)Command::FetchInv][state]->addData(1);
        recordPrefetchResult(line, statPrefetchInv);
        if (inMSHR || profile) mshr_->setProfiled(addr);
    }

    if (handle) {
        if (line->hasSharers() || line->hasOwner()) {
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                invalidateAll(event, line, inMSHR);
                line->setState(state1);
                status = MemEventStatus::Stall;
                mshr_->setProfiled(addr);
            }
        } else {
            sendResponseDown(event, line, true, true);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFetchInvX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInvX, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (line->hasOwner()) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    downgradeOwner(event, line, inMSHR);
                    mshr_->setInProgress(addr);
                    state == E ? line->setState(E_InvX) : line->setState(M_InvX);
                    status = MemEventStatus::Stall;
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::FetchInvX][state]->addData(1);
                }
                break;
            }
            sendResponseDown(event, line, true, true);
            line->setState(S);
            cleanUpAfterRequest(event, inMSHR);
            stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            break;
        case M_Inv:
        case E_Inv:
        case E_InvX:
        case M_InvX:
            // GetX is handled internally so the FetchInvX should stall for it
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS)) {
                status = inMSHR ? MemEventStatus::Stall : allocateMSHR(event, true, 1);
            } else if (line->hasOwner()) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
                mshr_->setProfiled(addr);
                if (status != MemEventStatus::Reject)
                    status = MemEventStatus::Stall;
            } else { // E_InvX/M_InvX never reach this bit
                line->setState(S_Inv);
                sendResponseDown(event, line, true, true);
                cleanUpAfterRequest(event, inMSHR);
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            }
            break;
        case S_B:
        case I_B:
        case IS:
        case IM:
        case I:
            if (is_debug_event(event))
                eventDI.action = "Drop";
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESIInclusive::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetSResp, false, addr, state);

    stat_eventState[(int)Command::GetSResp][state]->addData(1);

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    //if (is_debug_addr(addr))
        //debug->debug(_L5_, "    Request: %s\n", req->getBriefString().c_str());
    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    // Sanity check line state
    if (state != IS) {
        debug->fatal(CALL_INFO,-1,"%s, Error: Received GetSResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(line->getAddr()))
        printData(line->getData(), true);

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(S);
    if (localPrefetch) {
        line->setPrefetch(true);
    } else {
        line->addSharer(req->getSrc());
        Addr offset = req->getAddr() - req->getBaseAddr();
        uint64_t sendTime = sendResponseUp(req, line->getData(), true, line->getTimestamp());
        line->setTimestamp(sendTime-1);

    }

    if (is_debug_addr(line->getAddr()))
        printData(line->getData(), true);

    cleanUpAfterResponse(event, inMSHR);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIInclusive::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetXResp, false, addr, state);

    stat_eventState[(int)Command::GetXResp][state]->addData(1);

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - req->getBaseAddr();

    switch (state) {
        case IS:
        {
            line->setData(event->getPayload(), 0);
            if (is_debug_addr(line->getAddr()))
                printData(line->getData(), true);

            if (event->getDirty())  {
                line->setState(M); // Sometimes get dirty data from a noninclusive cache
            } else {
                line->setState(protocolState_);
            }

            if (localPrefetch) {
                line->setPrefetch(true);
                if (is_debug_event(event))
                    eventDI.action = "Done";
            } else {
                if (protocol_ && line->getState() != S && mshr_->getSize(addr) == 1) {
                    line->setOwner(req->getSrc());
                    uint64_t sendTime = sendResponseUp(req, line->getData(), true, line->getTimestamp(), Command::GetXResp);
                    line->setTimestamp(sendTime - 1);
                } else {
                    line->addSharer(req->getSrc());
                    uint64_t sendTime = sendResponseUp(req, line->getData(), true, line->getTimestamp(), Command::GetSResp);
                    line->setTimestamp(sendTime - 1);
                }
            }
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case IM:
            line->setData(event->getPayload(), 0);
            if (is_debug_addr(line->getAddr()))
                printData(line->getData(), true);
        case SM:
        {
            line->setState(M);
            line->setOwner(req->getSrc());
            if (line->isSharer(req->getSrc()))
                line->removeSharer(req->getSrc());

            uint64_t sendTime = sendResponseUp(req, line->getData(), true, line->getTimestamp());
            line->setTimestamp(sendTime-1);
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case SM_Inv:
            if (is_debug_event(event))
                eventDI.action = "Stall";
            line->setState(M_Inv);
            delete event;
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    return true;
}


bool MESIInclusive::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineResp, false, addr, state);

    stat_eventState[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool MESIInclusive::handleFetchResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line->getState();

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchResp, false, addr, state);

    stat_eventState[(int)Command::FetchResp][state]->addData(1);

    // Check acks needed
    mshr_->decrementAcksNeeded(addr);

    // Do invalidation & update data
    state = doEviction(event, line, state);
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    if (state == M_Inv) {
        line->setState(M);
    } else {
        line->setState(E);
    }
    retry(addr);

    if (is_debug_addr(addr)) {
        eventDI.action = "Retry";
        if (line) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleFetchXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line->getState();

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchXResp, false, addr, state);

    stat_eventState[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);

    state = doEviction(event, line, state);
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);
    line->addSharer(event->getSrc());

    if (state == M_InvX)
        line->setState(M);
    else
        line->setState(E);

    retry(addr);

    if (is_debug_addr(addr)) {
        eventDI.action = "Retry";
        if (line) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleAckInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line->getState();

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::AckInv, false, addr, state);

    stat_eventState[(int)Command::AckInv][state]->addData(1);

    if (line->isSharer(event->getSrc()))
        line->removeSharer(event->getSrc());
    else
        line->removeOwner();

    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    bool done = mshr_->decrementAcksNeeded(addr);
    if (!done) {
        if (is_debug_event(event))
            eventDI.action = "Stall";
        delete event;
        return true;
    }

    switch (state) {
        case S_Inv:
            line->setState(S);
            retry(addr);
            break;
        case E_Inv:
            line->setState(E);
            retry(addr);
            break;
        case M_Inv:
            line->setState(M);
            retry(addr);
            break;
        case SB_Inv:
            line->setState(S_B);
            retry(addr);
            break;
        case SM_Inv:
            line->setState(SM);
            if (!mshr_->getInProgress(addr))
                retry(addr);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received AckInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr)) {
        eventDI.action = (done ? "Retry" : "DecAcks");
        if (line) {
            eventDI.newst = line->getState();
            eventDI.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleAckPut(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::AckPut, false, addr, state);
        eventDI.action = "Done";
    }

    stat_eventState[(int)Command::AckPut][state]->addData(1);

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIInclusive::handleNULLCMD(MemEvent* event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    SharedCacheLine * line = cacheArray_->lookup(oldAddr, false);

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
    } else { // Check if we're waiting for a new address
        if (is_debug_addr(newAddr)) {
            eventDI.action = "Stall";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr << ", evict failed";
            eventDI.reason = reason.str();
        }
        if (oldAddr != line->getAddr()) { // We're waiting for a new line now...
            if (is_debug_addr(newAddr)) {
                std::stringstream reason;
                reason << "0x" << std::hex << newAddr << ", new targ";
                eventDI.reason = reason.str();
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    delete event;
    return true;
}

bool MESIInclusive::handleNACK(MemEvent * event, bool inMSHR) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();

    if (is_debug_event(event) || is_debug_event(nackedEvent)) {
        SharedCacheLine * line = cacheArray_->lookup(addr, false);
        eventDI.prefill(event->getID(), Command::NACK, false, addr, line ? line->getState() : I);
    }

    delete event;

    switch (cmd) {
        /* These always need to get retried */
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLineInv:
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        /* These *probably* need to get retried but need to handle races with Invs */
        case Command::FlushLine:
            resendEvent(nackedEvent, false);
            break;
        /* These get retried unless there's been a race with an eviction/writeback */
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            if (responses.find(addr) != responses.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst()) != responses.find(addr)->second.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst())->second == nackedEvent->getID()) {
                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (is_debug_event(nackedEvent))
                    eventDI.action = "Drop";
                    //debug->debug(_L5_, "\tDrop NACKed event: %s\n", nackedEvent->getVerboseString().c_str());
                delete nackedEvent;
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received NACK with unhandled command type. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), nackedEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return true;
}


/***********************************************************************************************************
 * MSHR and CacheArray management
 ***********************************************************************************************************/

MemEventStatus MESIInclusive::processCacheMiss(MemEvent * event, SharedCacheLine * line, bool inMSHR) {
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (inMSHR && mshr_->getFrontEvent(event->getBaseAddr()) != event) { // Sometimes happens if eviction and event both retry
        if (is_debug_event(event))
            eventDI.action = "Stall";
        return MemEventStatus::Stall;
    }
    if (status == MemEventStatus::OK && !line) { // Need a cache line too
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}


SharedCacheLine * MESIInclusive::allocateLine(MemEvent * event, SharedCacheLine * line) {
    bool evicted = handleEviction(event->getBaseAddr(), line);
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return line;
    } else {
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            std::stringstream reason;
            reason << "evict 0x" << std::hex << line->getAddr();
            eventDI.reason = reason.str();
        }
        return nullptr;
    }
}


bool MESIInclusive::handleEviction(Addr addr, SharedCacheLine *& line) {
    if (!line)
        line = cacheArray_->findReplacementCandidate(addr);
    State state = line->getState();

    if (is_debug_addr(addr) || (line && is_debug_addr(line->getAddr())))
        evictDI.oldst = state;

    stat_evict[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(S_Inv);
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, S_Inv");
                    break;
                } else if (!silentEvictClean_) {
                    sendWriteback(Command::PutS, line, false);
                    wbSent = true;
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Drop");
                line->setState(I);
                evict = true;
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(E_Inv);
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, E_Inv");
                    break;
                } else if (!silentEvictClean_) {
                    sendWriteback(Command::PutE, line, false);
                    wbSent = true;
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Drop");
                line->setState(I);
                evict = true;
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(M_Inv);
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, M_Inv");
                } else {
                    sendWriteback(Command::PutM, line, true);
                    wbSent = true;
                    line->setState(I);
                    evict = true;
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                }
                break;
            }
        default:
            if (is_debug_addr(line->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    if (wbSent && recvWritebackAck_) {
        mshr_->insertWriteback(line->getAddr(), false);
    }

    recordPrefetchResult(line, statPrefetchEvict);
    return evict;
}


void MESIInclusive::cleanUpEvent(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (inMSHR) {
        mshr_->removeFront(addr);
    }

    delete event;
}


void MESIInclusive::cleanUpAfterRequest(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    cleanUpEvent(event, inMSHR);
    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting for this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict && mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}


void MESIInclusive::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    }
    mshr_->removeFront(addr);
    delete event;
    if (req)
        delete req;

    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else {
            if (mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}


void MESIInclusive::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            //if (is_debug_addr(addr))
            //    debug->debug(_L5_, "    Retry: Waiting Event exists in MSHR and is ready to retry\n");
            retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
        } else if (!(mshr_->pendingWriteback(addr))) {
            //if (is_debug_addr(addr))
            //    debug->debug(_L5_, "    Retry: Waiting Evict in MSHR, retrying eviction\n");
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
        }
    }
}


State MESIInclusive::doEviction(MemEvent * event, SharedCacheLine * line, State state) {
    State nState = state;
    recordPrefetchResult(line, statPrefetchEvict);

    if (event->getDirty()) {
        line->setData(event->getPayload(), 0);
        switch (state) {
            case E:
                nState = M;
                break;
            case E_Inv:
                nState = M_Inv;
                break;
            case E_InvX:
                nState = M_InvX;
                break;
            default: // Already in a dirty state
                break;
        }
    }
    if (line->getOwner() == event->getSrc())
        line->removeOwner();
    else if (line->isSharer(event->getSrc()))
        line->removeSharer(event->getSrc());

    event->setEvict(false); // Avoid doing an eviction twice if the event gets replayed
    line->setState(nState);
    return nState;
}


/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

SimTime_t MESIInclusive::sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

    /* Only return the desired word */
    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (is_debug_event(event)) {
            printData(data, false);
        }
    }

    if (success)
        responseEvent->setSuccess(true);


    // Compute latency, accounting for serialization of requests to the address
    if (time < timestamp_) time = timestamp_;
    uint64_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);

    // Debugging
    if (is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
    }

    return deliveryTime;
}


void MESIInclusive::sendResponseDown(MemEvent * event, SharedCacheLine * line, bool data, bool evict) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*line->getData());
        if (line->getState() == M)
            responseEvent->setDirty(true);
    }

    responseEvent->setEvict(evict);

    responseEvent->setSize(lineSize_);

    uint64_t deliverTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    Response resp = {responseEvent, deliverTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueue(resp);

    if (is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
    }
}


void MESIInclusive::forwardFlush(MemEvent * event, SharedCacheLine * line, bool evict) {
    MemEvent * flush = new MemEvent(*event);

    flush->setSrc(cachename_);
    flush->setDst(getDestination(event->getBaseAddr()));

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
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (line)
        line->setTimestamp(deliveryTime-1);

    if (is_debug_addr(event->getBaseAddr())) {
        eventDI.action = "Forward";
    //    debug->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", deliveryTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str(), evict ? "with data" : "without data");
    }
}

/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIInclusive::sendWriteback(Command cmd, SharedCacheLine* line, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setDst(getDestination(line->getAddr()));
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    /* Writeback data */
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

   //f (is_debug_addr(line->getAddr()))
        //debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", deliveryTime, CommandString[(int)cmd], ((cmd == Command::PutM || writebackCleanBlocks_) ? "" : "out"));
}


void MESIInclusive::sendAckPut(MemEvent * event) {
    MemEvent * ack = event->makeResponse();
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t deliveryTime = timestamp_ + tagLatency_;

    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);

    if (is_debug_event(event))
        eventDI.action = "Ack";
        //debug->debug(_L7_, "Sending AckPut at cycle %" PRIu64 "\n", deliveryTime);
}


void MESIInclusive::downgradeOwner(MemEvent * event, SharedCacheLine* line, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    MemEvent * fetch = new MemEvent(cachename_, addr, addr, Command::FetchInvX);
    fetch->copyMetadata(event);
    fetch->setDst(line->getOwner());
    fetch->setSize(lineSize_);

    mshr_->incrementAcksNeeded(addr);

    if (responses.find(addr) != responses.end()) {
        responses.find(addr)->second.insert(std::make_pair(line->getOwner(), fetch->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(line->getOwner(), fetch->getID()));
        responses.insert(std::make_pair(addr, respid));
    }

    uint64_t baseTime = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    line->setTimestamp(deliveryTime);


    if (is_debug_addr(line->getAddr())) {
        eventDI.action = "Stall";
        eventDI.reason = "Dgr owner";
        //debug->debug(_L7_, "Sending %s: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n",
        //        CommandString[(int)cmd], line->getAddr(), line->getOwner().c_str(), deliveryTime);
    }
}


bool MESIInclusive::invalidateExceptRequestor(MemEvent * event, SharedCacheLine * line, bool inMSHR) {
    uint64_t deliveryTime = 0;
    std::string rqstr = event->getSrc();

    for (set<std::string>::iterator it = line->getSharers()->begin(); it != line->getSharers()->end(); it++) {
        if (*it == rqstr) continue;

        deliveryTime =  invalidateSharer(*it, event, line, inMSHR);
    }

    if (deliveryTime != 0) line->setTimestamp(deliveryTime);

    return deliveryTime != 0;
}


bool MESIInclusive::invalidateAll(MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd) {
    uint64_t deliveryTime = 0;
    if (invalidateOwner(event, line, inMSHR, (cmd == Command::NULLCMD ? Command::FetchInv : cmd))) {
        return true;
    } else {
        if (cmd == Command::NULLCMD)
            cmd = Command::Inv;
        for (std::set<std::string>::iterator it = line->getSharers()->begin(); it != line->getSharers()->end(); it++) {
            deliveryTime = invalidateSharer(*it, event, line, inMSHR, cmd);
        }
        if (deliveryTime != 0) {
            line->setTimestamp(deliveryTime);
            return true;
        }
    }
    return false;
}

uint64_t MESIInclusive::invalidateSharer(std::string shr, MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd) {
    if (line->isSharer(shr)) {
        Addr addr = line->getAddr();
        MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
        if (event) {
            inv->copyMetadata(event);
            inv->setRqstr(event->getRqstr());
        } else {
            inv->setRqstr(cachename_);
        }
        inv->setDst(shr);
        inv->setSize(lineSize_);
        if (responses.find(addr) != responses.end()) {
            responses.find(addr)->second.insert(std::make_pair(shr, inv->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
        } else {
            std::map<std::string,MemEvent::id_type> respid;
            respid.insert(std::make_pair(shr, inv->getID()));
            responses.insert(std::make_pair(addr, respid));
        }

        uint64_t baseTime = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
        uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(addr);

        if (is_debug_addr(addr)) {
            eventDI.action = "Stall";
            eventDI.reason = "Inv sharer(s)";
        }
        return deliveryTime;
    }
    return 0;
}


bool MESIInclusive::invalidateOwner(MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd) {
    Addr addr = line->getAddr();
    if (line->getOwner() == "")
        return false;

    MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
    if (event) {
        inv->copyMetadata(event);
        inv->setRqstr(event->getRqstr());
    } else {
        inv->setRqstr(cachename_);
    }
    inv->setDst(line->getOwner());
    inv->setSize(lineSize_);

    mshr_->incrementAcksNeeded(addr);

    // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    if (responses.find(addr) != responses.end()) {
        responses.find(addr)->second.insert(std::make_pair(inv->getDst(), inv->getID()));
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(inv->getDst(), inv->getID()));
        responses.insert(std::make_pair(addr,respid));
    }

    uint64_t baseTime = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {inv, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    line->setTimestamp(deliveryTime);

    if (is_debug_addr(addr)) {
        eventDI.action = "Stall";
        eventDI.reason = "Inv owner";
    }
    return true;
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/

void MESIInclusive::addToOutgoingQueue(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueue(resp);
}


void MESIInclusive::addToOutgoingQueueUp(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueueUp(resp);
}


/***********************************************************************************************************
 * Statistics and listeners
 ***********************************************************************************************************/

void MESIInclusive::recordPrefetchResult(SharedCacheLine * line, Statistic<uint64_t> * stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}


void MESIInclusive::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

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
            stat_latencyFlushLine->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latencyFlushLineInv->addData(latency);
            break;
        default:
            break;
    }
}


/***********************************************************************************************************
 * Statistics and listeners
 ***********************************************************************************************************/

MemEventInitCoherence * MESIInclusive::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true /* inclusive */, false /* sends WBAck */, false /* expects WBAck */, lineSize_, true /* tracks block presence */);
}


void MESIInclusive::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());

    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");
    */
}


void MESIInclusive::printLine(Addr addr) {
    if (!is_debug_addr(addr))
        return;

    SharedCacheLine * line = cacheArray_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());
}


void MESIInclusive::printStatus(Output &out) {
    cacheArray_->printCacheArray(out);
}

