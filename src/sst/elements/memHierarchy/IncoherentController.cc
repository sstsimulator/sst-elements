// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <vector>
#include "IncoherentController.h"
using namespace SST;
using namespace SST::MemHierarchy;

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
                sendWriteback(PutE, wbCacheLine, origRqstr);
            }
            wbCacheLine->setState(I);
            return DONE;
        case M:
            sendWriteback(PutM, wbCacheLine, origRqstr);
            wbCacheLine->setState(I);
            return DONE;
        case IS:
            return STALL;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    name_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle a Get* request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction IncoherentController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr())   d_->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);
#endif

    Command cmd = event->getCmd();

    switch(cmd) {
        case GetS:
            return handleGetSRequest(event, cacheLine, replay);
        case GetX:
        case GetSEx:
            return handleGetXRequest(event, cacheLine, replay);
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}


/**
 *  Handle replacement. 
 */
CacheAction IncoherentController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    // May need to update state since we just allocated
    if (reqEvent != NULL && reqEvent->getCmd() == GetS && cacheLine->getState() == I) cacheLine->setState(IS);
    if (reqEvent != NULL && reqEvent->getCmd() == GetX && cacheLine->getState() == I) cacheLine->setState(IM);

#ifdef __SST_DEBUG_OUTPUT__
    if (cacheLine != NULL && (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()))   d_->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);
#endif


    Command cmd = event->getCmd();
    CacheAction action = DONE;

    switch(cmd) {
        case PutM:
            action = handlePutMRequest(event, cacheLine);
            break;
        case PutE:
            action = handlePutMRequest(event, cacheLine);
            break;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return action;
}


/**
 *  Handle invalidation request.
 *  Invalidations do not exist for incoherent caches but function must be implemented.
 */
CacheAction IncoherentController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, bool replay) {
    d_->fatal(CALL_INFO, -1, "%s, Error: Received an invalidation request: %s, but incoherent protocol does not support invalidations. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    
    return STALL; // eliminate compiler warning
}


/**
 *  Handle data responses.
 */
CacheAction IncoherentController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case GetSResp:
        case GetXResp:
            return handleDataResponse(respEvent, cacheLine, reqEvent);
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
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
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == ((Component*)owner_)->getName()));
    recordStateEventCount(event->getCmd(), state);

    uint64_t sendTime = 0;

    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
#ifdef __SST_DEBUG_OUTPUT__
            d_->debug(_L6_,"Forwarding GetS, new state IS\n");
#endif
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            sendTime = sendResponseUp(event, E, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    ((Component *)owner_)->getName().c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
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
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            sendTime = sendResponseUp(event, M, cacheLine->getData(), replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
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
            cacheLine->setData(event->getPayload(), event);
            if (event->getDirty()) cacheLine->setState(M);
            else cacheLine->setState(E);
            break;
        case IS: // Occurs if we issued a request for a block that was cached by an upper level cache
        case IM:
            if (event->getDirty()) return BLOCK;
            else return DONE;
        case E:
            if (event->getDirty()) cacheLine->setState(M);
        case M:
            if (event->getDirty()) {
                cacheLine->setData(event->getPayload(), event);
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), true);
            }
            break;
        default:
	    d_->fatal(CALL_INFO, -1, "%s, Error: Updating data but cache is not in E or M state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 * Handles data responses (GetSResp, GetXResp)
 */
CacheAction IncoherentController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    if (!inclusive_ && (cacheLine == NULL || cacheLine->getState() == I)) {
        sendResponseUp(origRequest, responseEvent->getGrantedState(), &responseEvent->getPayload(), true, 0);
        return DONE;
    }

    cacheLine->setData(responseEvent->getPayload(), responseEvent);
    if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);

    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == ((Component*)owner_)->getName()));
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            cacheLine->setState(E);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            sendTime = sendResponseUp(origRequest, cacheLine->getState(), cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        case IM:
            cacheLine->setState(M); 
            sendTime = sendResponseUp(origRequest, M, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
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
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    newCommandEvent->setSize(cacheLine->getSize());
    if (cmd == PutM || writebackCleanBlocks_) {
        newCommandEvent->setPayload(*cacheLine->getData());
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) printData(cacheLine->getData(), false);
    }
    newCommandEvent->setRqstr(origRqstr);
    if (cacheLine->getState() == M) newCommandEvent->setDirty(true);
    
    uint64 deliveryTime = timestamp_ + accessLatency_;
    Response resp = {newCommandEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s\n", deliveryTime, CommandString[cmd]);
#endif
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

