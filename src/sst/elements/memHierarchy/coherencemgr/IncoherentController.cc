// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
                    parent->getName().c_str(), StateString[state], wbCacheLine->getBaseAddr(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle a Get* request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction IncoherentController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
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
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}


/**
 *  Handle replacement. 
 */
CacheAction IncoherentController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    // May need to update state since we just allocated
    if (reqEvent != NULL && cacheLine && cacheLine->getState() == I) {
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
            return handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
        case Command::FlushLine:
            return handleFlushLineRequest(event, cacheLine, reqEvent, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return action;
}


/**
 *  Handle invalidation request.
 *  Invalidations do not exist for incoherent caches but function must be implemented.
 */
CacheAction IncoherentController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay) {
    debug->fatal(CALL_INFO, -1, "%s, Error: Received an invalidation request: %s, but incoherent protocol does not support invalidations. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    
    return STALL; // eliminate compiler warning
}


/**
 *  Handle data responses.
 */
CacheAction IncoherentController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            return handleDataResponse(respEvent, cacheLine, reqEvent);
        case Command::FlushLineResp:
            recordStateEventCount(respEvent->getCmd(), cacheLine ? cacheLine->getState() : I);
            if (cacheLine && cacheLine->getState() == S_B) cacheLine->setState(E);
            else if (cacheLine && cacheLine->getState() == I_B) cacheLine->setState(I);
            sendFlushResponse(reqEvent, respEvent->success());
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return DONE;
}


/* Incoherent caches always retry NACKs since there are not Inv/Fetch's to race
 * with and resolve transactions early
 */
bool IncoherentController::isRetryNeeded(MemEvent * event, CacheLine * cacheLine) {
    return true;
}

/*
 *  Return type of miss for profiling incoming events
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request) --> N/A for incoherent
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition --> N/A for incoherent
 */
int IncoherentController::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
    State state = cacheLine->getState();
    if (state == I) return 1;
    return 0;
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Internal event handlers
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Handle GetS requests
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    if (is_debug_event(event)) printData(cacheLine->getData(), false);

    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == parent->getName());
    recordStateEventCount(event->getCmd(), state);

    uint64_t sendTime = 0;

    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            
            if (is_debug_event(event)) debug->debug(_L6_,"Forwarding GetS, new state IS\n");
            
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], getCurrentSimTimeNano());
    }
    return STALL;    // eliminate compiler warning
}


/**
 *  Handle GetX request
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
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
            
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
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
                    parent->getName().c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
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
    
    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == parent->getName());
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            cacheLine->setState(E);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                cacheLine->setPrefetch(true);
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
                    parent->getName().c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface 
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Send writeback to lower level cache
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void IncoherentController::sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr){
    MemEvent* newCommandEvent = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
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
    MemEvent * flush = new MemEvent(parent, baseAddr, baseAddr, cmd);
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
    switch (cmd) {
        case Command::GetS:
            if (state == I) stat_stateEvent_GetS_I->addData(1);
            else if (state == E) stat_stateEvent_GetS_E->addData(1);
            else if (state == M) stat_stateEvent_GetS_M->addData(1);
            break;
        case Command::GetX:    
            if (state == I) stat_stateEvent_GetX_I->addData(1);
            else if (state == E) stat_stateEvent_GetX_E->addData(1);
            else if (state == M) stat_stateEvent_GetX_M->addData(1);
            break;
        case Command::GetSX:    
            if (state == I) stat_stateEvent_GetSX_I->addData(1);
            else if (state == E) stat_stateEvent_GetSX_E->addData(1);
            else if (state == M) stat_stateEvent_GetSX_M->addData(1);
            break;
        case Command::GetSResp:
            if (state == IS) stat_stateEvent_GetSResp_IS->addData(1);
            break;
        case Command::GetXResp:
            if (state == IM) stat_stateEvent_GetXResp_IM->addData(1);
            else if (state == IS) stat_stateEvent_GetXResp_IS->addData(1);
            break;
        case Command::PutE:
            if (state == I) stat_stateEvent_PutE_I->addData(1);
            else if (state == E) stat_stateEvent_PutE_E->addData(1);
            else if (state == M) stat_stateEvent_PutE_M->addData(1);
            else if (state == IS) stat_stateEvent_PutE_IS->addData(1);
            else if (state == IM) stat_stateEvent_PutE_IM->addData(1);
            else if (state == I_B) stat_stateEvent_PutE_IB->addData(1);
            else if (state == S_B) stat_stateEvent_PutE_SB->addData(1);
            break;
        case Command::PutM:
            if (state == I) stat_stateEvent_PutM_I->addData(1);
            else if (state == E) stat_stateEvent_PutM_E->addData(1);
            else if (state == M) stat_stateEvent_PutM_M->addData(1);
            else if (state == IS) stat_stateEvent_PutM_IS->addData(1);
            else if (state == IM) stat_stateEvent_PutM_IM->addData(1);
            else if (state == I_B) stat_stateEvent_PutM_IB->addData(1);
            else if (state == S_B) stat_stateEvent_PutM_SB->addData(1);
            break;
        case Command::FlushLine:
            if (state == I) stat_stateEvent_FlushLine_I->addData(1);
            else if (state == E) stat_stateEvent_FlushLine_E->addData(1);
            else if (state == M) stat_stateEvent_FlushLine_M->addData(1);
            else if (state == IS) stat_stateEvent_FlushLine_IS->addData(1);
            else if (state == IM) stat_stateEvent_FlushLine_IM->addData(1);
            else if (state == I_B) stat_stateEvent_FlushLine_IB->addData(1);
            else if (state == S_B) stat_stateEvent_FlushLine_SB->addData(1);
            break;
        case Command::FlushLineInv:
            if (state == I) stat_stateEvent_FlushLineInv_I->addData(1);
            else if (state == E) stat_stateEvent_FlushLineInv_E->addData(1);
            else if (state == M) stat_stateEvent_FlushLineInv_M->addData(1);
            else if (state == IS) stat_stateEvent_FlushLineInv_IS->addData(1);
            else if (state == IM) stat_stateEvent_FlushLineInv_IM->addData(1);
            else if (state == I_B) stat_stateEvent_FlushLineInv_IB->addData(1);
            else if (state == S_B) stat_stateEvent_FlushLineInv_SB->addData(1);
            break;
        case Command::FlushLineResp:
            if (state == I) stat_stateEvent_FlushLineResp_I->addData(1);
            else if (state == I_B) stat_stateEvent_FlushLineResp_IB->addData(1);
            else if (state == S_B) stat_stateEvent_FlushLineResp_SB->addData(1);
            break;
        default:
            break;
    }
}


void IncoherentController::recordEventSentDown(Command cmd) {
    switch(cmd) {
        case Command::GetS:
            stat_eventSent_GetS->addData(1);
            break;
        case Command::GetX:
            stat_eventSent_GetX->addData(1);
            break;
        case Command::GetSX:
            stat_eventSent_GetSX->addData(1);
            break;
        case Command::PutE:
            stat_eventSent_PutE->addData(1);
            break;
        case Command::PutM:
            stat_eventSent_PutM->addData(1);
            break;
        case Command::FlushLine:
            stat_eventSent_FlushLine->addData(1);
            break;
        case Command::FlushLineInv:
            stat_eventSent_FlushLineInv->addData(1);
            break;
        case Command::NACK:
            stat_eventSent_NACK_down->addData(1);
            break;
        default:
            break;
    }
}

void IncoherentController::recordEventSentUp(Command cmd) {
    switch (cmd) {
        case Command::GetSResp:
            stat_eventSent_GetSResp->addData(1);
            break;
        case Command::GetXResp:
            stat_eventSent_GetXResp->addData(1);
            break;
        case Command::FlushLineResp:
            stat_eventSent_FlushLineResp->addData(1);
            break;
        case Command::NACK:
            stat_eventSent_NACK_up->addData(1);
            break;
        default:
            break;
    }   
}
