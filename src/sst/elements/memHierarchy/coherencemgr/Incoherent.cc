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
#include "coherencemgr/Incoherent.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * Incoherent Controller Implementation
 * Non-Inclusive -> does not allocate on Get* requests except for prefetches
 * No writebacks except dirty data
 * I = not present in the cache, E = present & clean, M = present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

bool Incoherent::handleGetS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetS, localPrefetch, addr, state);

    switch (state) {
        case I:
            if (localPrefetch)
                status = processCacheMiss(event, line, inMSHR);
            else
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)Command::GetS][I]->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                    stat_misses->addData(1);
                    stat_miss[(int)Command::GetS][(int)inMSHR]->addData(1);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                sendTime = forwardMessage(event, event->getSize(), 0, nullptr);
                mshr_->setInProgress(addr);
                if (localPrefetch) { // Only case where we allocate a line
                    line->setState(IS);
                    line->setTimestamp(sendTime);
                }
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case E:
        case M:
            if (!inMSHR || mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                stat_eventState[(int)Command::GetS][state]->addData(1);
                stat_hit[(int)Command::GetS][(int)inMSHR]->addData(1);
                stat_hits->addData(1);
            }
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            recordPrefetchResult(line, statPrefetchHit);
            recordLatencyType(event->getID(), LatType::HIT);

            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime);
            if (is_debug_event(event))
                eventDI.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            else if (is_debug_event(event))
                eventDI.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject) {
        if (localPrefetch) return false;
        sendNACK(event);
    }

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool Incoherent::handleGetX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), event->getCmd(), false, addr, state);

    switch (state) {
        case I:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventState[(int)event->getCmd()][I]->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                    stat_miss[(int)event->getCmd()][(int)inMSHR]->addData(1);
                    stat_misses->addData(1);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                forwardMessage(event, event->getSize(), 0, nullptr);
                mshr_->setInProgress(addr);
                if (is_debug_event(event))
                    eventDI.reason = "miss";
            }
            break;
        case E:
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                stat_eventState[(int)event->getCmd()][I]->addData(1);
                stat_hit[(int)event->getCmd()][(int)inMSHR]->addData(1);
                stat_hits->addData(1);
            }
            recordPrefetchResult(line, statPrefetchHit);
            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            line->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::HIT);

            if (is_debug_event(event))
                eventDI.reason = "hit";
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


bool Incoherent::handleGetSX(MemEvent * event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


bool Incoherent::handleFlushLine(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
    if (!inMSHR)
        stat_eventState[(int)Command::FlushLine][state]->addData(1);

    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        doEvict(event, line);
        state = line->getState();
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, state == M, line->getData(), state == M, 0);
                line->setState(S_B);
                mshr_->setInProgress(addr);
            }
            break;
        default:
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


bool Incoherent::handleFlushLineInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::FlushLine, false, addr, state);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
    if (!inMSHR)
        stat_eventState[(int)Command::FlushLineInv][state]->addData(1);

    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        doEvict(event, line);
        state = line->getState();
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                recordPrefetchResult(line, statPrefetchEvict);
                forwardFlush(event, true, line->getData(), state == M, line->getTimestamp());
                line->setState(I_B);
                mshr_->setInProgress(addr);
            }
            break;
        default:
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


bool Incoherent::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!inMSHR)
        stat_eventState[(int)Command::PutE][state]->addData(1);

    switch (state) {
        case I:
            status = allocateLine(event, line, inMSHR);
            if (status == MemEventStatus::OK) {
                line->setData(event->getPayload(), 0);
                line->setState(E);
                if (sendWritebackAck_)
                    sendWritebackAck(event);
                cleanUpAfterRequest(event, inMSHR);
            }
            break;
        case E:
        case M:
            if (sendWritebackAck_)
                sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool Incoherent::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!inMSHR)
        stat_eventState[(int)Command::PutM][state]->addData(1);

    switch (state) {
        case I:
            status = allocateLine(event, line, inMSHR);
            if (status == MemEventStatus::OK) {
                line->setData(event->getPayload(), 0);
                line->setState(M);
                if (sendWritebackAck_)
                    sendWritebackAck(event);
                cleanUpAfterRequest(event, inMSHR);
            }
            break;
        case E:
            line->setState(M);
        case M:
            line->setData(event->getPayload(), 0);
            if (sendWritebackAck_)
                sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool Incoherent::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetSResp, false, addr, state);

    stat_eventState[(int)Command::GetSResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    sendResponseUp(req, &event->getPayload(), true, 0);

    if (line) {
        line->setState(E);
        line->setData(event->getPayload(), 0);
        // Has to be a local prefetch
        line->setPrefetch(true);
        recordPrefetchLatency(req->getID(), LatType::MISS);
    }

    cleanUpAfterResponse(event);

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    return true;
}


bool Incoherent::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event))
        eventDI.prefill(event->getID(), Command::GetXResp, false, addr, state);

    stat_eventState[(int)Command::GetXResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    sendResponseUp(req, &event->getPayload(), true, 0);

    cleanUpAfterResponse(event);

    return true;
}


bool Incoherent::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
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
            cacheArray_->deallocate(line);
            break;
        case S_B:
            line->setState(E);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (is_debug_addr(addr) && line) {
        eventDI.newst = line->getState();
        eventDI.verboseline = line->getString();
    }

    cleanUpAfterResponse(event);

    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool Incoherent::handleNULLCMD(MemEvent* event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    PrivateCacheLine * line = cacheArray_->lookup(oldAddr, false);
    printLine(event->getBaseAddr());
    bool evicted = handleEviction(newAddr, line, eventDI);
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
        if (mshr_->removeEvictPointer(oldAddr, newAddr))
            retry(oldAddr);
    } else { // Could be stalling for a new address or locked line
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (is_debug_addr(oldAddr) || is_debug_addr(line->getAddr()) || is_debug_addr(newAddr))
                debug->debug(_L8_, "\tAddr 0x%" PRIx64 " now waiting for 0x%" PRIx64 " instead of 0x%" PRIx64 "\n",
                        oldAddr, line->getAddr(), newAddr);
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    printLine(event->getBaseAddr());
    delete event;
    return true;
}


bool Incoherent::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    Addr addr = nackedEvent->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (is_debug_event(event) || is_debug_event(nackedEvent))
        eventDI.prefill(event->getID(), Command::NACK, false, addr, state);

    delete event;

    resendEvent(nackedEvent, false); // Re-send since we don't have races that make events redundant

    return true;
}


/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

MemEventStatus Incoherent::processCacheMiss(MemEvent * event, PrivateCacheLine* &line, bool inMSHR) {
    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
    if (inMSHR && mshr_->getFrontEvent(event->getBaseAddr()) != event) {
        if (is_debug_event(event))
            eventDI.action = "Stall";
        return MemEventStatus::Stall;
    }

    if (status == MemEventStatus::OK && !line) {
        status = allocateLine(event, line, inMSHR);
    }
    return status;
}

MemEventStatus Incoherent::allocateLine(MemEvent * event, PrivateCacheLine* &line, bool inMSHR) {
    evictDI.prefill(event->getID(), Command::Evict, false, 0, I);

    bool evicted = handleEviction(event->getBaseAddr(), line, evictDI);

    if (is_debug_event(event) || is_debug_addr(line->getAddr())) {
        evictDI.newst = line->getState();
        evictDI.verboseline = line->getString();
        evictDI.action = eventDI.action;
        evictDI.reason = eventDI.reason;
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return MemEventStatus::OK;
    } else {
        if (inMSHR || mshr_->insertEvent(event->getBaseAddr(), event, -1, false, true) != -1) {
            mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
            if (inMSHR)
                mshr_->setStalledForEvict(event->getBaseAddr(), true);
            if (is_debug_event(event)) {
                eventDI.action = "Stall";
                std::stringstream reason;
                reason << "evict 0x" << std::hex << line->getAddr();
                eventDI.reason = reason.str();
            }
            return MemEventStatus::Stall;
        }
        if (is_debug_event(event) || is_debug_addr(line->getAddr()))
            printDebugInfo(&evictDI);
        return MemEventStatus::Reject;
    }
}


bool Incoherent::handleEviction(Addr addr, PrivateCacheLine* &line, dbgin &diStruct) {
    if (!line)
        line = cacheArray_->findReplacementCandidate(addr);

    State state = line->getState();

    if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
        diStruct.oldst = state;
        diStruct.addr = line->getAddr();
    }

    stat_evict[state]->addData(1);

    switch (state) {
        case I:
            if (is_debug_addr(addr) || is_debug_addr(line->getAddr())) {
                diStruct.action = "None";
                diStruct.reason = "already idle";
            }
            return true;
        case E:
            if (!silentEvictClean_) {
                sendWriteback(Command::PutE, line, false);
                if (is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Writeback");
            } else if (is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            line->setState(I);
            break;
        case M:
            sendWriteback(Command::PutM, line, true);
            line->setState(I);
            if (is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Writeback");
            break;
        default:
            if (is_debug_addr(line->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    recordPrefetchResult(line, statPrefetchEvict);
    return true;
}


void Incoherent::cleanUpAfterRequest(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    if (inMSHR)
        mshr_->removeFront(addr);

    delete event;

    /* Replay any waiting events */
    retry(addr);
}


void Incoherent::cleanUpAfterResponse(MemEvent * event) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    mshr_->removeFront(addr);
    delete event;

    if (req)
        delete req;

    retry(addr);
}


void Incoherent::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr))
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
        } else { // Pointer -> another request is waiting to evict this address
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
        }
    }
}


void Incoherent::doEvict(MemEvent * event, PrivateCacheLine * line) {
    State state = line ? line->getState() : I;

    if (state == E || state == M) {
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
        }

        event->setEvict(false);
    }
}

/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

SimTime_t Incoherent::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, SimTime_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size());
    }

    if (success)
        responseEvent->setSuccess(true);

    if (time < timestamp_) time = timestamp_;
    SimTime_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);

    if (is_debug_event(event))
        eventDI.action = "Respond";

    return deliveryTime;
}


void Incoherent::sendWriteback(Command cmd, PrivateCacheLine * line, bool dirty) {
    MemEvent * writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setDst(getDestination(line->getAddr()));
    writeback->setSize(lineSize_);

    uint64_t latency = tagLatency_;

    if (dirty) {
        writeback->setPayload(*(line->getData()));
        writeback->setDirty(dirty);

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t time = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    time += latency;
    Response resp = {writeback, time, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    line->setTimestamp(time-1);
}


void Incoherent::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
    MemEvent * flush = new MemEvent(*event);
    flush->setSrc(cachename_);
    flush->setDst(getDestination(event->getBaseAddr()));

    uint64_t latency = tagLatency_;
    if (evict) {
        flush->setEvict(true);
        flush->setPayload(*data);
        flush->setDirty(dirty);
        latency = accessLatency_;
    } else {
        flush->setPayload(0, nullptr);
    }

    uint64_t sendTime = timestamp_ + latency;
    Response resp = {flush, sendTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);

    if (is_debug_addr(event->getBaseAddr()))
        eventDI.action = "Forward";
}


void Incoherent::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = event->makeResponse();
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t time = timestamp_ + tagLatency_;

    Response resp = { ack, time, packetHeaderBytes };
    addToOutgoingQueueUp(resp);

    if (is_debug_event(event))
        eventDI.action = "Ack";
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void Incoherent::addToOutgoingQueue(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueue(resp);
}

void Incoherent::addToOutgoingQueueUp(Response& resp) {
    stat_eventSent[(int)resp.event->getCmd()]->addData(1);
    CoherenceController::addToOutgoingQueueUp(resp);
}


/*************************
 * Helper functions
 *************************/

MemEventInitCoherence * Incoherent::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, false, false, lineSize_, true);
}

void Incoherent::recordPrefetchResult(PrivateCacheLine * line, Statistic<uint64_t>* stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}

void Incoherent::recordLatency(Command cmd, int type, uint64_t latency) {
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

void Incoherent::printLine(Addr addr) { }

