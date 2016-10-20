// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <vector>
#include "L1IncoherentController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Incoherent Controller 
 * States:
 * I: not present
 * E: present & clean
 * M: present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/
	    
/*
 * Public interface functions:
 *      handleEviction
 *      handleRequest
 *      handleResponse
 *      handleReplacement (not relevant for L1s)
 *      handleInvalidationRequest
 *      isRetryNeeded 
 */
  
CacheAction L1IncoherentController::handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
   
    /* L1 specific code */
    if (wbCacheLine->isLocked()) {
        wbCacheLine->setEventsWaitingForLock(true);
        return STALL;
    }
    recordEvictionState(state);

    switch(state) {
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
        case IM:
        case I_B:
        case S_B:
            return STALL;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    name_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle request at bottomCC
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction L1IncoherentController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay){
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
 *  Handle replacement - Not relevant for L1s but required to implement 
 */
CacheAction L1IncoherentController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    if (event->getCmd() == FlushLineInv) {    
            return handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
    } else if (event->getCmd() == FlushLine) {
            return handleFlushLineRequest(event, cacheLine, reqEvent, replay);
    } else {
        d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return IGNORE;
}


/**
 *  Handle invalidation - Inv, FetchInv, or FetchInvX
 *  Return: whether Inv was successful (true) or we are waiting on further actions (false). L1 returns true (no sharers/owners).
 */
CacheAction L1IncoherentController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * collisonEvent, bool replay) {
    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    return IGNORE;
}




CacheAction L1IncoherentController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case GetSResp:
        case GetXResp:
            handleDataResponse(respEvent, cacheLine, reqEvent);
            break;
        case FlushLineResp:
            recordStateEventCount(respEvent->getCmd(), cacheLine ? cacheLine->getState() : I);
            if (cacheLine && cacheLine->getState() == S_B) cacheLine->setState(E);
            else if (cacheLine && cacheLine->getState() == I_B) cacheLine->setState(I);
            sendFlushResponse(reqEvent, respEvent->success(), timestamp_, true);
            break;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}



bool L1IncoherentController::isRetryNeeded(MemEvent * event, CacheLine * cacheLine) {
    return true;    // No coherence races to resolve a request
}


/*------------------------------------------------------------------------------------------------
 *  Private event handlers
 *      HandleGetS
 *      HandleGetX
 *      handleDataResponse
 *
 *------------------------------------------------------------------------------------------------*/


CacheAction L1IncoherentController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay){
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == name_));
    recordStateEventCount(event->getCmd(), state);
    
    uint64_t sendTime = 0;

    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            if (event->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(event, S, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

    }
    return STALL;    // eliminate compiler warning
}


/*
 *  Return: whether event was handled or is waiting for further responses
 */
CacheAction L1IncoherentController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    vector<uint8_t>* data = cacheLine->getData();
    
    uint64_t sendTime = 0;

    recordStateEventCount(event->getCmd(), state);
    switch (state) {
        case I:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            cacheLine->setState(IM);
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            if (cmd == GetX) {
                /* L1s write back immediately */
                if (!event->isStoreConditional() || cacheLine->isAtomic()) {
                    cacheLine->setData(event->getPayload(), event);
                }
                /* Handle GetX as unlock (store-unlock) */
                if (event->queryFlag(MemEvent::F_LOCKED)) {
            	    if (!cacheLine->isLocked()) {  // Sanity check - can't unlock an already unlocked line 
                        d_->fatal(CALL_INFO, -1, "%s, Error: Unlock request to an already unlocked cache line. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                                name_.c_str(), event->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
   	            }
                    cacheLine->decLock();
                }
            } else {
                /* Handle GetSEx - Load-lock */
                cacheLine->incLock(); 
            }
            
            if (event->isStoreConditional()) sendTime = sendResponseUp(event, M, data, replay, cacheLine->getTimestamp(), cacheLine->isAtomic());
            else sendTime = sendResponseUp(event, M, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);

            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            return DONE;
         default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/** 
 * Handle a FlushLine request by writing back and forwarding request 
 */
CacheAction L1IncoherentController::handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay) {
    State state = I;
    if (cacheLine != NULL) state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);

    if (state != I && cacheLine->inTransition()) return STALL;

    if (reqEvent != NULL) return STALL;

    // If line is locked, return failure
    if (state != I && cacheLine->isLocked()) {
        sendFlushResponse(event, false, cacheLine->getTimestamp(), replay);
        return DONE;
    }
    
    forwardFlushLine(event->getBaseAddr(), FlushLine, event->getRqstr(), cacheLine);
    
    if (cacheLine != NULL && state != I) cacheLine->setState(S_B);
    else if (cacheLine != NULL) cacheLine->setState(I_B);
    return STALL;   // wait for response
}


/**
 *  Handle a FlushLineInv request by writing back/invalidating line and forwarding request if needed
 */
CacheAction L1IncoherentController::handleFlushLineInvRequest(MemEvent * event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    State state = cacheLine ? cacheLine->getState() : I;
    recordStateEventCount(event->getCmd(), state);
   
    if (state != I && cacheLine->inTransition()) return STALL;

    if (reqEvent != NULL) return STALL;

    // If line is locked, return failure
    if (state != I && cacheLine->isLocked()) {
        sendFlushResponse(event, false, cacheLine->getTimestamp(), replay);
        return DONE;
    }

    forwardFlushLine(event->getBaseAddr(), FlushLineInv, event->getRqstr(), cacheLine);
    if (cacheLine != NULL) cacheLine->setState(I_B);
    return STALL;   // wait for response
}


void L1IncoherentController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    cacheLine->setData(responseEvent->getPayload(), responseEvent);
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    
    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);

    origRequest->setMemFlags(responseEvent->getMemFlags());

    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            cacheLine->setState(E);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) break;
            if (origRequest->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(origRequest, S, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            break;
        case IM:
            cacheLine->setState(M);
            if (origRequest->getCmd() == GetX) {
                if (!origRequest->isStoreConditional() || cacheLine->isAtomic()) {
                    cacheLine->setData(origRequest->getPayload(), origRequest);

                }
                /* Handle GetX as unlock (store-unlock) */
                if (origRequest->queryFlag(MemEvent::F_LOCKED)) {
            	    if (!cacheLine->isLocked()) {  // Sanity check - can't unlock an already unlocked line 
                        d_->fatal(CALL_INFO, -1, "%s, Error: Unlock request to an already unlocked cache line. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                                name_.c_str(), origRequest->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
   	            }
                    cacheLine->decLock();
                }
            } else {
                /* Handle GetSEx - Load-lock */
                cacheLine->incLock(); 
            }
            
            if (origRequest->isStoreConditional()) sendTime = sendResponseUp(origRequest, M, cacheLine->getData(), true, cacheLine->getTimestamp(), cacheLine->isAtomic());
            else sendTime = sendResponseUp(origRequest, M, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            break;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
}

/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/
/*
 *  Return type of coherence miss
 *  0:  Hit
 *  1:  NP/I
 */
int L1IncoherentController::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
    State state = cacheLine->getState();

    if (state == I) return 1;
    return 0;
}


/*********************************************
 *  Methods for sending & receiving messages
 *********************************************/

uint64_t L1IncoherentController::sendResponseUp(MemEvent * event, State grantedState, std::vector<uint8_t>* data, bool replay, uint64_t baseTime, bool finishedAtomically) {
    Command cmd = event->getCmd();
    MemEvent * responseEvent = event->makeResponse(grantedState);
    responseEvent->setDst(event->getSrc());
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    
    if (!noncacheable) {
        /* Only return the desire word */
        Addr base    = (event->getAddr()) & ~(((Addr)lineSize_) - 1);
        Addr offset  = event->getAddr() - base;
        if (cmd != GetX) {
            responseEvent->setPayload(event->getSize(), &data->at(offset));
        } else {
            /* If write (GetX) and LLSC set, then check if operation was Atomic */
  	    if (finishedAtomically) responseEvent->setAtomic(true);
            else responseEvent->setAtomic(false);
            responseEvent->setSize(event->getSize()); // Return size that was written
        }
    } else {
        responseEvent->setPayload(*data);
    }
    // Debugging
    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, true, packetHeaderBytes_ + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s, Size = %i, Granted State = %s\n", 
            deliveryTime, timestamp_, event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), StateString[responseEvent->getGrantedState()]);
#endif
    return deliveryTime;
}


/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void L1IncoherentController::sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr){
    MemEvent* writeback = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(cacheLine->getBaseAddr()));
    writeback->setSize(cacheLine->getSize());
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setPayload(*cacheLine->getData());
    }
    writeback->setRqstr(origRqstr);
    if (cacheLine->getState() == M) writeback->setDirty(true);
    
    
    uint64 deliveryTime = timestamp_ + accessLatency_;
    Response resp = {writeback, deliveryTime, false, packetHeaderBytes_ + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s\n", deliveryTime, CommandString[cmd]);
#endif
}


void L1IncoherentController::forwardFlushLine(Addr baseAddr, Command cmd, string origRqstr, CacheLine * cacheLine) {
    MemEvent * flush = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, cmd);
    flush->setDst(getDestination(baseAddr));
    flush->setRqstr(origRqstr);
    flush->setSize(lineSize_);
    uint64_t latency = tagLatency_;
    bool payload = false;
    // Writeback data to simplify race handling unless we don't have it
    if (cacheLine != NULL) {
        if (cacheLine->getState() == M) flush->setDirty(true);
        flush->setPayload(*cacheLine->getData());
        latency = accessLatency_;
        payload = true;
    }
    uint64_t baseTime = timestamp_;
    if (cacheLine != NULL && cacheLine->getTimestamp() > baseTime) baseTime = cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, false, packetHeaderBytes_ + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (cacheLine != NULL) cacheLine->setTimestamp(deliveryTime-1);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) { 
        d_->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", deliveryTime, CommandString[flush->getCmd()], flush->getSrc().c_str(), payload ? "with data" : "without data");
    }
#endif
}


void L1IncoherentController::sendFlushResponse(MemEvent * requestEvent, bool success, uint64_t baseTime, bool replay) {
    MemEvent * flushResponse = requestEvent->makeResponse();
    flushResponse->setSuccess(success);
    flushResponse->setDst(requestEvent->getSrc());
    flushResponse->setRqstr(requestEvent->getRqstr());
    
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : tagLatency_);
    Response resp = {flushResponse, deliveryTime, true, packetHeaderBytes_ + flushResponse->getPayloadSize()};
    addToOutgoingQueueUp(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == requestEvent->getBaseAddr()) { 
        d_->debug(_L3_,"Sending Flush Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[flushResponse->getCmd()], flushResponse->getSrc().c_str());
    }
#endif
}


/********************
 * Helper functions
 ********************/

void L1IncoherentController::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());
    
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n"); */
}


