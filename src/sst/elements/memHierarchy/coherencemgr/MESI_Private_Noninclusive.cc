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
#include "coherencemgr/MESI_Private_Noninclusive.h"

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
 * MESI/MSI Non-Inclusive Coherence Controller for private cache
 *
 * This protocol does not support prefetchers because the cache can't determine whether an address would be a hit
 * in an upper level (closer to cpu) cache.
 *
 *---------------------------------------------------------------------------------------------------------------------*/

bool MESIPrivNoninclusive::handleGetS(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;
    Command respcmd;
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetS, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][I]->addData(1);
                    stat_miss[0][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                sendTime = forwardMessage(event, event->getSize(), 0, nullptr);
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
            }
            line->setShared(true);
            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            recordLatencyType(event->getID(), LatType::HIT);
            line->setTimestamp(sendTime);
            if (is_debug_event(event))
                eventDI.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E:
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::GetS][state]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            }
            if (is_debug_event(event))
                eventDI.reason = "hit";
            if (protocol_) { // Transfer ownership of dirty block
                line->setOwned(true);
                sendTime = sendExclusiveResponse(event, line->getData(), inMSHR, line->getTimestamp(), state == M);
            } else { // Will writeback dirty block if we evict
                line->setShared(true);
                sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp(), Command::GetSResp);
            }
            recordLatencyType(event->getID(), LatType::HIT);
            line->setTimestamp(sendTime);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            else if (is_debug_event(event))
                eventDI.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), event->getCmd(), false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (state == S && lastLevel_) { // Special case, silent upgrade
        state = M;
        line->setState(M);
    }

    switch (state) {
        case I:
        case S:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)event->getCmd()][state]->addData(1);
                    stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, event->getSize(), 0, nullptr);
                mshr_->setInProgress(addr);
                if (state == S) {
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    line->setState(SM);
                    line->setTimestamp(sendTime);
                } else {
                    recordLatencyType(event->getID(), LatType::MISS);
                }
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case E:
            line->setState(M);
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)event->getCmd()][state]->addData(1);
                stat_hit[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            }
            line->setOwned(true);
            line->setShared(false);
            sendTime = sendExclusiveResponse(event, line->getData(), inMSHR, line->getTimestamp(), true);
            line->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::HIT);
            if (is_debug_event(event))
                eventDI.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (is_debug_event(event))
                eventDI.action = "Stall";
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetSX(MemEvent* event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


bool MESIPrivNoninclusive::handleFlushLine(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

    recordLatencyType(event->getID(), LatType::HIT);

    uint64_t sendTime;

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                event->setEvict(false);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            } else if (mshr_->getAcksNeeded(addr) != 0 && event->getEvict()) {
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                event->setEvict(false);
                if ((static_cast<MemEvent*>(mshr_->getFrontEvent(addr)))->getCmd() == Command::FetchInvX) {
                    responses.erase(addr);
                    mshr_->decrementAcksNeeded(addr);
                    retry(addr);
                }
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                sendTime = forwardFlush(event, false, nullptr, false, line->getTimestamp());
                line->setTimestamp(sendTime);
                line->setState(S_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    line->setOwned(false);
                    line->setShared(true);
                    if (event->getDirty()) {
                        line->setData(event->getPayload(), 0);
                    }
                    event->setEvict(false);
                }
                forwardFlush(event, true, line->getData(), (state == M || event->getDirty()), line->getTimestamp());
                line->setState(S_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E_Inv:
        case M_Inv:
            if (event->getEvict()) {
                line->setOwned(false);
                line->setShared(true);
                if (event->getDirty()) {
                    line->setData(event->getPayload(), 0);
                    line->setState(M_Inv);
                }
                event->setEvict(false);
            }
            break;
        case E_InvX:
        case M_InvX:
            state == E_InvX ? line->setState(E) : line->setState(M);
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setData(event->getPayload(), 0);
                line->setState(M_Inv);
            }
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            break;
        default:
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFlushLineInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

    recordLatencyType(event->getID(), LatType::HIT);

    uint64_t sendTime;

    switch (state) {
        case I:
            if (inMSHR && mshr_->getInProgress(addr))
                break; // Triggered an unneccessary retry
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0); // No need to evict since we didn't race
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            } else if (event->getEvict()) {
                MemEventBase * race;
                if (mshr_->getFrontEvent(addr)) { // There's an event with raced with
                    race = mshr_->getFrontEvent(addr);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    retry(addr);
                    if (is_debug_event(event))
                        eventDI.action = "Retry";
                } else if (status == MemEventStatus::Stall && mshr_->pendingWritebackIsDowngrade(addr) && mshr_->getEntryEvent(addr, 1)) { // There's an event we *will* race with
                    // In this case this cache issued a downgrade (PutX), an invalidation was received & stalled pending the AckPut and now we've got a flushlineinv
                    // Resolve race with the invalidation
                    race = mshr_->getEntryEvent(addr, 1);
                } else
                    break;

                // Copy data in and update state to resolve race with conflicting event
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                if (race->getCmd() == Command::FetchInvX) {
                    event->setDirty(false);
                } else if (race->getCmd() != Command::Fetch) { // FetchInv, ForceInv, or Inv
                    event->setDirty(false);
                    event->setEvict(false);
                }
            }
            /* Note: If there's a WB-X followed by a FetchInv/Inv/etc. in the MSHR, the FetchInv will get replayed when the WB-X resolves
             * and will not forward since we copied the evict data into the MSHR.
             */
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (event->getEvict())
                    line->setShared(false);
                line->setState(I_B);
                forwardFlush(event, true, line->getData(), false, line->getTimestamp());
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    line->setOwned(false);
                    line->setShared(false);
                    if (event->getDirty()) {
                        line->setData(event->getPayload(), 0);
                        line->setState(M);
                    }
                }
                forwardFlush(event, true, line->getData(), line->getState() == M, line->getTimestamp());
                line->setState(I_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case S_Inv:
            line->setShared(false);
            line->setState(S);
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (is_debug_event(event))
                eventDI.action = "Retry";
            break;
        case E_Inv:
        case E_InvX:
            line->setOwned(false);
            line->setShared(false);
            if (event->getDirty()) {
                line->setData(event->getPayload(), 0);
                line->setState(M);
            } else {
                line->setState(E);
            }
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (is_debug_event(event))
                eventDI.action = "Retry";
            break;
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            line->setShared(false);
            if (event->getDirty())
                line->setData(event->getPayload(), 0);
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (is_debug_event(event))
                eventDI.action = "Retry";
            break;
        default:
            if (inMSHR && is_debug_event(event))
                eventDI.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handlePutS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutS, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::PutS][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (!inMSHR && mshr_->exists(addr)) { // Raced with something; must be an Inv/Fetch since there can only be one cache above us
                mshr_->setData(addr, event->getPayload(), false);
                responses.erase(addr);
                mshr_->decrementAcksNeeded(addr);
                if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::Fetch) {
                    status = allocateLine(event, line, false);
                    if (status == MemEventStatus::OK) {
                        line->setState(S);
                        line->setData(event->getPayload(), 0);
                        mshr_->clearData(addr);
                        sendWritebackAck(event);
                        cleanUpAfterRequest(event, inMSHR);
                        break;
                    }
                } else { // Inv of some sort
                    sendWritebackAck(event);
                    delete event;
                }
                retry(addr);
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    line->setState(S);
                    line->setData(event->getPayload(), 0);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    sendWritebackAck(event);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case S:
        case E:
        case M:
            line->setShared(false);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_Inv:
            line->setShared(false);
            line->setState(S);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case E_Inv:
            line->setShared(false);
            line->setState(E);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case M_Inv:
            line->setShared(false);
            line->setState(M);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutE, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::PutE][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr) != 0) { // Race
                if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    mshr_->setData(addr, event->getPayload(), false);
                    event->setCmd(Command::PutS);
                    event->setDirty(false);
                    retry(addr);
                    status = allocateMSHR(event, false, 1, true);
                } else {
                    mshr_->setData(addr, event->getPayload(), false);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    sendWritebackAck(event);
                    delete event;
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    event->getDirty() ? line->setState(M) : line->setState(E);
                    line->setData(event->getPayload(), 0);
                    sendWritebackAck(event);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case E_InvX:
            line->setOwned(false);
            responses.erase(addr);
            line->setState(E);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            responses.erase(addr);
            line->setState(M);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());

    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutM, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::PutM][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr) != 0) { // Race with eviction, FetchInvX, or FetchInv/Inv/ForceInv
                if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    mshr_->setData(addr, event->getPayload(), true);
                    event->setCmd(Command::PutS);
                    event->setDirty(false);
                    retry(addr);
                    status = allocateMSHR(event, false, 1);
                } else { // Eviction or invalidation -> we won't need a line
                    mshr_->setData(addr, event->getPayload(), true);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    sendWritebackAck(event);
                    delete event;
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    line->setState(M);
                    line->setData(event->getPayload(), 0);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    sendWritebackAck(event);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            line->setState(M);
            line->setData(event->getPayload(), 0);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            line->setState(M);
            responses.erase(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
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


bool MESIPrivNoninclusive::handlePutX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

//    printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutX, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::PutX][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr)) {
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                sendWritebackAck(event);
                delete event;

                if (mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    event->getDirty() ? line->setState(M) : line->setState(E);
                    line->setData(event->getPayload(), 0);
                    sendWritebackAck(event);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M);
                line->setData(event->getPayload(), 0);
            }
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case M_Inv:
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M_Inv);
                line->setData(event->getPayload(), 0);
            }
            sendWritebackAck(event);
            delete event;
            break;
        case E_InvX:
        case M_InvX:
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M);
                line->setData(event->getPayload(), 0);
            } else {
                line->setState(E);
            }
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetch(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Fetch, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::Fetch][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->hasData(addr)) {
                sendResponseDown(event, event->getSize(), &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                cleanUpAfterRequest(event, inMSHR);
            } else if (!inMSHR && mshr_->exists(addr)) {
                if (mshr_->getFrontType(addr) == MSHREntryType::Writeback || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {  // Raced with eviction
                    if (is_debug_event(event)) {
                        eventDI.action = "Drop";
                        eventDI.reason = "MSHR conflict";
                    }
                    delete event;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutS) { // Raced with replacement
                    MemEvent* put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendResponseDown(event, event->getSize(), &(put->getPayload()), false);
                    delete event;
                } else { // Raced with GetX or FlushLine
                    status = allocateMSHR(event, true, 0);
                    sendFwdRequest(event, Command::Fetch, upperCacheName_, event->getSize(), 0, inMSHR);
                }
            } else {
                if (!inMSHR)
                    status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK)
                    sendFwdRequest(event, Command::Fetch, upperCacheName_, event->getSize(), 0, inMSHR);
            }
            break;
        case S:
        case SM:
        case S_B:
        case S_Inv:
            sendResponseDown(event, event->getSize(), line->getData(), false);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case I_B:
            if (is_debug_event(event)) {
                eventDI.action = "Drop";
                eventDI.reason = "MSHR conflict";
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Inv, false, addr, state);

    bool handle = false;
    State state1, state2;
    MemEventBase * req;

    if (!inMSHR)
        stat_eventState[(int)Command::Inv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            // GetX     Forward Inv & insert ahead
            // FL       Forward Inv & insert ahead
            if (mshr_->exists(addr) && (mshr_->getFrontType(addr) == MSHREntryType::Writeback || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) { // Already evicted block, drop the Inv
                if (is_debug_event(event)) {
                    eventDI.action = "Drop";
                    eventDI.reason = "MSHR conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutS) {
                // Handle Inv
                sendResponseDown(event, event->getSize(), nullptr, false);
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                // Drop PutS
                sendWritebackAck(static_cast<MemEvent*>(mshr_->getFrontEvent(addr)));
                req = mshr_->getFrontEvent(addr);
                mshr_->removeFront(addr);
                delete req;
                // Clean up
                cleanUpAfterRequest(event, inMSHR);
            } else if (mshr_->exists(addr) && mshr_->hasData(addr)) { // Race with Put (replay)
                // Handle Inv
                sendResponseDown(event, event->getSize(), nullptr, false);
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                // Clean up
                cleanUpAfterRequest(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::Inv, upperCacheName_, event->getSize(), 0, inMSHR);
                }
            }
            break;
        case S:
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B:   // If sharers -> SB_Inv & stall, if no sharers -> I & respond (done)
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:    // If requestor is sharer, send inv -> SM_Inv, & stall. If no sharers, -> I & respond(done)
            state1 = SM_Inv;
            state2 = I;
            handle = true;
            break;
        case I_B:
            if (is_debug_event(event)) {
                eventDI.action = "Drop";
                eventDI.reason = "MSHR conflict";
            }
            line->setState(I);
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }


    if (handle) {
        if (line->getShared()) {
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::Inv, upperCacheName_, event->getSize(), line->getTimestamp(), inMSHR);
                line->setState(state1);
                line->setTimestamp(sendTime);
            }
        } else {
            sendResponseDown(event, event->getSize(), line->getData(), false);
            line->setState(state2);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleForceInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::ForceInv, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::ForceInv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    bool handle = false;
    State state1, state2;
    MemEventBase * entry;
    switch (state) {
        case I:
            if (mshr_->pendingWritebackIsDowngrade(addr)) { // Waiting for AckPut to PutX, go ahead with the force inv
                entry = mshr_->getEntryEvent(addr, 1);
                if (entry) {
                    if (entry->getCmd() == Command::PutS) {
                        // Handle ForceInv
                        sendResponseDown(event, event->getSize(), nullptr, false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop PutS
                        sendWritebackAck(static_cast<MemEvent*>(entry));
                        delete entry;
                        mshr_->removeEntry(addr, 1);
                        break;
                    } else if (entry->getCmd() == Command::FlushLineInv) {
                        // Handle ForceInv
                        sendResponseDown(event, event->getSize(), nullptr, false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop evict part of Flush if needed
                        MemEvent* flush = static_cast<MemEvent*>(entry);
                        if (flush->getEvict())
                            flush->setEvict(false);
                        break;
                    }
                }
                status = allocateMSHR(event, true, 1); // Retry when we get the AckPut
            } else if (mshr_->pendingWriteback(addr) || (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                if (is_debug_event(event)) {
                    eventDI.action = "Drop";
                    eventDI.reason = "MSHR conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) { // Drop PutX, Ack it, forward request up
                MemEvent * put = static_cast<MemEvent*>(mshr_->swapFrontEvent(addr, event));
                sendWritebackAck(put);
                mshr_->setData(addr, put->getPayload(), put->getDirty());
                delete put;
                sendFwdRequest(event, Command::ForceInv, upperCacheName_, event->getSize(), 0, inMSHR);
            } else if (mshr_->exists(addr) && (CommandWriteback[(int)mshr_->getFrontEvent(addr)->getCmd()])) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                sendWritebackAck(static_cast<MemEvent*>(mshr_->getFrontEvent(addr)));
                delete mshr_->getFrontEvent(addr);
                mshr_->removeFront(addr);
                sendResponseDown(event, event->getSize(), nullptr, false);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::ForceInv, upperCacheName_, event->getSize(), 0, inMSHR);
                }
            }
            break;
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
        case SM: // ForceInv if un-inv'd sharer & SM_Inv, else I & ack
            state1 = SM_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B: // if shared, forward & SB_Inv, else ack & I
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case I_B:
            line->setState(I);
            if (is_debug_event(event)) {
                eventDI.action = "Drop";
                eventDI.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        case SM_Inv: { // ForceInv if there's an un-inv'd sharer, else in mshr & stall
            std::string src = mshr_->getFrontEvent(addr)->getSrc();
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                if (line->getShared()) {
                    uint64_t sendTime = sendFwdRequest(event, Command::ForceInv, src, lineSize_, line->getTimestamp(), inMSHR);
                    line->setTimestamp(sendTime);
                }
                status = MemEventStatus::Stall;
            }
            break;
            }
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (line->getShared() || line->getOwned()) {
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::ForceInv, upperCacheName_, event->getSize(), 0, inMSHR);
                line->setTimestamp(sendTime);
                line->setState(state1);
                status = MemEventStatus::Stall;
            }
        } else {
            sendResponseDown(event, event->getSize(), nullptr, false);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchInv(MemEvent * event, bool inMSHR){
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInv, false, addr, state);

    if (!inMSHR)
        stat_eventState[(int)Command::FetchInv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    State state1, state2;
    bool handle = false;
    MemEventBase* entry;
    switch (state) {
        case I:
            if (inMSHR && mshr_->hasData(addr)) { // Replay
                sendResponseDown(event, event->getSize(), &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                mshr_->clearData(addr);
                cleanUpAfterRequest(event, inMSHR);
            } else if (mshr_->pendingWritebackIsDowngrade(addr)) { // Resolve races with pending evictions from upper level caches
                /* In this case we're waiting for an AckPut for a PutX so we've gone from M/E to S. Check if any
                 * queued events are evictions and if so, handle them. Otherwise stallthe FetchInv until the writeback ack returns.
                 * Note that the next level can NACK a PutX if it is noninclusive and in a stable state,
                 * but it won't NACK the PutX if it's in a transition state.
                 * So stalling the response won't cause deadlock.
                 */
                entry = mshr_->getEntryEvent(addr, 1);
                if (entry) {
                    if (entry->getCmd() == Command::PutS) {
                        // Return AckInv
                        sendResponseDown(event, event->getSize(), &(static_cast<MemEvent*>(entry)->getPayload()), false);
                        delete event;
                        // Drop PutS
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        sendWritebackAck(static_cast<MemEvent*>(entry));
                        delete entry;
                        mshr_->removeEntry(addr, 1);
                        break;
                    } else if (entry->getCmd() == Command::FlushLineInv) {
                        // Handle FetchInv
                        sendResponseDown(event, event->getSize(), &(static_cast<MemEvent*>(entry)->getPayload()), false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop evict part of Flush if needed
                        MemEvent* flush = static_cast<MemEvent*>(entry);
                        if (flush->getEvict())
                            flush->setEvict(false);
                        break;
                    }
                }
                // Stall the event until the writeback ack returns (don't actually have to but it makes the tracking simpler)
                // Since we profiled this event in I above, that means we're going to have slightly imprecise statistics (will eventually be correct) TODO fix this maybe?
                status = allocateMSHR(event, true, 1);
            } else if (mshr_->pendingWriteback(addr) || (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                if (is_debug_event(event)) {
                    eventDI.action = "Drop";
                    eventDI.reason = "Req conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) { // Drop PutX, Ack it, forward request up
                MemEvent * put = static_cast<MemEvent*>(mshr_->swapFrontEvent(addr, event));
                sendWritebackAck(put);
                mshr_->setData(addr, put->getPayload(), put->getDirty());
                delete put;
                sendFwdRequest(event, Command::FetchInv, upperCacheName_, event->getSize(), 0, inMSHR);
            } else if (mshr_->exists(addr) && (CommandWriteback[(int)mshr_->getFrontEvent(addr)->getCmd()])) {
                MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                sendWritebackAck(put);
                sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                mshr_->removeFront(addr);
                delete put;
                cleanUpAfterRequest(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::FetchInv, upperCacheName_, event->getSize(), 0, inMSHR);
                }
            }
            break;
        case I_B:
            line->setState(I);
            if (is_debug_event(event)) {
                eventDI.action = "Drop";
                eventDI.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
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
            state2 = I;
            handle = true;
            break;
        case S_B:
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (line->getShared() || line->getOwned()) {
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::FetchInv, upperCacheName_, event->getSize(), 0, inMSHR);
                line->setTimestamp(sendTime);
                line->setState(state1);
            }
        } else {
            sendResponseDown(event, event->getSize(), line->getData(), state == M);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }
    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchInvX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!inMSHR)
        stat_eventState[(int)Command::FetchInvX][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInvX, false, addr, state);

    switch (state) {
        case I:
            if (inMSHR && mshr_->hasData(addr)) { // Replay
                sendResponseDown(event, event->getSize(), &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                mshr_->clearData(addr);
                cleanUpAfterRequest(event, inMSHR);
                break;
            } else if (mshr_->pendingWriteback(addr)) {
                if (is_debug_event(event)) {
                    eventDI.action = "Drop";
                    eventDI.reason = "MSHR conflict";
                }
                delete event;
                break;
            } else if (mshr_->exists(addr)) {
                if (mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLine || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                    if (is_debug_event(event)) {
                        eventDI.action = "Drop";
                        eventDI.reason = "MSHR conflict";
                    }
                    delete event;
                    break;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) {
                    MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendWritebackAck(put);
                    sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                    delete put;
                    mshr_->removeFront(addr);
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutE || mshr_->getFrontEvent(addr)->getCmd() == Command::PutM) {
                    MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                    put->setCmd(Command::PutS); // Make this a PutS so we only record the block in shared later
                    put->setDirty(false);
                    delete event;
                    break;
                }
            }
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                sendFwdRequest(event, Command::FetchInvX, upperCacheName_, event->getSize(), 0, inMSHR);
            }
            break;
        case E:
        case M:
            if (line->getOwned()) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    uint64_t sendTime = sendFwdRequest(event, Command::FetchInvX, upperCacheName_, lineSize_, line->getTimestamp(), inMSHR);
                    line->setTimestamp(sendTime);
                    mshr_->setInProgress(addr);
                    state == E ? line->setState(E_InvX) : line->setState(M_InvX);
                    status = MemEventStatus::Stall;
                }
                break;
            }
            sendResponseDown(event, event->getSize(), line->getData(), state == M);
            line->setState(S);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_B:
        case I_B:
            if (is_debug_event(event)) {
                eventDI.action = "Drop";
                eventDI.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}

bool MESIPrivNoninclusive::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetSResp, false, addr, state);

    stat_eventState[(int)Command::GetSResp][state]->addData(1);

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    if (is_debug_event(req))
        printData(&(event->getPayload()), true);

    uint64_t sendTime = sendResponseUp(req, &(event->getPayload()), true, line ? line->getTimestamp() : 0);

    // Update line
    if (line) {
        line->setData(event->getPayload(), 0);
        line->setState(S);
        line->setShared(true);
        line->setTimestamp(sendTime-1);
    }

    cleanUpAfterResponse(event, inMSHR);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetXResp, false, addr, state);

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - req->getBaseAddr();

    switch (state) {
        case I:
        {
            sendExclusiveResponse(req, &(event->getPayload()), true, 0, event->getDirty());
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case SM:
        {
            line->setState(M);
            line->setOwned(true);
            if (line->getShared())
                line->setShared(false);

            uint64_t sendTime = sendExclusiveResponse(req, line->getData(), true, line->getTimestamp(), event->getDirty());
            line->setTimestamp(sendTime-1);
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventState[(int)Command::GetXResp][state]->addData(1);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineResp, false, addr, state);

    stat_eventState[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            cacheArray_->deallocate(line);
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    cleanUpAfterResponse(event, inMSHR);

    return true;
}


// Response to a Fetch or FetchInv
bool MESIPrivNoninclusive::handleFetchResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    //printLine(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchResp, false, addr, state);

    stat_eventState[(int)Command::FetchResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);
    responses.erase(addr);

    if (state == I) { // Fetch or FetchInv
        MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
        sendResponseDown(req, event->getSize(), &(event->getPayload()), event->getDirty());
        cleanUpAfterResponse(event, inMSHR);
    } else {    // FetchInv only
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
        } else if (state == M_Inv) {
            line->setState(M);
        } else {
            line->setState(E);
        }
        if (line->getOwned())
            line->setOwned(false);
        else
            line->setShared(false);
        retry(addr);
        delete event;
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchXResp, false, addr, state);

    stat_eventState[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);
    responses.erase(addr);

    if (state == I) {
        MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
        sendResponseDown(req, event->getSize(), &(event->getPayload()), event->getDirty());
        cleanUpAfterResponse(event, inMSHR);
    } else {
        line->setOwned(false);
        line->setShared(true);
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
        } else if (state == M_InvX) {
            line->setState(M);
        } else {
            line->setState(E);
        }
        line->setOwned(false);
        line->setShared(true);
        retry(addr);
        delete event;
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleAckInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::AckInv, false, addr, state);

    if (line) {
        line->setShared(false);
        line->setOwned(false);
    }
    responses.erase(addr);

    mshr_->decrementAcksNeeded(addr);

    stat_eventState[(int)Command::AckInv][state]->addData(1);

    switch (state) {
        case I:
            {
            MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            if (mshr_->hasData(addr) && req->getCmd() == Command::FetchInv)
                sendResponseDown(req, req->getSize(), &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
            else
                sendResponseDown(req, req->getSize(), nullptr, false);
            cleanUpAfterResponse(event, inMSHR);
            return true;
            }
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

    delete event;

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleAckPut(MemEvent * event, bool inMSHR) {
    stat_eventState[(int)Command::AckPut][I]->addData(1);

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::AckPut, false, event->getBaseAddr(), I);
        eventDI.action = "Done";
    }
    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIPrivNoninclusive::handleNULLCMD(MemEvent* event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    PrivateCacheLine * line = cacheArray_->lookup(oldAddr, false);

    if (is_debug_addr(oldAddr))
        eventDI.prefill(event->getID(), Command::Evict, false, oldAddr, (line ? line->getState() : I));

    bool evicted = handleEviction(newAddr, line, eventDI);
    if (is_debug_addr(oldAddr) || is_debug_addr(newAddr) || is_debug_addr(line->getAddr())) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        if (mshr_->exists(newAddr) && mshr_->getStalledForEvict(newAddr)) {
            debug->debug(_L5_, "%s, Retry for 0x%" PRIx64 "\n", cachename_.c_str(), newAddr);
            retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
            mshr_->addPendingRetry(newAddr);
            mshr_->setStalledForEvict(newAddr, false);
        }
        if (mshr_->removeEvictPointer(oldAddr, newAddr))
            retry(oldAddr);
    } else { // Check if we're waiting for a new address
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (is_debug_addr(oldAddr) || is_debug_addr(line->getAddr()) || is_debug_addr(newAddr)) {
                stringstream reason;
                reason << "New target, old was 0x" << std::hex << oldAddr;
                eventDI.reason = reason.str();
                eventDI.action = "Stall";
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    if (is_debug_event(event) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    delete event;
    return true;
}


bool MESIPrivNoninclusive::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(event->getBaseAddr(), false);

    if (is_debug_event(event) || is_debug_event(nackedEvent)) {
        eventDI.prefill(event->getID(), Command::NACK, false, event->getBaseAddr(), (line ? line->getState() : I));
    }

    delete event;

    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLine:
        case Command::FlushLineInv:
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
        case Command::PutX:
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            /*if (is_debug_addr(addr)) {
                if (responses.find(addr) != responses.end()) {
                    debug->debug(_L3_, "\tResponse for 0x%" PRIx64 " is: %" PRIu64 "\n", addr, responses.find(addr)->second);
                } else {
                    debug->debug(_L3_, "\tResponse for 0x%" PRIx64 " is: None\n", addr);
                }
            }*/
            if ((responses.find(addr) != responses.end()) && (responses.find(addr)->second == nackedEvent->getID())) {
                //if (is_debug_addr(addr))
                //    debug->debug(_L5_, "\tRetry NACKed event\n");
                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (is_debug_event(nackedEvent)) {
                    eventDI.action = "Drop";
                    stringstream reason;
                    reason << "already satisfied, event: " << nackedEvent->getBriefString();
                    eventDI.reason = reason.str();
                }
                   // debug->debug(_L5_, "\tDrop NACKed event: %s\n", nackedEvent->getVerboseString().c_str());
                delete nackedEvent;
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received NACK with unhandled command type. Event: %s. NackedEvent: %s Time = %" PRIu64 "ns\n",
                    getName().c_str(), event->getVerboseString().c_str(), nackedEvent ? nackedEvent->getVerboseString().c_str() : "nullptr", getCurrentSimTimeNano());
    }
    return true;
}


/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

/**
 * Allocate a new cache line
 */
MemEventStatus MESIPrivNoninclusive::allocateLine(MemEvent * event, PrivateCacheLine* &line, bool inMSHR) {

    evictDI.prefill(event->getID(), Command::Evict, false, 0, I);

    bool evicted = handleEviction(event->getBaseAddr(), line, evictDI);

    if (is_debug_event(event) || is_debug_addr(line->getAddr())) {
        evictDI.newst = line->getState();
        evictDI.verboseline = line->getString();
        eventDI.action = evictDI.action;
        eventDI.reason = evictDI.reason;
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return MemEventStatus::OK;
    } else {
        if (inMSHR || mshr_->insertEvent(event->getBaseAddr(), event, -1, false, true) != -1) {
            mshr_->insertEviction(line->getAddr(), event->getBaseAddr()); // Since we're private we never start an eviction (e.g., send invs) so it's safe to ignore an attempted eviction
            if (inMSHR)
                mshr_->setStalledForEvict(event->getBaseAddr(), true);
            if (is_debug_event(event) || is_debug_addr(line->getAddr())) {
                stringstream reason;
                reason << "New evict target 0x" << std::hex << line->getAddr();
                evictDI.reason = reason.str();
                printDebugInfo(&evictDI);
            }
            return MemEventStatus::Stall;
        }
        if (is_debug_event(event) || is_debug_addr(line->getAddr()))
            printDebugInfo(&evictDI);
        return MemEventStatus::Reject;
    }
}


bool MESIPrivNoninclusive::handleEviction(Addr addr, PrivateCacheLine*& line, dbgin &diStruct) {
    if (!line)
        line = cacheArray_->findReplacementCandidate(addr);

    State state = line->getState();

    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
        diStruct.oldst = state;
        diStruct.addr = line->getAddr();
    }

    //if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
    //    debug->debug(_L5_, "    Evicting line (0x%" PRIx64 ", %s)\n", line->getAddr(), StateString[state]);

    stat_evict[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    switch (state) {
        case I:
            if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                diStruct.action = "None";
                diStruct.reason = "already idle";
            }
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!line->getShared() && !silentEvictClean_) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutS, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    mshr_->insertWriteback(line->getAddr(), false);
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                        diStruct.action = "Writeback";
                    }
                } else if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                    diStruct.action = "Drop";
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                return true;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutX, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    mshr_->insertWriteback(line->getAddr(), true);
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (!line->getOwned() && !silentEvictClean_) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutE, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    mshr_->insertWriteback(line->getAddr(), false);
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                } else if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                    diStruct.action = "Drop";
                }
                line->setState(I);
                return true;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutX, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    mshr_->insertWriteback(line->getAddr(), true);
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (!line->getOwned()) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutM, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    mshr_->insertWriteback(line->getAddr(), false);
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                        diStruct.action = "Writeback";
                    }
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                    diStruct.action = "Drop";

                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                return true;
            }
        case S_B:
        case S_Inv:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) { // This cache is not responsible for the block, drop it
                    line->setState(I);
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                        diStruct.action = "Drop";
                    }
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                } else if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                    diStruct.action = "Stall";
                }
                return false;
            }
        case M_Inv:
        case E_Inv:
        case M_InvX:
        case E_InvX:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getOwned()) { // This cache is not responsible for the block, drop it
                    line->setState(I);
                    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                        diStruct.action = "Drop";
                    }
                    if (is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                    return true;
                } else if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
                    diStruct.action = "Stall";
                return false;
            }
        default:
            if (is_debug_addr(addr) || is_debug_addr(line->getAddr()))
                diStruct.action = "Stall";
            return false;
    }

    return false;
}


void MESIPrivNoninclusive::cleanUpEvent(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (inMSHR) {
        mshr_->removeFront(addr);
    }

    delete event;
}


void MESIPrivNoninclusive::cleanUpAfterRequest(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    cleanUpEvent(event, inMSHR);

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && !mshr_->getStalledForEvict(addr) && mshr_->getAcksNeeded(addr) == 0) {
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


void MESIPrivNoninclusive::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    /* Clean up matching request from MSHR MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    }
    mshr_->removeFront(addr);

    // Clean up request and response
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


void MESIPrivNoninclusive::retry(Addr addr) {
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
 * Protocol helper functions
 ***********************************************************************************************************/

uint64_t MESIPrivNoninclusive::sendExclusiveResponse(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t time, bool dirty) {
    MemEvent * responseEvent = event->makeResponse();
    responseEvent->setCmd(Command::GetXResp);

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (is_debug_event(event)) {
            printData(data, false);
        }
        responseEvent->setDirty(dirty);
    }

    if (time < timestamp_) time = timestamp_;
    uint64_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);

    // Debugging
    if (is_debug_event(responseEvent))
        eventDI.action = "Respond";
//    if (is_debug_event(responseEvent)) {
//        debug->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliveryTime, timestamp_, responseEvent->getBriefString().c_str());
//    }

    return deliveryTime;
}

uint64_t MESIPrivNoninclusive::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, uint64_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

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
//        debug->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliveryTime, timestamp_, responseEvent->getBriefString().c_str());
    }

    return deliveryTime;
}

void MESIPrivNoninclusive::sendResponseDown(MemEvent * event, uint32_t size, vector<uint8_t>* data, bool dirty) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setDirty(dirty);
    }

    responseEvent->setSize(size);

    uint64_t deliverTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    Response resp = {responseEvent, deliverTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueue(resp);

    if (is_debug_event(responseEvent)) {
        eventDI.action = "Respond";
//        debug->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliverTime, timestamp_, responseEvent->getBriefString().c_str());
    }
}


uint64_t MESIPrivNoninclusive::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
    MemEvent * flush = new MemEvent(*event);

    flush->setSrc(cachename_);
    flush->setDst(getDestination(event->getBaseAddr()));

    uint64_t latency = tagLatency_;
    if (evict) {
        flush->setEvict(true);
        // TODO only send payload when needed
        flush->setPayload(*data);
        flush->setDirty(dirty);
        latency = accessLatency_;
    } else {
        flush->setPayload(0, nullptr);
    }
    bool payload = false;

    uint64_t baseTime = (time > timestamp_) ? time : timestamp_;
    uint64_t sendTime = baseTime + latency;
    Response resp = {flush, sendTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);

    if (is_debug_addr(event->getBaseAddr())) {
        eventDI.action = "Forward";
        //debug->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", sendTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str(), evict ? "with data" : "without data");
    }
    return sendTime;
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

uint64_t MESIPrivNoninclusive::sendWriteback(Addr addr, uint32_t size, Command cmd, std::vector<uint8_t>* data, bool dirty, uint64_t startTime) {
    MemEvent* writeback = new MemEvent(cachename_, addr, addr, cmd);
    writeback->setDst(getDestination(addr));
    writeback->setSize(size);

    uint64_t latency = tagLatency_;

    /* Writeback data */
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*data);
        writeback->setDirty(dirty);

        if (is_debug_addr(addr)) {
            printData(data, false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t sendTime = timestamp_ > startTime ? timestamp_ : startTime;
    sendTime += latency;
    Response resp = {writeback, sendTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);

    return sendTime;
    //if (is_debug_addr(addr))
        //debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", sendTime, CommandString[(int)cmd], ((cmd == Command::PutM || writebackCleanBlocks_) ? "" : "out"));
}


uint64_t MESIPrivNoninclusive::sendFwdRequest(MemEvent * event, Command cmd, std::string dst, uint32_t size, uint64_t startTime, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    MemEvent * req = new MemEvent(cachename_, addr, addr, cmd);
    req->copyMetadata(event);
    req->setDst(dst);
    req->setSize(size);

    mshr_->incrementAcksNeeded(addr);

    responses.insert(std::make_pair(addr, req->getID()));  // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK

    uint64_t baseTime = timestamp_ > startTime ? timestamp_ : startTime;
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {req, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    if (is_debug_addr(addr)) {
        eventDI.action = "Forward";
//        debug->debug(_L7_, "Sending %s: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n",
//                CommandString[(int)cmd], addr, dst.c_str(), deliveryTime);
    }
    return deliveryTime;
}

void MESIPrivNoninclusive::sendWritebackAck(MemEvent * event) {
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

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIPrivNoninclusive::addToOutgoingQueue(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueue(resp);
}

void MESIPrivNoninclusive::addToOutgoingQueueUp(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueueUp(resp);
}

/********************
 * Helper functions
 ********************/

MemEventInitCoherence* MESIPrivNoninclusive::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, true, true, lineSize_, true);
}

void MESIPrivNoninclusive::hasUpperLevelCacheName(std::string cachename) {
    upperCacheName_ = cachename;
}

void MESIPrivNoninclusive::printLine(Addr addr) {
    if (!is_debug_addr(addr)) return;
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());
}

void MESIPrivNoninclusive::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());

    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");*/
}

void MESIPrivNoninclusive::recordLatency(Command cmd, int type, uint64_t latency) {
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

