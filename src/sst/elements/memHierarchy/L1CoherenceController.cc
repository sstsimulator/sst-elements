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
#include "L1CoherenceController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Coherence Controller 
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
  
CacheAction L1CoherenceController::handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
   
    /* L1 specific code */
    if (wbCacheLine->isLocked()) {
        stat_eventStalledForLock->addData(1);
        wbCacheLine->setEventsWaitingForLock(true);
        return STALL;
    }
    recordEvictionState(state);
    switch(state) {
        case I:
            return DONE;
        case S:
            sendWriteback(PutS, wbCacheLine, origRqstr);
            wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
            return DONE;
        case E:
            sendWriteback(PutE, wbCacheLine, origRqstr);
	    wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
	    return DONE;
        case M:
	    sendWriteback(PutM, wbCacheLine, origRqstr);
            wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
	    return DONE;
        case IS:
        case IM:
        case SM:
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
CacheAction L1CoherenceController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay){
    
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
CacheAction L1CoherenceController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());

    return IGNORE;
}


/**
 *  Handle invalidation - Inv, FetchInv, or FetchInvX
 *  Return: whether Inv was successful (true) or we are waiting on further actions (false). L1 returns true (no sharers/owners).
 */
CacheAction L1CoherenceController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, bool replay) {
    
    if (cacheLine == NULL) {
        recordStateEventCount(event->getCmd(), I);
        if (mshr_->pendingWriteback(event->getBaseAddr())) {
#ifdef __SST_DEBUG_OUTPUT__
            d_->debug(_L8_, "Treating Inv as AckPut, not sending AckInv\n");
#endif
            mshr_->removeWriteback(event->getBaseAddr());
            return DONE;
        } else {
            return IGNORE;
        }
    }

    /* L1 specific code for gem5 integration */
    if (snoopL1Invs_) {
        MemEvent* snoop = new MemEvent((Component*)owner_, event->getAddr(), event->getBaseAddr(), Inv);
        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {snoop, deliveryTime, true};
        addToOutgoingQueueUp(resp);
    }

    cacheLine->atomicEnd();

    Command cmd = event->getCmd();
    switch (cmd) {
        case Inv: 
            return handleInv(event, cacheLine, replay);
        case Fetch:
            return handleFetch(event, cacheLine, replay);
        case FetchInv:
            return handleFetchInv(event, cacheLine, replay);
        case FetchInvX:
            return handleFetchInvX(event, cacheLine, replay);
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized invalidation: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // eliminate compiler warning
}




CacheAction L1CoherenceController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case GetSResp:
        case GetXResp:
            handleDataResponse(respEvent, cacheLine, reqEvent);
            break;
        case AckPut:
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            break;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/* Retry of access requests is needed if the request is still outstanding
 * Retry of replacement requests is needed if the lower cache can NACK them and 
 * the replacement has not already been ack'd via an Inv race
 */
bool L1CoherenceController::isRetryNeeded(MemEvent * event, CacheLine * cacheLine) {
    Command cmd = event->getCmd();
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
            return true;
        case PutS:
        case PutE:
        case PutM:
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) 
                if (!mshr_->pendingWriteback(event->getBaseAddr())) return false;   // The Put was resolved (probably raced with Inv)
            return true;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: NACKed event is unrecognized: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64"ns\n",
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return true;
}


/*------------------------------------------------------------------------------------------------
 *  Private event handlers
 *      HandleGetS
 *      HandleGetX
 *      handleDataResponse
 *      handleInv
 *      handleFetch
 *      handleFetchInv
 *      handleFetchInvX
 *
 *------------------------------------------------------------------------------------------------*/


CacheAction L1CoherenceController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay){
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == name_));
    recordStateEventCount(event->getCmd(), state);
    uint64_t sendTime = 0;
    switch (state) {
        case I:
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            cacheLine->setTimestamp(sendTime);
            return STALL;
        case S:
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            if (event->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(event, S, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
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
CacheAction L1CoherenceController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    vector<uint8_t>* data = cacheLine->getData();
    uint64_t sendTime = 0;

    recordStateEventCount(event->getCmd(), state);
    switch (state) {
        case I:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            cacheLine->setState(IM);
            cacheLine->setTimestamp(sendTime);
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), cacheLine->getTimestamp(), NULL);
            cacheLine->setState(SM);
            cacheLine->setTimestamp(sendTime);
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            if (cmd == GetX) {
                /* L1s write back immediately */
                if (!event->isStoreConditional() || cacheLine->isAtomic()) {
                    cacheLine->setData(event->getPayload(), event);
                    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
                        printData(cacheLine->getData(), true);
                    }
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
            cacheLine->setTimestamp(sendTime-1);

            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            return DONE;
         default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle a GetXResp or GetSResp from a lower level in the hierarchy
 */
void L1CoherenceController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    
    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
            if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
                printData(cacheLine->getData(), true);
            }
            if (responseEvent->getGrantedState() == E) cacheLine->setState(E);
            else if (responseEvent->getGrantedState() == M) cacheLine->setState(M);
            else cacheLine->setState(S);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) break;
            if (origRequest->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(origRequest, S, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
            break;
        case IM:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
            if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
                printData(cacheLine->getData(), true);
            }
        case SM:
            cacheLine->setState(M);
            if (origRequest->getCmd() == GetX) {
                if (!origRequest->isStoreConditional() || cacheLine->isAtomic()) {
                    cacheLine->setData(origRequest->getPayload(), origRequest);
                    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
                        printData(cacheLine->getData(), true);
                    }
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
            cacheLine->setTimestamp(sendTime-1);
            
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            break;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
}

CacheAction L1CoherenceController::handleInv(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);

    switch(state) {
        case I:
        case IS:
        case IM:
            return IGNORE;  // Our Put raced with the invalidation and will serve as the AckInv when it arrives
        case S:
            sendAckInv(event->getBaseAddr(), event->getRqstr(), cacheLine);
            cacheLine->setState(I);
            return DONE;
        case SM:
            sendAckInv(event->getBaseAddr(), event->getRqstr(), cacheLine);
            cacheLine->setState(IM);
            return DONE;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an invalidation in an unhandled state: %s. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;
}



CacheAction L1CoherenceController::handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();

    if (cacheLine->isLocked()) {
        stat_eventStalledForLock->addData(1);
        cacheLine->setEventsWaitingForLock(true); 
        return STALL;
    }
    
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case SM:
            sendResponseDown(event, cacheLine, replay);
            cacheLine->setState(IM);
            return DONE;
        case S:
        case E:
        case M:
            sendResponseDown(event, cacheLine, replay);
            cacheLine->setState(I);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());    

    }
    return STALL;
}

CacheAction L1CoherenceController::handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    
    if (cacheLine->isLocked()) {
        stat_eventStalledForLock->addData(1);
        cacheLine->setEventsWaitingForLock(true); 
        return STALL;
    }
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
        case SM:
            return IGNORE;
        case E:
        case M:
            sendResponseDown(event, cacheLine, replay);
            cacheLine->setState(S);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;
}


CacheAction L1CoherenceController::handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    
    if (cacheLine->isLocked()) {
        stat_eventStalledForLock->addData(1);
        cacheLine->setEventsWaitingForLock(true); 
        return STALL;
    }
    
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case SM:
        case S:
            sendResponseDown(event, cacheLine, replay);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;
}



/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/
/*
 *  Return type of coherence miss
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request)
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition
 */
int L1CoherenceController::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
    Command cmd = event->getCmd();
    State state = cacheLine->getState();
    if (cmd == GetSEx) cmd = GetX;  // for our purposes these are equal

    if (state == I) return 1;
    if (event->isPrefetch() && event->getRqstr() == name_) return 0;
    
    switch (state) {
        case S:
            if (cmd == GetS) return 0;
            return 2;
        case E:
        case M:
            return 0;
        case IS:
        case IM:
        case SM:
            return 3;
        default:
            return 0;   // this is profiling so don't die on unhandled state
    }
}


/*********************************************
 *  Methods for sending & receiving messages
 *********************************************/

/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload or mshr access if we just got the data from elsewhere
 */
void L1CoherenceController::sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*cacheLine->getData());
    //printf("Sending response down: ");
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
        printData(cacheLine->getData(), false);
    }
    responseEvent->setSize(cacheLine->getSize());
    
    if (cacheLine->getState() == M) responseEvent->setDirty(true);

    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = replay ? baseTime + mshrLatency_ :baseTime + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, true};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);

#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) { 
        d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
#endif
}


uint64_t L1CoherenceController::sendResponseUp(MemEvent * event, State grantedState, std::vector<uint8_t>* data, bool replay, uint64_t baseTime, bool finishedAtomically) {
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
    //printf("Sending data up to core: ");
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
        printData(data, false);
    }

    // Compute latency, accounting for serialization of requests to the address
    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, true};
    addToOutgoingQueueUp(resp);
    
    // Debugging
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
void L1CoherenceController::sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr) {
    MemEvent* writeback = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(cacheLine->getBaseAddr()));
    writeback->setSize(cacheLine->getSize());
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setPayload(*cacheLine->getData());
        //printf("Sending writeback data: ");
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) {
            printData(cacheLine->getData(), false);
        }
    }

    writeback->setRqstr(origRqstr);
    if (cacheLine->getState() == M) writeback->setDirty(true);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64 deliveryTime = baseTime + accessLatency_;
    Response resp = {writeback, deliveryTime, false};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) 
        d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", deliveryTime, CommandString[cmd], ((cmd == PutM || writebackCleanBlocks_) ? "" : "out"));
#endif
}


/*
 *  Handles: sending AckInv responses
 *  Latency: cache access + tag to update coherence state
 */
void L1CoherenceController::sendAckInv(Addr baseAddr, string origRqstr, CacheLine * cacheLine) {
    MemEvent* ack = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, AckInv);
    ack->setDst(getDestination(baseAddr));
    ack->setRqstr(origRqstr);
    ack->setSize(lineSize_);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64 deliveryTime = baseTime + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
#endif
}



/********************
 * Helper functions
 ********************/

void L1CoherenceController::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());
    
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");*/
}


