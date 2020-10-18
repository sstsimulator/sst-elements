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
#include "coherencemgr/MESI_Shared_Noninclusive.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * MESI or MSI NonInclusive Coherence Controller for shared caches
 * - Has an inclusive tag directory & noninclusive data array
 * - Unlike the private noninclusive protocol, this one supports prefetchers due to the inclusive tag directory
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/* Handle GetS (load/read) requests */
bool MESISharNoninclusive::handleGetS(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, true);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, true) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    Command respcmd;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetS, localPrefetch, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
        case IA:
            status = processDirectoryMiss(event, tag, inMSHR);

            if (status == MemEventStatus::OK) {
                recordLatencyType(event->getID(), LatType::MISS);
                tag = dirArray_->lookup(addr, false);

                if (localPrefetch) { // Also need a data line
                    if (processDataMiss(event, tag, data, true) != MemEventStatus::OK) {
                        tag->setState(IA); // Don't let anyone steal this line, mark that we're waiting for a data line allocation
                        break;
                    } // Status is now inaccurate but it won't be Reject which is the only thing we're checking for
                }

                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][state]->addData(1);
                    stat_miss[0][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }

                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                tag->setState(IS);
                tag->setTimestamp(sendTime);
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
                if (inMSHR) mshr_->setProfiled(addr);
            }

            if (is_debug_event(event))
                eventDI.reason = "hit";

            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                if (is_debug_event(event))
                    eventDI.action = "Done";
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(tag, statPrefetchHit);

            if (data || mshr_->hasData(addr)) {
                tag->addSharer(event->getSrc());
                if (mshr_->hasData(addr))
                    sendTime = sendResponseUp(event, &(mshr_->getData(addr)), inMSHR, tag->getTimestamp());
                else
                    sendTime = sendResponseUp(event, data->getData(), inMSHR, tag->getTimestamp());
                tag->setTimestamp(sendTime-1);
                recordLatencyType(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, inMSHR);
            } else {
                if (!inMSHR) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    recordLatencyType(event->getID(), LatType::INV);
                    sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                    tag->setState(S_D);
                    tag->setTimestamp(sendTime - 1);
                    if (is_debug_event(event))
                        eventDI.reason = "data miss";
                    mshr_->setInProgress(addr);
                }
            }
            break;
        case E:
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::GetS][state]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                if (inMSHR) mshr_->setProfiled(addr);
            }

            if (is_debug_event(event))
                eventDI.reason = "hit";

            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, inMSHR);
                break;
            }

            recordPrefetchResult(tag, statPrefetchHit);

            if (tag->hasOwner()) {
                if (!inMSHR) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    sendTime = sendFetch(Command::FetchInvX, event, tag->getOwner(), inMSHR, tag->getTimestamp());
                    state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                    tag->setTimestamp(sendTime - 1);
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setInProgress(addr);
                }
            } else if (data || mshr_->hasData(addr)) {
                recordLatencyType(event->getID(), LatType::HIT);
                if (tag->hasSharers()) {
                    respcmd = Command::GetSResp;
                    tag->addSharer(event->getSrc());
                } else {
                    respcmd = Command::GetXResp;
                    tag->setOwner(event->getSrc());
                }
                if (mshr_->hasData(addr))
                    sendTime = sendResponseUp(event, &(mshr_->getData(addr)), inMSHR, tag->getTimestamp(), respcmd);
                else
                    sendTime = sendResponseUp(event, data->getData(), inMSHR, tag->getTimestamp(), respcmd);
                tag->setTimestamp(sendTime - 1);
                cleanUpAfterRequest(event, inMSHR);
            } else {
                if (!inMSHR) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                    state == E ? tag->setState(E_D) : tag->setState(M_D);
                    tag->setTimestamp(sendTime - 1);
                    if (is_debug_event(event))
                        eventDI.reason = "data miss";
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setInProgress(addr);
                }
            }
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            break;
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject) {
        if (localPrefetch)
            return false;
        else
            sendNACK(event);
    }

    return true;
}


bool MESISharNoninclusive::handleGetX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, true);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, true) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    Command respcmd;

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), event->getCmd(), false, addr, state);

    switch (state) {
        case I:
            status = processDirectoryMiss(event, tag, inMSHR);

            if (status == MemEventStatus::OK) {
                recordLatencyType(event->getID(), LatType::MISS);
                tag = dirArray_->lookup(addr, false);

                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)event->getCmd()][I]->addData(1);
                    stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                tag->setState(IM);
                tag->setTimestamp(sendTime);
                mshr_->setInProgress(addr);
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case S:
            if (!lastLevel_) { // If last level, upgrade silently by falling through to next case
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_eventState[(int)event->getCmd()][S]->addData(1);
                        stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                        stat_misses->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                        mshr_->setProfiled(addr);
                    }
                    recordPrefetchResult(tag, statPrefetchUpgradeMiss);
                    recordLatencyType(event->getID(), LatType::UPGRADE);

                    sendTime = forwardMessage(event, lineSize_, 0, nullptr);

                    if (invalidateExceptRequestor(event, tag, inMSHR, data == nullptr)) {
                        tag->setState(SM_Inv);
                    } else {
                        tag->setState(SM);
                        tag->setTimestamp(sendTime);
                    }
                    mshr_->setInProgress(addr);

                    if (is_debug_event(event))
                        eventDI.reason = "miss";
                }
                break;
            }
        case E:
        case M:
            if (!tag->hasOtherSharers(event->getSrc()) && !tag->hasOwner()) {
                if (is_debug_event(event))
                    eventDI.reason = "hit";
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    stat_eventState[(int)event->getCmd()][state]->addData(1);
                    stat_hit[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_hits->addData(1);
                }
                tag->setOwner(event->getSrc());
                if (tag->isSharer(event->getSrc())) {
                    tag->removeSharer(event->getSrc());
                    sendTime = sendResponseUp(event, nullptr, inMSHR, tag->getTimestamp(), Command::GetXResp);
                } else if (mshr_->hasData(addr))
                    sendTime = sendResponseUp(event, &(mshr_->getData(addr)), inMSHR, tag->getTimestamp(), Command::GetXResp);
                else
                    sendTime = sendResponseUp(event, data->getData(), inMSHR, tag->getTimestamp(), Command::GetXResp);
                tag->setTimestamp(sendTime - 1);
                recordLatencyType(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, inMSHR);
                break;
            }
            if (!inMSHR)
                status = allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    stat_eventState[(int)event->getCmd()][state]->addData(1);
                    stat_hit[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_hits->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordLatencyType(event->getID(), LatType::INV);
                if (tag->hasOtherSharers(event->getSrc())) {
                    invalidateExceptRequestor(event, tag, inMSHR, !data && !tag->isSharer(event->getSrc()));
                } else {
                    invalidateOwner(event, tag, inMSHR, Command::FetchInv);
                }
                tag->setState(M_Inv);
                mshr_->setInProgress(addr);
            }
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            else if (is_debug_event(event))
                eventDI.action = "Stall";
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;

}


bool MESISharNoninclusive::handleGetSX(MemEvent * event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


/* Flush a line from the cache but retain a clean copy */
bool MESISharNoninclusive::handleFlushLine(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    // Always need an MSHR for a flush
    MemEventStatus status = MemEventStatus::OK;
    if (inMSHR) {
        if (mshr_->getFrontEvent(addr) != event) {
            if (is_debug_event(event))
                eventDI.action = "Stall";
            status = MemEventStatus::Stall;
        }
    } else {
        status = allocateMSHR(event, false);
    }

    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                // event, evict, *data, dirty, time)
                forwardFlush(event, false, nullptr, false, 0);
                mshr_->setInProgress(addr);
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                forwardFlush(event, false, nullptr, false, tag->getTimestamp());
                tag->setState(S_B);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    removeOwnerViaInv(event, tag, data, false);
                    tag->addSharer(event->getSrc());
                    event->setEvict(false); // Don't stall
                } else if (tag->hasOwner()) {
                    uint64_t sendTime = sendFetch(Command::FetchInvX, event, tag->getOwner(), inMSHR, tag->getTimestamp());
                    tag->setTimestamp(sendTime - 1);
                    state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                    break;
                }
                if (data)
                    forwardFlush(event, true, data->getData(), tag->getState() == M, tag->getTimestamp());
                else
                    forwardFlush(event, true, &(mshr_->getData(addr)), tag->getState() == M, tag->getTimestamp());
                tag->getState() == E ? tag->setState(E_B) : tag->setState(M_B);
                mshr_->setInProgress(addr);
            }
            break;
        case E_InvX:
        case M_InvX:
            if (event->getEvict()) {
                removeOwnerViaInv(event, tag, data, true);
                tag->addSharer(event->getSrc());

                mshr_->decrementAcksNeeded(addr);
                tag->setState(NextState[tag->getState()]);

                retry(addr);
                event->setEvict(false); // Don't replay the eviction part of this flush
            }
            break;
        case E_Inv:
        case M_Inv:
            if (event->getEvict()) {
                removeOwnerViaInv(event, tag, data, false);
                tag->addSharer(event->getSrc());
                event->setEvict(false);
            }
            break;
        default:
            break;
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


/* Flush a line from cache & invalidate it */
bool MESISharNoninclusive::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    vector<uint8_t>* datavec = nullptr;
    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, false, nullptr, false, 0);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    removeSharerViaInv(event, tag, data, false);
                    event->setEvict(false); // Don't replay eviction
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->setState(S_Inv);
                    break;
                }

                if (data)
                    forwardFlush(event, true, data->getData(), false, tag->getTimestamp());
                else
                    forwardFlush(event, true, &(mshr_->getData(addr)), false, tag->getTimestamp());
                mshr_->setInProgress(addr);
                tag->setState(I_B);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FlushLineInv][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    if (tag->hasOwner())
                        removeOwnerViaInv(event, tag, data, false);
                    else
                        removeSharerViaInv(event, tag, data, false);
                    event->setEvict(false);
                }

                if (tag->hasOwner()) {
                    invalidateOwner(event, tag, inMSHR, Command::FetchInv);
                    tag->getState() == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->getState() == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else {
                    if (data)
                        forwardFlush(event, true, data->getData(), tag->getState() == M, tag->getTimestamp());
                    else
                        forwardFlush(event, true, &(mshr_->getData(addr)), tag->getState() == M, tag->getTimestamp());
                    mshr_->setInProgress(addr);
                    tag->setState(I_B);
                }
            }
            break;
        case SM_Inv:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case E_InvX:
        case M_InvX:
            if (event->getEvict()) {
                if (tag->hasOwner())
                    removeOwnerViaInv(event, tag, data, true);
                else
                    removeSharerViaInv(event, tag, data, true);
                event->setEvict(false);

                if (mshr_->decrementAcksNeeded(addr)) {
                    tag->setState(NextState[tag->getState()]);
                    retry(addr);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
        case SM_D:
        case SB_D:
            if (event->getEvict()) {
                if (*(tag->getSharers()->begin()) == event->getSrc()) {
                    removeSharerViaInv(event, tag, data, true);
                    mshr_->decrementAcksNeeded(addr);
                    tag->setState(NextState[tag->getState()]);
                    retry(addr);
                } else
                    removeSharerViaInv(event, tag, data, false);
                event->setEvict(false);
            }
            break;
        default:
            break;
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handlePutS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutS, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEventBase* entry;

    switch (state) {
        case S:
        case E:
        case M:
            if (!data && tag->numSharers() == 1) { // Need to allocate line
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false, -1);
                if (status != MemEventStatus::OK) {
                    break;
                }
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_eventState[(int)Command::PutS][I]->addData(1);
                        mshr_->setProfiled(addr);
                    }
                    if (state == S) tag->setState(SA);
                    else if (state == E) tag->setState(EA);
                    else tag->setState(MA);
                    break;
                }
                data = dataArray_->lookup(addr, true);
                data->setData(event->getPayload(), 0);
                inMSHR = true;
            }
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutS][I]->addData(1);
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
            removeSharerViaInv(event, tag, data, true);
            if (mshr_->decrementAcksNeeded(addr))
                tag->setState(NextState[state]);
            sendWritebackAck(event);
            if (inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutS][state]->addData(1);
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_B:
        case E_B:
        case M_B:
            if (!data && tag->numSharers() == 1) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false, 1);   // Put just after the Flush, will handle next
                break;
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            if (inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutS][state]->addData(1);
            }
            cleanUpEvent(event, inMSHR);
            break;
        case S_D:
        case E_D:
        case M_D:
        case SB_D:
            if (event->getSrc() == *(tag->getSharers()->begin())) { // Sent fetch to this requestor
                // Retry the pending fetch
                mshr_->decrementAcksNeeded(addr);
                mshr_->setData(addr, event->getPayload());
                responses.find(addr)->second.erase(event->getSrc());
                if (responses.find(addr)->second.empty())
                    responses.erase(addr);
                tag->setState(NextState[state]);
                retry(addr);

                // Handle PutS now if we can, later if not
                if (tag->numSharers() > 1) {
                    tag->removeSharer(event->getSrc());
                    sendWritebackAck(event);
                    if (inMSHR || !mshr_->getProfiled(addr)) {
                        stat_eventState[(int)Command::PutS][state]->addData(1);
                    }
                    cleanUpEvent(event, inMSHR);
                } else {
                    if (inMSHR)
                        mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                    entry = mshr_->getEntryEvent(addr, 1);
                    if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                        status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                    else
                        status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement
                }
                break;
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            if (inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutS][state]->addData(1);
            }
            cleanUpEvent(event, inMSHR);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutE, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (!data) { // Need to allocate line
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false, -1);
                if (status != MemEventStatus::OK)
                    break;
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    if (!inMSHR || !mshr_->getProfiled(addr)) {
                        stat_eventState[(int)Command::PutE][state]->addData(1);
                        mshr_->setProfiled(addr);
                    }
                    state == E ? tag->setState(EA) : tag->setState(MA);
                    break;
                }
                data = dataArray_->lookup(addr, true);
                data->setData(event->getPayload(), 0);
                inMSHR = true;
            }
            tag->removeOwner();
            sendWritebackAck(event);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutE][state]->addData(1);
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_InvX:
        case M_InvX:
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            if (!data && !mshr_->hasData(addr))
                mshr_->setData(addr, event->getPayload());
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty())
                responses.erase(addr);
            tag->setState(NextState[state]);
            // Handle PutE now if possible, later if not
            if (data) {
                sendWritebackAck(event);
                cleanUpEvent(event, inMSHR);
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::PutE][state]->addData(1);
                }
            } else {
                tag->addSharer(event->getSrc());
                event->setCmd(Command::PutS);
                if (inMSHR)
                    mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                MemEventBase* entry = mshr_->getEntryEvent(addr, 1);
                if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                    status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                else
                    status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement
                retry(addr);
            }
            break;
        case E_Inv:
        case M_Inv:
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            if (!data && !mshr_->hasData(addr))
                mshr_->setData(addr, event->getPayload());
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty())
                responses.erase(addr);
            sendWritebackAck(event);
            tag->setState(NextState[state]);
            cleanUpAfterRequest(event, inMSHR);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutE][state]->addData(1);
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }
    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutM, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (!data) { // Need to allocate line
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false, -1);
                if (status != MemEventStatus::OK)
                    break;
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::PutM][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    tag->setState(MA);
                    break;
                }
                data = dataArray_->lookup(addr, true);
                data->setData(event->getPayload(), 0);
                inMSHR = true;
            } else if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutM][state]->addData(1);
            }
            if (is_debug_event(event))
                eventDI.reason = "hit";

            tag->removeOwner();
            tag->setState(M);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_InvX:
        case M_InvX:
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty())
                responses.erase(addr);
            tag->setState(M);

            // Handle PutM now if possible, later if not
            if (data) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::PutM][state]->addData(1);
                }
                data->setData(event->getPayload(), 0);
                sendWritebackAck(event);
                cleanUpEvent(event, inMSHR);
            } else {
                tag->addSharer(event->getSrc());
                event->setCmd(Command::PutS);
                mshr_->setData(addr, event->getPayload());
                if (inMSHR)
                    mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                MemEventBase* entry = mshr_->getEntryEvent(addr, 1);
                if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                    status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                else
                    status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement

                retry(addr);
            }
            break;
        case E_Inv:
        case M_Inv:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::PutM][state]->addData(1);
            }
            // Handle the coherence state part and buffer the data in the MSHR, we won't need a line because we're either losing the data or one of our children wants it
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            if (!data && !mshr_->hasData(addr))
                mshr_->setData(addr, event->getPayload());
            responses.find(addr)->second.erase(event->getSrc());
            if (responses.find(addr)->second.empty())
                responses.erase(addr);
            sendWritebackAck(event);
            tag->setState(M);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }
    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handlePutX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::PutX, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    tag->removeOwner();
    tag->addSharer(event->getSrc());

    sendWritebackAck(event);

    if (!inMSHR || !mshr_->getProfiled(addr)) {
        stat_eventState[(int)Command::PutX][state]->addData(1);
    }

    switch (state) {
        case E:
        case M:
            if (event->getDirty())
                tag->setState(M);

            if (data)
                data->setData(event->getPayload(), 0);

            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_InvX:
        case M_InvX:
            if (event->getDirty() || state == M_InvX)
                tag->setState(M);
            else
                tag->setState(E);

            if (data)
                data->setData(event->getPayload(), 0);
            else
                mshr_->setData(addr, event->getPayload());

            mshr_->decrementAcksNeeded(addr);

            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case M_Inv:
            if (event->getDirty())
                tag->setState(M_Inv);

            if (data)
                data->setData(event->getPayload(), 0);
            else
                mshr_->setData(addr, event->getPayload());

            cleanUpEvent(event, inMSHR);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    return true;

}

bool MESISharNoninclusive::handleFetch(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Fetch, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    uint64_t sendTime;
    MemEvent* put;
    switch (state) {
        case I: // Happens if we replaced and are waiting for a writeback ack or we are replaying the event after an eviction -> drop the fetch
        case I_B: // Happens if we sent a FlushLineInv and it raced with a Fetch
        case E_B: // Happens if we sent a FlushLine and it raced with Fetch
        case M_B: // Happens if we sent a FlushLine and it raced with Fetch
            stat_eventState[(int)Command::Fetch][state]->addData(1);
            delete event;
            break;
        case S:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, inMSHR);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(S_D);
                    if (!applyPendingReplacement(addr))
                        sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                }
            }
            break;
        case SA:
            //Look for a PutS in the MSHR
            put = static_cast<MemEvent*>(mshr_->getFirstEventEntry(addr, Command::PutS));
            sendResponseDown(event, &(put->getPayload()), false, false);
            stat_eventState[(int)Command::Fetch][state]->addData(1);
            cleanUpEvent(event, inMSHR);
            break;
        case SM:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, inMSHR);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(SM_D);
                    sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                }
            }
            break;
        case S_D:
        case S_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case S_B:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, inMSHR);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(SB_D);
                    sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                }
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::Inv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I_B:
            dirArray_->deallocate(tag);
            if (data)
                dataArray_->deallocate(data);
        case I:
            stat_eventState[(int)Command::Inv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, statPrefetchInv);
                    stat_eventState[(int)Command::Inv][state]->addData(1);
                    mshr_->setProfiled(addr);
                }

                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, inMSHR, false, Command::Inv);
                    tag->setState(S_Inv);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dirArray_->deallocate(tag);
                    if (data)
                        dataArray_->deallocate(data);
                    cleanUpAfterRequest(event, inMSHR);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case SA: // Merge with PutS and drop
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, nullptr, false, true);
            dirArray_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::Inv][state]->addData(1);
            }
            cleanUpEvent(event, inMSHR);
            cleanUpAfterRequest(put, true);
            break;
        case S_D:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case S_Inv: // Evict
            if (!inMSHR) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::Inv][state]->addData(1);
                }
            }
            break;
        case SM:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::Inv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, false, Command::Inv);
                    tag->setState(SM_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    tag->setState(IM);
                    cleanUpAfterRequest(event, inMSHR);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case SM_Inv:
            if (!inMSHR) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::Inv][state]->addData(1);
                }
            }
            break;
        case S_B:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::Inv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, false, Command::Inv);
                    tag->setState(SB_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dirArray_->deallocate(tag);
                    if (data)
                        dataArray_->deallocate(data);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleForceInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::ForceInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I_B:
            dirArray_->deallocate(tag);
            if (data)
                dataArray_->deallocate(data);
        case I:
            stat_eventState[(int)Command::ForceInv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, statPrefetchInv);
                    stat_eventState[(int)Command::ForceInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, inMSHR, false, Command::ForceInv);
                    tag->setState(S_Inv);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dirArray_->deallocate(tag);
                    if (data) {
                        dataArray_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, inMSHR);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case E:
        case M:
            if ((tag->hasSharers() || tag->hasOwner()) && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::ForceInv][state]->addData(1);
                    recordPrefetchResult(tag, statPrefetchInv);
                }
                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, inMSHR, false, Command::ForceInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                    mshr_->setProfiled(addr);
                } else if (tag->hasOwner()) {
                    if (!applyPendingReplacement(addr))
                        invalidateOwner(event, tag, inMSHR, Command::ForceInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dirArray_->deallocate(tag);
                    if (data) {
                        dataArray_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, inMSHR);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            if ((tag->hasSharers() || tag->hasOwner()) && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr))  {
                    stat_eventState[(int)Command::ForceInv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, false, Command::ForceInv);
                    tag->setState(SB_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dirArray_->deallocate(tag);
                    if (data)
                        dataArray_->deallocate(data);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
        case E_InvX:
        case M_InvX:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case S_Inv:     // Evict, FlushLineInv
        case E_Inv:     // Evict, FlushLineInv
        case M_Inv:     // Evict, FlushlIneInv, Internal GetX
            if (!inMSHR) {
                if (mshr_->getFrontType(addr) == MSHREntryType::Evict || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                    status = allocateMSHR(event, true, 0);
                    if (status == MemEventStatus::OK) {
                        mshr_->setProfiled(addr);
                        stat_eventState[(int)Command::ForceInv][state]->addData(1);
                    }
                } else { // In a race with GetX/GetSX, let the other event complete first since it always can and this will avoid repeatedly losing the block before the Get* can complete
                    status = allocateMSHR(event, true, 1);
                }
            }
            break;
        case SM:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::ForceInv][state]->addData(1);
            }
            if (!tag->hasSharers()) {
                sendResponseDown(event, nullptr, false, true);
                tag->setState(IM);
                cleanUpEvent(event, inMSHR);
            } else {
                if (!inMSHR)
                    status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    invalidateSharers(event, tag, inMSHR, false, Command::Inv);
                    tag->setState(SM_Inv);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case SM_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                stat_eventState[(int)Command::ForceInv][state]->addData(1);
                mshr_->setProfiled(addr);
            }
            break;
        case SA:
        case EA:
        case MA:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::ForceInv][state]->addData(1);
            }
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, nullptr, state == MA, true);
            dirArray_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpEvent(event, inMSHR);
            cleanUpAfterRequest(put, true);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleFetchInv(MemEvent * event, bool inMSHR){
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInv, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I:
            stat_eventState[(int)Command::FetchInv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, statPrefetchInv);
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    tag->setState(S_Inv);
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, inMSHR, !(data || mshr_->hasData(addr)), Command::Inv);
                } else {
                    if (data)
                        sendResponseDown(event, data->getData(), false, true);
                    else {
                        sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                        mshr_->clearData(addr);
                    }
                    dirArray_->deallocate(tag);
                    if (data) {
                        dataArray_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            if ((tag->hasSharers() || tag->hasOwner())&& !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasOwner() || tag->hasSharers()) mshr_->setProfiled(addr);
                    recordPrefetchResult(tag, statPrefetchInv);
                }
                if (applyPendingReplacement(addr)) {
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, !data, Command::Inv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasOwner()) {
                    invalidateOwner(event, tag, inMSHR, Command::FetchInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else {
                    if (data)
                        sendResponseDown(event, data->getData(), state == M, true);
                    else {
                        sendResponseDown(event, &(mshr_->getData(addr)), state == M, true);
                        mshr_->clearData(addr);
                    }
                    dirArray_->deallocate(tag);
                    if (data) {
                        dataArray_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            if (tag->hasSharers() && !inMSHR)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, !data, Command::Inv);
                    tag->setState(SB_Inv);
                } else {
                    if (data) {
                        sendResponseDown(event, data->getData(), false, true);
                        dataArray_->deallocate(data);
                    } else {
                        sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                        mshr_->clearData(addr);
                    }
                    dirArray_->deallocate(tag);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        case SA:
        case EA:
        case MA:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::FetchInv][state]->addData(1);
            }
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, &(put->getPayload()), state == MA, true);
            dirArray_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpEvent(event, inMSHR);
            cleanUpAfterRequest(put, true);
            break;
        case SM:
            if (!tag->hasSharers()) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                }
                tag->setState(IM);
                if (data)
                    sendResponseDown(event, data->getData(), false, true);
                else
                    sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                tag->setState(IM);
                if (mshr_->hasData(addr))
                    mshr_->clearData(addr);
                cleanUpEvent(event, inMSHR);
                break;
            }
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                if (!inMSHR || !mshr_->getProfiled(addr)) {
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, inMSHR, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->setState(SM_Inv);
                    break;
                }
            }
            break;
        case SM_Inv:
            if (!inMSHR) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                }
            }
            break;
        case S_Inv:
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_eventState[(int)Command::FetchInv][state]->addData(1);
                }
            } else if (!inMSHR) {
                status = allocateMSHR(event, true, 1);
            }
            break;
        case E_InvX:
        case M_InvX:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleFetchInvX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchInvX, false, addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    uint64_t sendTime;

    MemEvent * req;

    switch (state) {
        case I_B:
            dirArray_->deallocate(tag);
            if (data) dataArray_->deallocate(data);
        case I:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            }
            if (inMSHR) {
                cleanUpAfterRequest(event, inMSHR);
            } else {
                delete event;
            }
            break;
        case E_B:
        case M_B:
            tag->setState(S_B);
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            }
            delete event;
            break;
        case E:
        case M:
            if ((tag->hasOwner()|| (!data && !mshr_->hasData(addr))) && !inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status != MemEventStatus::OK)
                break;
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            }
            if (tag->hasOwner()) { // Get data from owner
                if (!applyPendingReplacement(addr)) {
                    sendTime = sendFetch(Command::FetchInvX, event, tag->getOwner(), inMSHR, tag->getTimestamp());
                    tag->setTimestamp(sendTime-1);
                }
                state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                mshr_->setProfiled(addr);
            } else if (!data && !mshr_->hasData(addr)) {
                if (!applyPendingReplacement(addr)) {
                    sendTime = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), inMSHR, tag->getTimestamp());
                    tag->setTimestamp(sendTime-1);
                }
                state == E ? tag->setState(E_D) : tag->setState(M_D);
                mshr_->setProfiled(addr);
            } else {
                tag->setState(S);
                if (data)
                    sendResponseDown(event, data->getData(), state == M, true); // TODO Double check that a downgrade counts as an evict
                else {
                    sendResponseDown(event, &(mshr_->getData(addr)), state == M, true);
                }
                cleanUpAfterRequest(event, inMSHR);
            }
            break;
        case EA:
        case MA:
            if (!inMSHR || mshr_->getProfiled(addr)) {
                stat_eventState[(int)Command::FetchInvX][state]->addData(1);
            }
            req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendResponseDown(event, &(req->getPayload()), state == M, true); // TODO Double check that a downgrade counts as an evict
            // Clean up so that when we replay the replacement we get the right downgraded state
            req->setCmd(Command::PutS);
            tag->removeOwner();
            tag->addSharer(req->getSrc());
            tag->setState(SA);
            delete event;
            break;
        case E_InvX:
        case M_InvX:
        case E_D:
        case M_D:
        case E_Inv:
        case M_Inv:
            if (!inMSHR)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetSResp, localPrefetch, addr, state);

    stat_eventState[(int)Command::GetSResp][state]->addData(1);

    tag->setState(S);
    if (data)
        data->setData(event->getPayload(), 0);

    if (localPrefetch) {
        tag->setPrefetch(true);
        if (is_debug_event(event))
            eventDI.action = "Done";
    } else {
        tag->addSharer(req->getSrc());
        uint64_t sendTime = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetSResp);
        tag->setTimestamp(sendTime-1);
    }

    cleanUpAfterResponse(event, inMSHR);

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }

    return true;
}


bool MESISharNoninclusive::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));

    bool localPrefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetXResp, localPrefetch, addr, state);

    if (data)
        data->setData(event->getPayload(), 0);

    stat_eventState[(int)Command::GetXResp][state]->addData(1);

    switch (state) {
        case IS:
        {
            // Update state
            if (event->getDirty())
                tag->setState(M);
            else
                tag->setState(protocolState_); // E for MESI, S for MSI

            if (localPrefetch) {
                tag->setPrefetch(true);
                if (is_debug_event(event))
                    eventDI.action = "Done";
            } else {
                if (tag->getState() == S || !protocol_ || mshr_->getSize(addr) > 1) {
                    tag->addSharer(req->getSrc());
                    uint64_t sendTime = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetSResp);
                    tag->setTimestamp(sendTime - 1);
                } else {
                    tag->setOwner(req->getSrc());
                    uint64_t sendTime = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetXResp);
                    tag->setTimestamp(sendTime - 1);
                }
            }

            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case IM:
        case SM:
        {
            tag->setState(M);
            tag->setOwner(req->getSrc());
            uint64_t sendTime = 0;
            if (tag->isSharer(req->getSrc())) {
                tag->removeSharer(req->getSrc());
                sendTime = sendResponseUp(req, nullptr, true, tag->getTimestamp(), Command::GetXResp);
            } else if (event->getPayloadSize() != 0) {
                sendTime = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetXResp);
            } else {
                sendTime = sendResponseUp(req, &(mshr_->getData(addr)), true, tag->getTimestamp(), Command::GetXResp);
            }
            tag->setTimestamp(sendTime - 1);

            if (mshr_->hasData(addr))
                mshr_->clearData(addr);

            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case SM_Inv:
            tag->setState(M_Inv);
            mshr_->setInProgress(addr, false);
            if (!data && event->getPayloadSize() != 0)
                mshr_->setData(addr, event->getPayload());
            if (is_debug_event(event)) {
                eventDI.action = "Stall";
                eventDI.reason = "Acks needed";
            }
            cleanUpEvent(event, inMSHR);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }
    return true;
}


bool MESISharNoninclusive::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLineResp, false, addr, state);

    stat_eventState[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));

    switch (state) {
        case I:
            break;
        case I_B:
            dirArray_->deallocate(tag);
            if (data)  {
                dataArray_->deallocate(data);
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            tag->setState(S);
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }
    cleanUpAfterResponse(event, inMSHR);
    return true;
}


bool MESISharNoninclusive::handleFetchResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FetchResp, false, addr, state);

    // Check acks needed
    bool done = mshr_->decrementAcksNeeded(addr);

    // Remove response from expected response list & extract payload
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty())
        responses.erase(addr);

    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload());

    stat_eventState[(int)Command::FetchResp][state]->addData(1);

    switch (state) {
        case S_D:
        case E_D:
        case M_D:
        case SM_D:
        case SB_D:
            tag->setState(NextState[state]); // S or E or M or SM or S_B
            retry(addr);
            break;
        case S_Inv:
        case SB_Inv:
            tag->removeSharer(event->getSrc());
            if (done) {
                tag->setState(S);
                retry(addr);
            }
            break;
        case SM_Inv:
            tag->removeSharer(event->getSrc());
            if (done) {
                tag->setState(SM);
                if (!mshr_->getInProgress(addr))
                    retry(addr);
            }
            break;
        case E_InvX:
        case M_InvX:
            tag->removeOwner();
            tag->addSharer(event->getSrc());
            tag->setState(NextState[state]); // E or M
            retry(addr);
            break;
        case E_Inv:
        case M_Inv:
            if (tag->hasOwner())
                tag->removeOwner();
            else
                tag->removeSharer(event->getSrc());
            if (done) {
                tag->setState(NextState[state]);    // E or M
                retry(addr);
            }
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_event(event)) {
        eventDI.action = done ? "Retry" : "Stall";
        if (tag) {
            eventDI.newst = tag->getState();
            eventDI.verboseline = tag->getString();
            if (data)
                eventDI.verboseline += "/" + data->getString();
        }
    }

    delete event;

    return true;
}


bool MESISharNoninclusive::handleFetchXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::FetchXResp, false, addr, state);
        eventDI.action = "Retry";
    }

    stat_eventState[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);

    // Clear expected responses
    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    // Update coherence state
    tag->removeOwner();
    tag->addSharer(event->getSrc());

    if (state == M_InvX || event->getDirty())
        tag->setState(M);
    else
        tag->setState(E);

    // Save data
    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload());

    // Clean up and retry
    retry(addr);
    delete event;

    if (is_debug_addr(addr) && tag) {
        eventDI.newst = tag->getState();
        eventDI.verboseline = tag->getString();
        if (data)
            eventDI.verboseline += "/" + data->getString();
    }
    return true;
}


bool MESISharNoninclusive::handleAckInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getAddr();
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? dataArray_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::AckInv, false, addr, state);

    stat_eventState[(int)Command::AckInv][state]->addData(1);

    if (tag->isSharer(event->getSrc()))
        tag->removeSharer(event->getSrc());
    else
        tag->removeOwner();

    responses.find(addr)->second.erase(event->getSrc());
    if (responses.find(addr)->second.empty()) responses.erase(addr);

    bool done = mshr_->decrementAcksNeeded(addr);

    if (done) {
        tag->setState(NextState[state]);
        retry(addr);
    }

    if (is_debug_addr(addr)) {
        eventDI.action = done ? "Retry" : "DecAcks";
        if (tag) {
            eventDI.newst = tag->getState();
            eventDI.verboseline = tag->getString();
            if (data)
                eventDI.verboseline += "/" + data->getString();
        }
    }

    delete event;
    return true;
}


bool MESISharNoninclusive::handleAckPut(MemEvent * event, bool inMSHR) {
    DirectoryLine * tag = dirArray_->lookup(event->getBaseAddr(), false);
    State state = tag ? tag->getState() : I;
    stat_eventState[(int)Command::AckPut][state]->addData(1);
    if (is_debug_event(event)) {
        eventDI.prefill(event->getID(), Command::AckPut, false, event->getBaseAddr(), state);
        eventDI.action = "Done";
        if (tag)
            eventDI.verboseline = tag->getString();
    }

    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESISharNoninclusive::handleNULLCMD(MemEvent* event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    // Is this a data eviction or directory eviction?
    bool dirEvict = evictionType_.find(std::make_pair(oldAddr,newAddr))->second;
    bool evicted;

    // Tag/directory array eviction
    if (dirEvict) {
        DirectoryLine * tag = dirArray_->lookup(oldAddr, false);
        if (is_debug_event(event)) {
            eventDI.prefill(event->getID(), Command::NULLCMD, false, oldAddr, evictDI.oldst);
        }

        evicted = handleDirEviction(newAddr, tag);
        if (evicted) {
            notifyListenerOfEvict(tag->getAddr(), lineSize_, event->getInstructionPointer());
            retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
            mshr_->addPendingRetry(newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
            evictionType_.erase(std::make_pair(oldAddr, newAddr));
            if (is_debug_event(event))
                eventDI.action = "Retry";
        } else {
            if (is_debug_event(event)) {
                eventDI.action = "Stall";
                eventDI.reason = "Dir evict failed";
            }
            if (oldAddr != tag->getAddr()) { // Now evicting a different address than we were before
                if (is_debug_event(event)) {
                    std::stringstream reason;
                    reason << "New dir evict targ: 0x" << std::hex << tag->getAddr();
                    eventDI.reason = reason.str();
                }
                if (mshr_->removeEvictPointer(oldAddr, newAddr))
                    retry(oldAddr);
                evictionType_.erase(std::make_pair(oldAddr, newAddr));
                std::pair<Addr,Addr> evictpair = std::make_pair(tag->getAddr(),newAddr);
                if (evictionType_.find(evictpair) == evictionType_.end()) {
                    mshr_->insertEviction(tag->getAddr(), newAddr);
                    evictionType_.insert(std::make_pair(evictpair, true));
                } else {
                    evictionType_[evictpair] = true;
                }
            }
        }
    // Data array eviction
    // Races can mean that this eviction is no longer neccessary but we can't tell if we haven't started the eviction
    // or if we're finishing. So finish it, but only replace if we currently need an eviction
    // (the event waiting for eviction now and the one that triggered this eviction might not be the same)
    } else {
        DirectoryLine * tag = dirArray_->lookup(newAddr, false);
        DataLine * data = dataArray_->lookup(oldAddr, false);
        evicted = handleDataEviction(newAddr, data);
        if (is_debug_event(event)) {
            eventDI.prefill(event->getID(), Command::NULLCMD, false, data->getAddr(), evictDI.oldst);
        }
        if (evicted) {
            if (tag && (tag->getState() == IA || tag->getState() == SA || tag->getState() == EA || tag->getState() == MA)) {
                dataArray_->replace(newAddr, data);
                if (is_debug_addr(newAddr))
                    printDebugAlloc(true, newAddr, "Data");

                data->setTag(tag);
                if (tag->getState() == IA)
                    tag->setState(I);
                else if (tag->getState() == SA)
                    tag->setState(S);
                else if (tag->getState() == EA)
                    tag->setState(E);
                else if (tag->getState() == MA)
                    tag->setState(M);
                retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
                mshr_->addPendingRetry(newAddr);
            } else {
                dataArray_->deallocate(data);
            }

            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
            evictionType_.erase(std::make_pair(oldAddr, newAddr));

            if (is_debug_event(event)) {
                eventDI.action = "Retry";
                eventDI.newst = data->getState();
            }
        } else {
            if (is_debug_event(event)) {
                eventDI.action = "Stall";
                eventDI.reason = "Data evict failed";
            }
            if (oldAddr != data->getAddr()) { // Now evicting a different address than we were before
                if (is_debug_event(event) || is_debug_addr(data->getAddr())) {
                    std::stringstream reason;
                    reason << "New data evict targ: 0x" << std::hex << data->getAddr();
                    eventDI.reason = reason.str();
                }

                if (mshr_->removeEvictPointer(oldAddr, newAddr))
                    retry(oldAddr);
                evictionType_.erase(std::make_pair(oldAddr, newAddr));
                std::pair<Addr,Addr> evictpair = std::make_pair(data->getAddr(), newAddr);
                if (evictionType_.find(evictpair) == evictionType_.end()) {
                    mshr_->insertEviction(data->getAddr(), newAddr);
                    evictionType_.insert(std::make_pair(evictpair, false));
                } else {
                    evictionType_[evictpair] = false;
                }
            }
        }
    }

    delete event;
    return true;
}


bool MESISharNoninclusive::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();

    if (is_debug_event(event) || is_debug_event(nackedEvent)) {
        DirectoryLine * tag = dirArray_->lookup(addr, false);
        eventDI.prefill(event->getID(), Command::NACK, false, addr, tag ? tag->getState() : I);
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
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Fetch:
        case Command::Inv:
        case Command::ForceInv:
            if (is_debug_addr(addr)) {
            }
            if (responses.find(addr) != responses.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst()) != responses.find(addr)->second.end()
                    && responses.find(addr)->second.find(nackedEvent->getDst())->second == nackedEvent->getID()) {

                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (is_debug_event(nackedEvent))
                    eventDI.action = "Drop";
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
 * MSHR & CacheArray management
 ***********************************************************************************************************/

/**
 * Handle a cache miss
 */
MemEventStatus MESISharNoninclusive::processDirectoryMiss(MemEvent * event, DirectoryLine * tag, bool inMSHR) {
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (inMSHR && mshr_->getFrontEvent(event->getBaseAddr()) != event) {
        if (is_debug_event(event))
            eventDI.action = "Stall";
        return MemEventStatus::Stall;
    }
    if (status == MemEventStatus::OK && !tag) { // Need a cache line too
        tag = allocateDirLine(event, tag);
        status = tag ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}

MemEventStatus MESISharNoninclusive::processDataMiss(MemEvent * event, DirectoryLine * tag, DataLine * data, bool inMSHR) {
    // Evict a data line, if that requires evicting a dirline, evict a dirline too
    bool evicted = handleDataEviction(event->getBaseAddr(), data);

    if (evicted) {
        if (is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "Data");
        dataArray_->replace(event->getBaseAddr(), data);
        data->setTag(tag);
        return MemEventStatus::OK;
    } else {
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            std::stringstream reason;
            reason << "evict data 0x" << std::hex << tag->getAddr();
            eventDI.reason = reason.str();
        }
        std::pair<Addr,Addr> evictpair = std::make_pair(data->getAddr(), event->getBaseAddr());
        if (evictionType_.find(evictpair) == evictionType_.end()) {
            mshr_->insertEviction(data->getAddr(), event->getBaseAddr());
            evictionType_.insert(std::make_pair(evictpair, false));
        } else {
            evictionType_[evictpair] = false;
        }
        return MemEventStatus::Stall;
    }
}


/**
 * Allocate a new directory line
 */
DirectoryLine* MESISharNoninclusive::allocateDirLine(MemEvent * event, DirectoryLine * tag) {
    bool evicted = handleDirEviction(event->getBaseAddr(), tag);
    if (evicted) {
        notifyListenerOfEvict(tag->getAddr(), lineSize_, event->getInstructionPointer());
        dirArray_->replace(event->getBaseAddr(), tag);
        if (is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "Dir");
        return tag;
    } else {
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            stringstream reason;
            reason << "evict dir 0x" << std::hex << tag->getAddr();
            eventDI.reason = reason.str();
        }

        std::pair<Addr,Addr> evictpair = std::make_pair(tag->getAddr(), event->getBaseAddr());
        if (evictionType_.find(evictpair) == evictionType_.end()) {
            mshr_->insertEviction(tag->getAddr(), event->getBaseAddr());
            evictionType_.insert(std::make_pair(evictpair, true));
        } else {
            evictionType_[evictpair] = true;
        }
        return nullptr;
    }
}

bool MESISharNoninclusive::handleDirEviction(Addr addr, DirectoryLine*& tag) {
    if (!tag)
        tag = dirArray_->findReplacementCandidate(addr);
    State state = tag->getState();

    if (is_debug_addr(tag->getAddr()))
        evictDI.oldst = tag->getState();

    stat_evict[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    DataLine * data;
    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                data = dataArray_->lookup(tag->getAddr(), false);
                if (invalidateAll(nullptr, tag, false)) {
                    evict = false;
                    tag->setState(S_Inv);
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, S_Inv");
                    break;
                } else if (!silentEvictClean_) {
                    if (data) {
                        sendWritebackFromCache(Command::PutS, tag, data, false);
                    } else {
                        sendWritebackFromMSHR(Command::PutS, tag, false);
                    }
                    wbSent = true;
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                } else if (is_debug_addr(tag->getAddr()))
                    printDebugAlloc(false, tag->getAddr(), "Dir, Drop");
                if (data) {
                    if (is_debug_addr(data->getAddr()))
                        printDebugAlloc(false, data->getAddr(), "Data");
                    dataArray_->deallocate(data);
                }
                tag->setState(I);
                evict = true;
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                data = dataArray_->lookup(tag->getAddr(), false);
                if (invalidateAll(nullptr, tag, false)) {
                    evict = false;
                    tag->setState(E_Inv);
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, E_Inv");
                    break;
                } else if (!silentEvictClean_) {
                    if (data) {
                        sendWritebackFromCache(Command::PutE, tag, data, false);
                    } else {
                        sendWritebackFromMSHR(Command::PutE, tag, false);
                    }
                    wbSent = true;
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                } else if (is_debug_addr(tag->getAddr()))
                    printDebugAlloc(false, tag->getAddr(), "Dir, Drop");
                tag->setState(I);

                if (data) {
                    if (is_debug_addr(data->getAddr()))
                        printDebugAlloc(false, data->getAddr(), "Data");
                    dataArray_->deallocate(data);
                }
                evict = true;
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                if (invalidateAll(nullptr, tag, false)) {
                    evict = false;
                    tag->setState(M_Inv);
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, M_Inv");
                } else {
                    if (is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                    data = dataArray_->lookup(tag->getAddr(), false);
                    if (data) {
                        sendWritebackFromCache(Command::PutM, tag, data, true);
                       if (is_debug_addr(data->getAddr()))
                           printDebugAlloc(false, data->getAddr(), "Data");
                        dataArray_->deallocate(data);
                    } else {
                        sendWritebackFromMSHR(Command::PutM, tag, true);
                    }
                    wbSent = true;
                    tag->setState(I);
                    evict = true;
                }
                break;
            }
        default:
            if (is_debug_addr(tag->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, tag->getAddr(), note.str());
            }
            return false;
    }

    if (wbSent && recvWritebackAck_) {
        mshr_->insertWriteback(tag->getAddr(), false);
    }

    recordPrefetchResult(tag, statPrefetchEvict);
    return evict;
}

bool MESISharNoninclusive::handleDataEviction(Addr addr, DataLine *&data) {
    bool evict = false;

    if (!data)
        data = dataArray_->findReplacementCandidate(addr);
    State state = data->getState();

    if (is_debug_addr(data->getAddr()))
        evictDI.oldst = data->getState();

    DirectoryLine* tag;

    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasSharers())) {
                    if (is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutS, tag, data, false);
                    if (recvWritebackAck_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, statPrefetchEvict);
                    notifyListenerOfEvict(data->getAddr(), lineSize_, 0);
                    tag->setState(I);
                    dirArray_->deallocate(tag);
                } else if (is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                return true;
            }
        case E:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasOwner() || tag->hasSharers())) {
                    if (is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutE, tag, data, false);
                    if (recvWritebackAck_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, statPrefetchEvict);
                    notifyListenerOfEvict(data->getAddr(), lineSize_, 0);
                    tag->setState(I);
                    dirArray_->deallocate(tag);
                } else if (is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                return true;
            }
        case M:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasOwner() || tag->hasSharers())) {
                    if (is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutM, tag, data, false);
                    if (recvWritebackAck_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, statPrefetchEvict);
                    notifyListenerOfEvict(data->getAddr(), lineSize_, 0);
                    tag->setState(I);
                    dirArray_->deallocate(tag);
                } else if (is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                return true;
            }
        default:
            if (is_debug_addr(data->getAddr())) {
                std::stringstream reason;
                reason << "InProg, " << StateString[state];
                printDebugAlloc(false, data->getAddr(), reason.str());
            }
            return false;
    }

    return false;
}


void MESISharNoninclusive::cleanUpEvent(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (inMSHR) {
        mshr_->removeFront(addr);
    }

    delete event;
}


void MESISharNoninclusive::cleanUpAfterRequest(MemEvent* event, bool inMSHR) {
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


void MESISharNoninclusive::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
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


void MESISharNoninclusive::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
            if (is_debug_addr(addr)) {
                if (eventDI.reason != "")
                    eventDI.reason = eventDI.reason + ",retry";
                else
                    eventDI.reason = "retry";
            }
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
            if (is_debug_addr(addr)) {
                if (eventDI.reason != "")
                    eventDI.reason = eventDI.reason + ",retry";
                else
                    eventDI.reason = "retry";
            }
        }
    }
}


/***********************************************************************************************************
 * Protocol helper functions
 ***********************************************************************************************************/

uint64_t MESISharNoninclusive::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, uint64_t time, Command cmd, bool success) {
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

    if (is_debug_event(event))
        eventDI.action = "Respond";

    return deliveryTime;
}

void MESISharNoninclusive::sendResponseDown(MemEvent * event, std::vector<uint8_t> * data, bool dirty, bool evict) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setDirty(dirty);
    }

    responseEvent->setEvict(evict);

    responseEvent->setSize(lineSize_);

    uint64_t deliverTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    Response resp = {responseEvent, deliverTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueue(resp);

    if (is_debug_event(event))
        eventDI.action = "Respond";
}


uint64_t MESISharNoninclusive::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
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

    uint64_t baseTime = (time > timestamp_) ? time : timestamp_;
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);

    if (is_debug_event(event))
        eventDI.action = "Forward";

    return deliveryTime-1;
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
void MESISharNoninclusive::sendWritebackFromCache(Command cmd, DirectoryLine* tag, DataLine* data, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, tag->getAddr(), tag->getAddr(), cmd);
    writeback->setDst(getDestination(tag->getAddr()));
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    /* Writeback data */
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*(data->getData()));
        writeback->setDirty(dirty);

        if (is_debug_addr(tag->getAddr())) {
            printData(data->getData(), false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t baseTime = (timestamp_ > tag->getTimestamp()) ? timestamp_ : tag->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    tag->setTimestamp(deliveryTime-1);

}

void MESISharNoninclusive::sendWritebackFromMSHR(Command cmd, DirectoryLine* tag, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, tag->getAddr(), tag->getAddr(), cmd);
    writeback->setDst(getDestination(tag->getAddr()));
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    /* Writeback data */
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(mshr_->getData(tag->getAddr()));
        writeback->setDirty(dirty);

        if (is_debug_addr(tag->getAddr())) {
            printData(&(mshr_->getData(tag->getAddr())), false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t baseTime = (timestamp_ > tag->getTimestamp()) ? timestamp_ : tag->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    tag->setTimestamp(deliveryTime-1);

}


void MESISharNoninclusive::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = event->makeResponse();
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t deliveryTime = timestamp_ + tagLatency_;

    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);

    if (is_debug_event(event))
        eventDI.action = "Ack";
}

uint64_t MESISharNoninclusive::sendFetch(Command cmd, MemEvent * event, std::string dst, bool inMSHR, uint64_t ts) {
    Addr addr = event->getBaseAddr();
    MemEvent * fetch = new MemEvent(cachename_, addr, addr, cmd);
    fetch->copyMetadata(event);
    fetch->setDst(dst);
    fetch->setSize(event->getSize());

    mshr_->incrementAcksNeeded(addr);

    if (responses.find(addr) != responses.end()) {
        responses.find(addr)->second.insert(std::make_pair(dst, fetch->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(dst, fetch->getID()));
        responses.insert(std::make_pair(addr, respid));
    }

    uint64_t baseTime = timestamp_ > ts ? timestamp_ : ts;
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);

    if (is_debug_addr(event->getBaseAddr())) {
        eventDI.action = "Stall";
        eventDI.reason = (cmd == Command::Fetch) ? "fetch data" : "Dgr owner";
    }
    return deliveryTime;
}


bool MESISharNoninclusive::invalidateExceptRequestor(MemEvent * event, DirectoryLine * tag, bool inMSHR, bool needData) {
    uint64_t deliveryTime = 0;
    std::string rqstr = event->getSrc();

    bool getData = needData;
    if (getData && tag->isSharer(event->getSrc()))
        getData = false;

    for (set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
        if (*it == rqstr) continue;

        if (getData) { // FetchInv
            getData = false;
            deliveryTime =  invalidateSharer(*it, event, tag, inMSHR, Command::FetchInv);
        } else { // Inv
            deliveryTime =  invalidateSharer(*it, event, tag, inMSHR);
        }
    }

    if (deliveryTime != 0) tag->setTimestamp(deliveryTime);

    return deliveryTime != 0;
}


bool MESISharNoninclusive::invalidateAll(MemEvent * event, DirectoryLine * tag, bool inMSHR, Command cmd) {
    uint64_t deliveryTime = 0;
    if (invalidateOwner(event, tag, inMSHR, (cmd == Command::NULLCMD ? Command::FetchInv : cmd))) {
        return true;
    } else {
        if (cmd == Command::NULLCMD)
            cmd = Command::Inv;
        for (std::set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
            deliveryTime = invalidateSharer(*it, event, tag, inMSHR, cmd);
        }
        if (deliveryTime != 0) {
            tag->setTimestamp(deliveryTime);
            return true;
        }
    }
    return false;
}

void MESISharNoninclusive::invalidateSharers(MemEvent * event, DirectoryLine * tag, bool inMSHR, bool needData, Command cmd) {
    uint64_t deliveryTime = 0;
    for (std::set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
        if (needData) {
            deliveryTime = invalidateSharer(*it, event, tag, inMSHR, Command::FetchInv);
            needData = false;
        } else {
            deliveryTime = invalidateSharer(*it, event, tag, inMSHR, cmd);
        }
    }
    tag->setTimestamp(deliveryTime);

}

uint64_t MESISharNoninclusive::invalidateSharer(std::string shr, MemEvent * event, DirectoryLine * tag, bool inMSHR, Command cmd) {
    if (tag->isSharer(shr)) {
        Addr addr = tag->getAddr();
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

        if (is_debug_addr(addr)) {
            eventDI.action = "Stall";
            eventDI.reason = "Inv sharer(s)";
        }

        uint64_t baseTime = timestamp_ > tag->getTimestamp() ? timestamp_ : tag->getTimestamp();
        uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(addr);

        return deliveryTime;
    }
    return 0;
}


bool MESISharNoninclusive::invalidateOwner(MemEvent * metaEvent, DirectoryLine * tag, bool inMSHR, Command cmd) {
    Addr addr = tag->getAddr();
    if (tag->getOwner() == "")
        return false;

    if (is_debug_addr(addr)) {
        eventDI.action = "Stall";
        eventDI.reason = "Inv owner";
    }

    MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
    if (metaEvent) {
        inv->copyMetadata(metaEvent);
        inv->setRqstr(metaEvent->getRqstr());
    } else {
        inv->setRqstr(cachename_);
    }
    inv->setDst(tag->getOwner());
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

    uint64_t baseTime = timestamp_ > tag->getTimestamp() ? timestamp_ : tag->getTimestamp();
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {inv, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    tag->setTimestamp(deliveryTime);

    return true;
}


// Search MSHR for a stalled replacement, move it to the front, and retry
// It'll be retried in the context of whatever request is currently stalled
bool MESISharNoninclusive::applyPendingReplacement(Addr addr) {
    for (int i = mshr_->getSize(addr) - 1; i > 0; i--) {
        MemEventBase * evb = mshr_->getEntryEvent(addr, i);
        if (evb && CommandWriteback[(int)(evb->getCmd())]) {
            mshr_->incrementAcksNeeded(addr);
            mshr_->moveEntryToFront(addr, i);
            if (responses.find(addr) != responses.end()) {
                responses.find(addr)->second.insert(std::make_pair(evb->getSrc(), evb->getID()));
            } else {
                std::map<std::string,MemEvent::id_type> respid;
                respid.insert(std::make_pair(evb->getSrc(), evb->getID()));
                responses.insert(std::make_pair(addr,respid));
            }
            retry(addr);
            return true;
        }
    }
    return false;
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESISharNoninclusive::addToOutgoingQueue(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueue(resp);
}

void MESISharNoninclusive::addToOutgoingQueueUp(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueueUp(resp);
}

/********************
 * Helper functions
 ********************/


void MESISharNoninclusive::removeSharerViaInv(MemEvent * event, DirectoryLine * tag, DataLine * data, bool remove) {
    Addr addr = event->getBaseAddr();
    tag->removeSharer(event->getSrc());
    if (!data && !mshr_->hasData(addr))
        mshr_->setData(addr, event->getPayload());

    if (remove) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty())
            responses.erase(addr);
    }
}

void MESISharNoninclusive::removeOwnerViaInv(MemEvent * event, DirectoryLine * tag, DataLine * data, bool remove) {
    Addr addr = event->getBaseAddr();
    tag->removeOwner();
    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload());

    if (event->getDirty()) {
        if (tag->getState() == E)
            tag->setState(M);
        else if (tag->getState() == E_Inv)
            tag->setState(M_Inv);
        else if (tag->getState() == E_InvX)
            tag->setState(M_InvX);
    }

    if (remove) {
        responses.find(addr)->second.erase(event->getSrc());
        if (responses.find(addr)->second.empty())
            responses.erase(addr);
    }
}

void MESISharNoninclusive::recordPrefetchResult(DirectoryLine * tag, Statistic<uint64_t> * stat) {
    if (tag->getPrefetch()) {
        stat->addData(1);
        tag->setPrefetch(false);
    }
}

MemEventInitCoherence* MESISharNoninclusive::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, true, false, lineSize_, true);
}

void MESISharNoninclusive::printLine(Addr addr) {
    return;
    if (!is_debug_addr(addr)) return;
    DirectoryLine * tag = dirArray_->lookup(addr, false);
    DataLine * data = dataArray_->lookup(addr, false);
    std::string state = (tag == nullptr) ? "NP" : tag->getString();
    debug->debug(_L8_, "  Line 0x%" PRIx64 ": %s Data Present: (%s)\n", addr, state.c_str(), data && data->getTag() ? "Y" : "N");
    if (data && data->getTag() && data->getTag() != tag) {
        if (data->getTag() != tag) {
            debug->fatal(CALL_INFO, -1, "Error: Data has a tag but it does not match the tag found in the directory. Addr 0x%" PRIx64 ". Data = (%" PRIu32 ",0x%" PRIx64 "), Tag = (%" PRIu32 ",0x%" PRIx64 ")\n",
                    addr, data->getIndex(), data->getAddr(), tag->getIndex(), tag->getAddr());
        }
    }
}

void MESISharNoninclusive::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());

    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");*/
}


void MESISharNoninclusive::recordLatency(Command cmd, int type, uint64_t latency) {
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
