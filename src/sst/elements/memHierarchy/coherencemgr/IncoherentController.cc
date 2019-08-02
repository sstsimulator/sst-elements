// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
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
#include "coherencemgr/IncoherentController.h"

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
 * Incoherent Controller Implementation
 * Non-Inclusive caches do not allocate on Get* requests except for prefetches
 * Inclusive caches do
 * No writebacks except dirty data
 * I = not present in the cache, E = present & clean, M = present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------
 *  External interface functions for routing events from cache controller to appropriate coherence handlers
 *---------------------------------------------------------------------------------------------------------------------*/

/**
 *  Handle eviction. Stall if eviction candidate is in transition.
 */
CacheAction IncoherentController::handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
    recordEvictionState(state);
    
    switch (state) {
        case I:
            return DONE;
        case E:
            if (writebackCleanBlocks_) {
                sendWriteback(Command::PutE, wbCacheLine, origRqstr);
            }
            wbCacheLine->setState(I);
            if (wbCacheLine->getPrefetch()) {
                wbCacheLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            return DONE;
        case M:
            sendWriteback(Command::PutM, wbCacheLine, origRqstr);
            wbCacheLine->setState(I);
            if (wbCacheLine->getPrefetch()) {
                wbCacheLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            return DONE;
        case IS:
        case IM:
        case S_B:
        case I_B:
            return STALL;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle a Get* request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction IncoherentController::handleRequest(MemEvent* event, bool replay) {
    Addr addr = event->getBaseAddr();

    CacheLine * cacheLine = cacheArray_->lookup(addr, !replay);
    
    if (cacheLine && cacheLine->inTransition()) {
        allocateMSHR(addr, event);
        return STALL;
    }

    if (!cacheLine || !cacheLine->valid()) {
        if (is_debug_addr(addr))
            debug->debug(_L3_, "-- Miss --\n");

        allocateMSHR(addr, event);
        if (event->inProgress()) {
            if (is_debug_addr(addr))
                debug->debug(_L8_, "Attempted retry too early, continue stalling\n");
            return STALL;
        }

        vector <uint8_t> * data = &event->getPayload();
        forwardMessage(event, addr, event->getSize(), 0, data);
        event->setInProgress(true);
        recordMiss(event->getID());
        return STALL;
    }
    
    if (is_debug_addr(cacheLine->getBaseAddr()))   
        debug->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);

    Command cmd = event->getCmd();

    switch(cmd) {
        case Command::GetS:
            return handleGetSRequest(event, cacheLine, replay);
        case Command::GetX:
        case Command::GetSX:
            return handleGetXRequest(event, cacheLine, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}


/**
 *  Handle replacement. 
 */
CacheAction IncoherentController::handleReplacement(MemEvent* event, bool replay) {
    Addr addr = event->getBaseAddr();
    CacheLine* cacheLine = cacheArray_->lookup(addr, true);
    if (cacheLine == nullptr) {
        if (is_debug_addr(addr)) debug->debug(_L3_, "-- Cache Miss --\n");

        if (!allocateLine(addr, event)) {
            if (mshr_->getAcksNeeded(addr) == 0) {
                allocateMSHR(addr, event);
                return STALL;
            } else {
                if (is_debug_addr(addr)) debug->debug(_L3_, "Can't allocate immediately, but handling collision anyways\n");
            }
        }
        cacheLine = cacheArray_->lookup(addr, false);
    }

    MemEvent * reqEvent = nullptr;
    if (mshr_->exists(addr))
            reqEvent = static_cast<MemEvent*>(mshr_->lookupFront(addr));

    // May need to update state since we just allocated
    if (reqEvent != nullptr && cacheLine && cacheLine->getState() == I) {
        if (reqEvent->getCmd() == Command::GetS) cacheLine->setState(IS);
        else if (reqEvent->getCmd() == Command::GetX) cacheLine->setState(IM);
        else if (reqEvent->getCmd() == Command::FlushLine) cacheLine->setState(S_B);
        else if (reqEvent->getCmd() == Command::FlushLineInv) cacheLine->setState(I_B);
    }

    if (cacheLine != NULL && (is_debug_event(event)))
        debug->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);


    Command cmd = event->getCmd();
    CacheAction action = DONE;

    if (cmd == Command::PutE || cmd == Command::PutM) {
        action = handlePutMRequest(event, cacheLine);
    } else {
	debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                ownerName_.c_str(), CommandString[(int)cmd], addr, event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    
    if ((action == DONE || action == STALL) && reqEvent != nullptr) {
        mshr_->removeFront(addr);
        delete reqEvent;
    }
    if (action == STALL || action == BLOCK)
        allocateMSHR(addr, event);

    return action;
}

CacheAction IncoherentController::handleFlush(MemEvent* event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay) {
    // May need to update state since we just allocated
    if (reqEvent != nullptr && cacheLine && cacheLine->getState() == I) {
        if (reqEvent->getCmd() == Command::GetS) cacheLine->setState(IS);
        else if (reqEvent->getCmd() == Command::GetX) cacheLine->setState(IM);
        else if (reqEvent->getCmd() == Command::FlushLine) cacheLine->setState(S_B);
        else if (reqEvent->getCmd() == Command::FlushLineInv) cacheLine->setState(I_B);
    }

    if (cacheLine != NULL && (is_debug_event(event)))
        debug->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);


    Command cmd = event->getCmd();
    CacheAction action = DONE;

    switch(cmd) {
        case Command::PutE:
        case Command::PutM:
            action = handlePutMRequest(event, cacheLine);
            break;
        case Command::FlushLineInv:
            action = handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
            break;
        case Command::FlushLine:
            action = handleFlushLineRequest(event, cacheLine, reqEvent, replay);
            break;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    
    return action;
}


/**
 *  Handle invalidation request.
 *  Invalidations do not exist for incoherent caches but function must be implemented.
 */
CacheAction IncoherentController::handleInvalidationRequest(MemEvent * event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: Received an invalidation request: %s, but incoherent protocol does not support invalidations. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            ownerName_.c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    
    return STALL; // eliminate compiler warning
}


/**
 *  Handle data responses.
 */
CacheAction IncoherentController::handleCacheResponse(MemEvent * event, bool inMSHR) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* line = cacheArray_->lookup(bAddr, false);
    MemEvent* reqEvent = static_cast<MemEvent*>(mshr_->lookupFront(bAddr));

    if (is_debug_addr(bAddr))
        printLine(bAddr);

    CacheAction action = DONE;
    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            action = handleDataResponse(event, line, reqEvent);
            break;
        case Command::FlushLineResp:
            recordStateEventCount(event->getCmd(), line ? line->getState() : I);
            if (line && line->getState() == S_B) line->setState(E);
            else if (line && line->getState() == I_B) line->setState(I);
            sendFlushResponse(reqEvent, event->success());
            action = DONE;
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    
    if (is_debug_addr(bAddr))
        printLine(bAddr);
    
    if (action == DONE) {
        mshr_->removeFront(bAddr);
    }
        
    delete event;
    delete reqEvent;

    return action;
}

CacheAction IncoherentController::handleFetchResponse(MemEvent * event, bool inMSHR) {
    Command cmd = event->getCmd(); 
    debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                ownerName_.c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    return DONE;
}

bool IncoherentController::isCacheHit(MemEvent* event) {
    CacheLine * line = cacheArray_->lookup(event->getBaseAddr(), false);
    if (!line || line->getState() == I)
        return false;
    else
        return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Internal event handlers
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Handle GetS requests
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Addr addr = event->getBaseAddr();
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    if (is_debug_event(event)) printData(cacheLine->getData(), false);

    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == ownerName_);
    recordStateEventCount(event->getCmd(), state);

    uint64_t sendTime = 0;

    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            
            if (is_debug_event(event)) debug->debug(_L6_,"Forwarding GetS, new state IS\n");
            recordLatencyType(event->getID(), LatType::MISS);
            allocateMSHR(addr, event); 
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], getCurrentSimTimeNano());
    }
    return STALL;    // eliminate compiler warning
}


/**
 *  Handle GetX request
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Addr addr = event->getBaseAddr();
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    recordStateEventCount(event->getCmd(), state);
    
    uint64_t sendTime = 0;
    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            cacheLine->setState(IM);
            
            if (is_debug_event(event)) debug->debug(_L6_,"Forwarding GetX, new state IM\n");
            
            recordLatencyType(event->getID(), LatType::MISS);
            allocateMSHR(addr, event); 
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            sendTime = sendResponseUp(event, cacheLine->getData(), replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
           
            if (cacheLine->getPrefetch()) {
                cacheLine->setPrefetch(false);
                statPrefetchHit->addData(1);
            }

            if (is_debug_event(event)) printData(cacheLine->getData(), false);
            
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle PutM
 *  Incoherent caches only replace dirty data
 */
CacheAction IncoherentController::handlePutMRequest(MemEvent* event, CacheLine* cacheLine) {
    State state = cacheLine->getState();

    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
            cacheLine->setData(event->getPayload(), 0);
            if (event->getDirty()) cacheLine->setState(M);
            else cacheLine->setState(E);
            break;
        case IS: // Occurs if we issued a request for a block that was cached by an upper level cache
        case IM:
        case S_B:
        case I_B:
            if (event->getDirty()) return BLOCK;
            else return DONE;
        case E:
            if (event->getDirty()) cacheLine->setState(M);
        case M:
            if (event->getDirty()) {
                cacheLine->setData(event->getPayload(), 0);
                
                if (is_debug_event(event)) printData(cacheLine->getData(), true);
            }

            break;
        default:
	    debug->fatal(CALL_INFO, -1, "%s, Error: Received PutM/E but cache state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE;
}


CacheAction IncoherentController::handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay) {
    State state = cacheLine ? cacheLine->getState() : I;
    recordStateEventCount(event->getCmd(), state);
    
    if (reqEvent != NULL) return STALL;
    
    forwardFlushLine(event->getBaseAddr(), event->getRqstr(), cacheLine, Command::FlushLine);

    if (cacheLine && state != I) cacheLine->setState(S_B);
    else if (cacheLine) cacheLine->setState(I_B);
    event->setInProgress(true);
    recordLatencyType(event->getID(), LatType::HIT);
    return STALL;   // wait for response
}


CacheAction IncoherentController::handleFlushLineInvRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay) {
    State state = cacheLine ? cacheLine->getState() : I;
    recordStateEventCount(event->getCmd(), state);

    if (reqEvent != NULL) return STALL;

    forwardFlushLine(event->getBaseAddr(), event->getRqstr(), cacheLine, Command::FlushLineInv);
    
    if (cacheLine && cacheLine->getPrefetch()) {
        cacheLine->setPrefetch(false);
        statPrefetchEvict->addData(1);
    }

    if (cacheLine) cacheLine->setState(I_B);
    event->setInProgress(true);
    recordLatencyType(event->getID(), LatType::HIT);
    return STALL;   // wait for response
}


/**
 * Handles data responses (GetSResp, GetXResp)
 */
CacheAction IncoherentController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    if (!inclusive_ && (cacheLine == NULL || cacheLine->getState() == I)) {
        sendResponseUp(origRequest, &responseEvent->getPayload(), true, 0);
        return DONE;
    }

    cacheLine->setData(responseEvent->getPayload(), 0);
    if (is_debug_event(responseEvent)) printData(cacheLine->getData(), true);

    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == ownerName_);
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            cacheLine->setState(E);
            if (localPrefetch) {
                cacheLine->setPrefetch(true);
                recordPrefetchLatency(origRequest->getID(), LatType::MISS);
                return DONE;    
            }
            sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), false);
            return DONE;
        case IM:
            cacheLine->setState(M); 
            sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), false);
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


bool IncoherentController::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    if (is_debug_event(nackedEvent)) debug->debug(_L3_, "NACK received.\n");

    resendEvent(nackedEvent, nackedEvent->fromHighNetNACK());    // Always resend

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for managing data structures
 *---------------------------------------------------------------------------------------------------------------------*/
bool IncoherentController::allocateLine(Addr addr, MemEvent* event) {
    CacheLine* replacementLine = cacheArray_->findReplacementCandidate(addr, true);

    if (replacementLine->valid() && is_debug_addr(addr)) {
        debug->debug(_L6_, "Evicting 0x%" PRIx64 "\n", replacementLine->getBaseAddr());
    }

    if (replacementLine->valid()) {
        if (replacementLine->inTransition()) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }

        CacheAction action = handleEviction(replacementLine, getName(), false);
        if (action == STALL) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }
    }

    notifyListenerOfEvict(event, replacementLine);
    cacheArray_->replace(addr, replacementLine);
    return true;
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface 
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Send writeback to lower level cache
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void IncoherentController::sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr){
    MemEvent* newCommandEvent = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    newCommandEvent->setSize(cacheLine->getSize());
    if (cmd == Command::PutM || writebackCleanBlocks_) {
        newCommandEvent->setPayload(*cacheLine->getData());
        
        if (is_debug_addr(cacheLine->getBaseAddr())) printData(cacheLine->getData(), false);
    }
    newCommandEvent->setRqstr(origRqstr);
    if (cacheLine->getState() == M) newCommandEvent->setDirty(true);
    
    uint64_t deliveryTime = timestamp_ + accessLatency_;
    Response resp = {newCommandEvent, deliveryTime, packetHeaderBytes + newCommandEvent->getPayloadSize()};
    addToOutgoingQueue(resp);

    if (is_debug_addr(cacheLine->getBaseAddr())) debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s\n", deliveryTime, CommandString[(int)cmd]);
}


/**
 *  Forward a flush line request, with or without data
 */
void IncoherentController::forwardFlushLine(Addr baseAddr, string origRqstr, CacheLine * cacheLine, Command cmd) {
    MemEvent * flush = new MemEvent(ownerName_, baseAddr, baseAddr, cmd);
    flush->setDst(getDestination(baseAddr));
    flush->setRqstr(origRqstr);
    flush->setSize(lineSize_);
    uint64_t latency = tagLatency_;
    // Always include block if we have it TODO relax this to just include dirty data
    if (cacheLine) {
        if (cacheLine->getState() == M) flush->setDirty(true);
        flush->setPayload(*cacheLine->getData());
        latency = accessLatency_;
    }
    uint64_t baseTime = timestamp_;
    if (cacheLine && cacheLine->getTimestamp() > baseTime) baseTime = cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (cacheLine) cacheLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_addr(baseAddr)) {
        debug->debug(_L3_,"Forwarding %s at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", CommandString[(int)cmd], deliveryTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str());
    }
}


/**
 *  Send a flush response up
 */
void IncoherentController::sendFlushResponse(MemEvent * requestEvent, bool success) {
    MemEvent * flushResponse = requestEvent->makeResponse();
    flushResponse->setSuccess(success);
    flushResponse->setDst(requestEvent->getSrc());

    uint64_t deliveryTime = timestamp_ + mshrLatency_;
    Response resp = {flushResponse, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    
    if (is_debug_event(requestEvent)) {
        debug->debug(_L3_,"Sending Flush Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)flushResponse->getCmd()], flushResponse->getSrc().c_str());
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void IncoherentController::addToOutgoingQueue(Response& resp) {
    CoherenceController::addToOutgoingQueue(resp);
    recordEventSentDown(resp.event->getCmd());
}

void IncoherentController::addToOutgoingQueueUp(Response& resp) {
    CoherenceController::addToOutgoingQueueUp(resp);
    recordEventSentUp(resp.event->getCmd());

}

/*----------------------------------------------------------------------------------------------------------------------
 *  Miscellaneous helper functions
 *---------------------------------------------------------------------------------------------------------------------*/


/*
 *  Print data values for debugging
 */
void IncoherentController::printData(vector<uint8_t> * data, bool set) {
    /*if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());
    
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");
    */
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Miscellaneous helper functions
 *---------------------------------------------------------------------------------------------------------------------*/


void IncoherentController::recordStateEventCount(Command cmd, State state) {
    stat_eventState[(int)cmd][state]->addData(1);
}


void IncoherentController::recordEventSentDown(Command cmd) {
    stat_eventSent[(int)cmd]->addData(1);
}

void IncoherentController::recordEventSentUp(Command cmd) {
    stat_eventSent[(int)cmd]->addData(1);
}

void IncoherentController::recordLatency(Command cmd, int type, uint64_t latency) {
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

void IncoherentController::printLine(Addr addr) {
    if (!is_debug_addr(addr)) return;
    CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : NP;
    unsigned int sharers = line ? line->numSharers() : 0;
    string owner = line ? line->getOwner() : "";
    debug->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\"\n", addr, StateString[state], sharers, owner.c_str());
}
