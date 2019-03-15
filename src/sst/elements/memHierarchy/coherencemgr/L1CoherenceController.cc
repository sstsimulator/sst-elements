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
#include "coherencemgr/L1CoherenceController.h"

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
            if (!silentEvictClean_) sendWriteback(Command::PutS, wbCacheLine, false, origRqstr);
            if (expectWritebackAck_) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
            wbCacheLine->setState(I);
            wbCacheLine->atomicEnd(); // All bets are off if this line is LL and evicted...later SC should fail
            if (wbCacheLine->getPrefetch()) {
                statPrefetchEvict->addData(1);
                wbCacheLine->setPrefetch(false);
            }
            return DONE;
        case E:
            if (!silentEvictClean_) sendWriteback(Command::PutE, wbCacheLine, false, origRqstr);
            if (expectWritebackAck_) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
	    wbCacheLine->setState(I);
            if (wbCacheLine->getPrefetch()) {
                statPrefetchEvict->addData(1);
                wbCacheLine->setPrefetch(false);
            }
            wbCacheLine->atomicEnd();
	    return DONE;
        case M:
	    sendWriteback(Command::PutM, wbCacheLine, true, origRqstr);
            if (expectWritebackAck_) mshr_->insertWriteback(wbCacheLine->getBaseAddr());
            wbCacheLine->setState(I);
            if (wbCacheLine->getPrefetch()) {
                statPrefetchEvict->addData(1);
                wbCacheLine->setPrefetch(false);
            }
            wbCacheLine->atomicEnd();
	    return DONE;
        case IS:
        case IM:
        case SM:
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
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction L1CoherenceController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay){
    
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
 *  Handle replacement - Not relevant for L1s but required to implement 
 */
CacheAction L1CoherenceController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    if (event->getCmd() == Command::FlushLineInv) {    
            return handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
    } else if (event->getCmd() == Command::FlushLine) {
            return handleFlushLineRequest(event, cacheLine, reqEvent, replay);
    } else {
        debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return IGNORE;
}


/**
 *  Handle invalidation - Inv, FetchInv, or FetchInvX
 *  Return: whether Inv was successful (true) or we are waiting on further actions (false). L1 returns true (no sharers/owners).
 */
CacheAction L1CoherenceController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay) {
   
    // Handle case where an inv raced with a replacement -> assumes non-silent replacements
    if (cacheLine == NULL) {
        recordStateEventCount(event->getCmd(), I);
        if (mshr_->pendingWriteback(event->getBaseAddr())) {
            if (is_debug_event(event)) debug->debug(_L8_, "Treating Inv as AckPut, not sending AckInv\n");
            
            mshr_->removeWriteback(event->getBaseAddr());
            return DONE;
        } else {
            return IGNORE;
        }
    }

    /* L1 specific code for gem5 integration */
    if (snoopL1Invs_) {
        MemEvent* snoop = new MemEvent(parent, event->getAddr(), event->getBaseAddr(), Command::Inv);
        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {snoop, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);
    }
    
    /* Handle locked cache lines */
    if (cacheLine->isLocked()) {
        stat_eventStalledForLock->addData(1);
        cacheLine->setEventsWaitingForLock(true); 
        return STALL;
    }

    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::Inv: 
            cacheLine->atomicEnd();
            return handleInv(event, cacheLine, replay);
        case Command::Fetch:
            return handleFetch(event, cacheLine, replay);
        case Command::FetchInv:
            cacheLine->atomicEnd();
            return handleFetchInv(event, cacheLine, replay);
        case Command::FetchInvX:
            return handleFetchInvX(event, cacheLine, replay);
        case Command::ForceInv:
            cacheLine->atomicEnd();
            return handleForceInv(event, cacheLine, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized invalidation: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // eliminate compiler warning
}




CacheAction L1CoherenceController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            handleDataResponse(respEvent, cacheLine, reqEvent);
            break;
        case Command::AckPut:
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            break;
        case Command::FlushLineResp:
            recordStateEventCount(respEvent->getCmd(), cacheLine ? cacheLine->getState() : I);
            if (cacheLine && cacheLine->getState() == S_B) cacheLine->setState(S);
            else if (cacheLine && cacheLine->getState() == I_B) {
                cacheLine->setState(I);
                cacheLine->atomicEnd();
            }
            sendFlushResponse(reqEvent, respEvent->success(), timestamp_, true);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), getCurrentSimTimeNano());
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
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLine:
        case Command::FlushLineInv:
            return true;
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
            if (expectWritebackAck_ && !mshr_->pendingWriteback(event->getBaseAddr()))
                return false;   // The Put was resolved (probably raced with Inv)
            return true;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: NACKed event is unrecognized: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64"ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
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
    
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == parent->getName());
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
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);  // This prefetch was an immediate hit
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            if (event->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns.\n",
                    parent->getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
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
    
    /* Special case - if this is the last coherence level (e.g., just mem below), 
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        cacheLine->setState(M);
    }

    bool atomic = cacheLine->isAtomic();

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
            if (cacheLine->getPrefetch()) {
                statPrefetchUpgradeMiss->addData(1);
                cacheLine->setPrefetch(false);
            }
            cacheLine->setTimestamp(sendTime);
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            if (cmd == Command::GetX) {
                /* L1s write back immediately */
                if (!event->isStoreConditional() || atomic) {
                    cacheLine->setData(event->getPayload(), event->getAddr() - event->getBaseAddr());
                    
                    if (is_debug_addr(cacheLine->getBaseAddr())) {
                        printData(cacheLine->getData(), true);
                    }
                    
                    cacheLine->atomicEnd(); // Any write (SC or regular) invalidates the LLSC link
                }
                /* Handle GetX as unlock (store-unlock) */
                if (event->queryFlag(MemEvent::F_LOCKED)) {
            	    if (!cacheLine->isLocked()) {  // Sanity check - can't unlock an already unlocked line 
                        debug->fatal(CALL_INFO, -1, "%s, Error: Unlock request to an already unlocked cache line. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                                parent->getName().c_str(), event->getBaseAddr(), getCurrentSimTimeNano());
   	            }
                    cacheLine->decLock();
                }
            } else {
                /* Handle GetSX - Load-lock */
                cacheLine->incLock(); 
            }
            
            /* Send response */
            if (event->isStoreConditional()) {
                sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp(), atomic);
            } else {
                sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            }

            cacheLine->setTimestamp(sendTime-1);

            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            return DONE;
         default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/** 
 * Handle a FlushLine request by writing back and forwarding request 
 */
CacheAction L1CoherenceController::handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay) {
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
    
    forwardFlushLine(event->getBaseAddr(), Command::FlushLine, event->getRqstr(), cacheLine);
    
    if (cacheLine != NULL && state != I) cacheLine->setState(S_B);
    else if (cacheLine != NULL) cacheLine->setState(I_B);
    return STALL;   // wait for response
}


/**
 *  Handle a FlushLineInv request by writing back/invalidating line and forwarding request if needed
 */
CacheAction L1CoherenceController::handleFlushLineInvRequest(MemEvent * event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    State state = cacheLine ? cacheLine->getState() : I;
    recordStateEventCount(event->getCmd(), state);
   
    if (state != I && cacheLine->inTransition()) return STALL;

    if (reqEvent != NULL) return STALL;

    // If line is locked, return failure
    if (state != I && cacheLine->isLocked()) {
        sendFlushResponse(event, false, cacheLine->getTimestamp(), replay);
        return DONE;
    }
    
    if (cacheLine && cacheLine->getPrefetch()) {
        statPrefetchEvict->addData(1);
        cacheLine->setPrefetch(false);
    }

    forwardFlushLine(event->getBaseAddr(), Command::FlushLineInv, event->getRqstr(), cacheLine);
    if (cacheLine != NULL) cacheLine->setState(I_B);
    return STALL;   // wait for response
}


/**
 *  Handle a GetXResp or GetSResp from a lower level in the hierarchy
 */
void L1CoherenceController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == parent->getName());
    
    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    
    // Copy memflags through to processor
    origRequest->setMemFlags(responseEvent->getMemFlags());
    
    uint64_t sendTime = 0;
    bool atomic = cacheLine->isAtomic();

    switch (state) {
        case IS:
            cacheLine->setData(responseEvent->getPayload(), 0);
            
            if (is_debug_addr(cacheLine->getBaseAddr())) {
                printData(cacheLine->getData(), true);
            }
            
            if (responseEvent->getCmd() == Command::GetXResp && responseEvent->getDirty()) cacheLine->setState(M);
            else if (protocol_ && responseEvent->getCmd() == Command::GetXResp) cacheLine->setState(E);
            else cacheLine->setState(S);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                cacheLine->setPrefetch(true);
                break;
            }
            if (origRequest->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
            break;
        case IM:
            cacheLine->setData(responseEvent->getPayload(), 0);
            
            if (is_debug_addr(cacheLine->getBaseAddr())) {
                printData(cacheLine->getData(), true);
            }
        
        case SM:
            cacheLine->setState(M);
            if (origRequest->getCmd() == Command::GetX) {
                if (!origRequest->isStoreConditional() ||atomic) {
                    cacheLine->setData(origRequest->getPayload(), origRequest->getAddr() - origRequest->getBaseAddr());
                    
                    if (is_debug_addr(cacheLine->getBaseAddr())) {
                        printData(cacheLine->getData(), true);
                    }

                    cacheLine->atomicEnd(); // No longer atomic after this write
                }
                /* Handle GetX as unlock (store-unlock) */
                if (origRequest->queryFlag(MemEvent::F_LOCKED)) {
            	    if (!cacheLine->isLocked()) {  // Sanity check - can't unlock an already unlocked line 
                        debug->fatal(CALL_INFO, -1, "%s, Error: Unlock request to an already unlocked cache line. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                                parent->getName().c_str(), origRequest->getBaseAddr(), getCurrentSimTimeNano());
   	            }
                    cacheLine->decLock();
                }
            } else {
                /* Handle GetSX - Load-lock */
                cacheLine->incLock(); 
            }
            
            if (origRequest->isStoreConditional()) sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp(), atomic);
            else sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
            
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
}

CacheAction L1CoherenceController::handleInv(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);

    if (cacheLine->getPrefetch()) {
        statPrefetchInv->addData(1);
        cacheLine->setPrefetch(false);
    }

    switch(state) {
        case I:
        case IS:
        case IM:
        case I_B:
            return IGNORE;  // Our Put/flush raced with the invalidation and will serve as the AckInv when it arrives
        case S:
            sendAckInv(event, cacheLine);
            cacheLine->setState(I);
            return DONE;
        case SM:
            sendAckInv(event, cacheLine);
            cacheLine->setState(IM);
            return DONE;
        case S_B:
            sendAckInv(event, cacheLine);
            cacheLine->setState(I_B);
            return IGNORE;  // don't retry waiting flush
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an invalidation in an unhandled state: %s. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL;
}


CacheAction L1CoherenceController::handleForceInv(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);

    if (cacheLine->getPrefetch()) {
        statPrefetchInv->addData(1);
        cacheLine->setPrefetch(false);
    }

    switch(state) {
        case I:
        case IS:
        case IM:
        case I_B:
            return IGNORE; // Assume Put/flush raced with invalidation
        case S:
        case E:
        case M:
            sendAckInv(event, cacheLine);
            cacheLine->setState(I);
            return DONE;
        case SM:
            sendAckInv(event, cacheLine);
            cacheLine->setState(IM);
            return DONE;
        case S_B:
            sendAckInv(event, cacheLine);
            cacheLine->setState(I_B);
            return IGNORE; // don't retry waiting flush
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received a ForceInv in an unhandled state: %s. Addr = 0x%" PRIu64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL;
}


CacheAction L1CoherenceController::handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);
    
    if (cacheLine->getPrefetch()) {
        statPrefetchInv->addData(1);
        cacheLine->setPrefetch(false);
    }

    switch (state) {
        case I:
        case IS:
        case IM:
        case I_B:
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
        case S_B:
            sendAckInv(event, cacheLine);
            cacheLine->setState(I_B);
            return IGNORE; // don't retry waiting flush
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());    

    }
    return STALL;
}

CacheAction L1CoherenceController::handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
        case SM:
        case I_B:
        case S_B:
            return IGNORE;
        case E:
        case M:
            sendResponseDown(event, cacheLine, replay);
            cacheLine->setState(S);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL;
}


CacheAction L1CoherenceController::handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
        case I_B:
        case S_B:
            return IGNORE;
        case SM:
        case S:
            sendResponseDown(event, cacheLine, replay);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: Received Fetch but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
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
    if (cmd == Command::GetSX) cmd = Command::GetX;  // for our purposes these are equal

    if (state == I) return 1;
    if (event->isPrefetch() && event->getRqstr() == parent->getName()) return 0;
    
    switch (state) {
        case S:
            if (cmd == Command::GetS || lastLevel_) return 0;
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

    if (is_debug_addr(cacheLine->getBaseAddr())) {
        printData(cacheLine->getData(), false);
    }

    responseEvent->setSize(cacheLine->getSize());
    
    if (cacheLine->getState() == M) responseEvent->setDirty(true);

    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = replay ? baseTime + mshrLatency_ :baseTime + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);

    if (is_debug_event(event)) { 
        debug->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}


uint64_t L1CoherenceController::sendResponseUp(MemEvent * event, std::vector<uint8_t>* data, bool replay, uint64_t baseTime, bool finishedAtomically) {
    Command cmd = event->getCmd();
    MemEvent * responseEvent = event->makeResponse();
    responseEvent->setDst(event->getSrc());
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
     
    if (!noncacheable) {
        /* Only return the desire word */
        Addr offset  = event->getAddr() - event->getBaseAddr();
        if (cmd != Command::GetX) {
            responseEvent->setPayload(event->getSize(), &data->at(offset));
        } else {
            /* If write (GetX) and LLSC set, then check if operation was Atomic */
  	    if (finishedAtomically) responseEvent->setSuccess(true);
            else responseEvent->setSuccess(false);
            responseEvent->setSize(event->getSize()); // Return size that was written
        }
    } else {
        responseEvent->setPayload(*data);
    }
    
    if (is_debug_event(event)) {
        printData(data, false);
    }

    // Compute latency, accounting for serialization of requests to the address
    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueueUp(resp);
    
    // Debugging
    if (is_debug_event(responseEvent)) debug->debug(_L3_,"Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Response: (%s)\n", 
            deliveryTime, timestamp_, responseEvent->getBriefString().c_str());
    
    return deliveryTime;
}


/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void L1CoherenceController::sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string origRqstr) {
    MemEvent* writeback = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(cacheLine->getBaseAddr()));
    writeback->setSize(cacheLine->getSize());
    uint64_t latency = tagLatency_;
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*cacheLine->getData());
        
        //printf("Sending writeback data: ");
        if (is_debug_addr(cacheLine->getBaseAddr())) {
            printData(cacheLine->getData(), false);
        }
        
        latency = accessLatency_;
    }
        
    writeback->setDirty(dirty);

    writeback->setRqstr(origRqstr);
    if (cacheLine->getState() == M) writeback->setDirty(true);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) 
        debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", deliveryTime, CommandString[(int)cmd], ((cmd == Command::PutM || writebackCleanBlocks_) ? "" : "out"));
}


/*
 *  Handles: sending AckInv responses
 *  Latency: cache access + tag to update coherence state
 */
void L1CoherenceController::sendAckInv(MemEvent * request, CacheLine * cacheLine) {
    MemEvent* ack = request->makeResponse();
    ack->setCmd(Command::AckInv);
    ack->setDst(getDestination(ack->getBaseAddr()));
    ack->setSize(lineSize_);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + tagLatency_;
    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_event(request)) debug->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
}


void L1CoherenceController::forwardFlushLine(Addr baseAddr, Command cmd, string origRqstr, CacheLine * cacheLine) {
    MemEvent * flush = new MemEvent(parent, baseAddr, baseAddr, cmd);
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
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (cacheLine != NULL) cacheLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_addr(baseAddr)) {
        debug->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", deliveryTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str(), payload ? "with data" : "without data");
    }
}


void L1CoherenceController::sendFlushResponse(MemEvent * requestEvent, bool success, uint64_t baseTime, bool replay) {
    MemEvent * flushResponse = requestEvent->makeResponse();
    flushResponse->setSuccess(success);
    flushResponse->setDst(requestEvent->getSrc());
    flushResponse->setRqstr(requestEvent->getRqstr());
    
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : tagLatency_);
    Response resp = {flushResponse, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    
    if (is_debug_event(requestEvent)) {
        debug->debug(_L3_,"Sending Flush Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)flushResponse->getCmd()], flushResponse->getSrc().c_str());
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void L1CoherenceController::addToOutgoingQueue(Response& resp) {
    CoherenceController::addToOutgoingQueue(resp);
    recordEventSentDown(resp.event->getCmd());
}

void L1CoherenceController::addToOutgoingQueueUp(Response& resp) {
    CoherenceController::addToOutgoingQueueUp(resp);
    recordEventSentUp(resp.event->getCmd());
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

    
/***********************
 * Statistics functions
 ***********************/
void L1CoherenceController::recordEvictionState(State state) {
    switch(state) {
        case I:
            stat_evict_I->addData(1);
            break;
        case S:
            stat_evict_S->addData(1);
            break;
        case E:
            stat_evict_E->addData(1);
            break;
        case M:
            stat_evict_M->addData(1);
            break;
        case IS:
            stat_evict_IS->addData(1);
            break;
        case IM:
            stat_evict_IM->addData(1);
            break;
        case SM:
            stat_evict_SM->addData(1);
            break;
        case I_B:
            stat_evict_IB->addData(1);
            break;
        case S_B:
            stat_evict_SB->addData(1);
            break;
        default:
            break;
    }
}


void L1CoherenceController::recordStateEventCount(Command cmd, State state) {
    switch(cmd) {
        case Command::GetS:
            if (state == I) stat_stateEvent_GetS_I->addData(1);
            else if (state == S) stat_stateEvent_GetS_S->addData(1);
            else if (state == E) stat_stateEvent_GetS_E->addData(1);
            else if (state == M) stat_stateEvent_GetS_M->addData(1);
            break;
        case Command::GetX:
            if (state == I) stat_stateEvent_GetX_I->addData(1);
            else if (state == S) stat_stateEvent_GetX_S->addData(1);
            else if (state == E) stat_stateEvent_GetX_E->addData(1);
            else if (state == M) stat_stateEvent_GetX_M->addData(1);
            break;
        case Command::GetSX:
            if (state == I) stat_stateEvent_GetSX_I->addData(1);
            else if (state == S) stat_stateEvent_GetSX_S->addData(1);
            else if (state == E) stat_stateEvent_GetSX_E->addData(1);
            else if (state == M) stat_stateEvent_GetSX_M->addData(1);
            break;
        case Command::GetSResp:
            if (state == IS) stat_stateEvent_GetSResp_IS->addData(1);
            break;
        case Command::GetXResp:
            if (state == IS) stat_stateEvent_GetXResp_IS->addData(1);
            else if (state == IM) stat_stateEvent_GetXResp_IM->addData(1);
            else if (state == SM) stat_stateEvent_GetXResp_SM->addData(1);
            break;
        case Command::Inv:
            if (state == I) stat_stateEvent_Inv_I->addData(1);
            else if (state == S) stat_stateEvent_Inv_S->addData(1);
            else if (state == IS) stat_stateEvent_Inv_IS->addData(1);
            else if (state == IM) stat_stateEvent_Inv_IM->addData(1);
            else if (state == SM) stat_stateEvent_Inv_SM->addData(1);
            else if (state == S_B) stat_stateEvent_Inv_SB->addData(1);
            else if (state == I_B) stat_stateEvent_Inv_IB->addData(1);
            break;
        case Command::FetchInvX:
            if (state == I) stat_stateEvent_FetchInvX_I->addData(1);
            else if (state == E) stat_stateEvent_FetchInvX_E->addData(1);
            else if (state == M) stat_stateEvent_FetchInvX_M->addData(1);
            else if (state == IS) stat_stateEvent_FetchInvX_IS->addData(1);
            else if (state == IM) stat_stateEvent_FetchInvX_IM->addData(1);
            else if (state == S_B) stat_stateEvent_FetchInvX_SB->addData(1);
            else if (state == I_B) stat_stateEvent_FetchInvX_IB->addData(1);
            break;
        case Command::Fetch:
            if (state == I) stat_stateEvent_Fetch_I->addData(1);
            else if (state == S) stat_stateEvent_Fetch_S->addData(1);
            else if (state == IS) stat_stateEvent_Fetch_IS->addData(1);
            else if (state == IM) stat_stateEvent_Fetch_IM->addData(1);
            else if (state == SM) stat_stateEvent_Fetch_SM->addData(1);
            else if (state == I_B) stat_stateEvent_Fetch_IB->addData(1);
            else if (state == S_B) stat_stateEvent_Fetch_SB->addData(1);
            break;
        case Command::FetchInv:
            if (state == I) stat_stateEvent_FetchInv_I->addData(1);
            else if (state == S) stat_stateEvent_FetchInv_S->addData(1);
            else if (state == E) stat_stateEvent_FetchInv_E->addData(1);
            else if (state == M) stat_stateEvent_FetchInv_M->addData(1);
            else if (state == IS) stat_stateEvent_FetchInv_IS->addData(1);
            else if (state == IM) stat_stateEvent_FetchInv_IM->addData(1);
            else if (state == SM) stat_stateEvent_FetchInv_SM->addData(1);
            else if (state == S_B) stat_stateEvent_FetchInv_SB->addData(1);
            else if (state == I_B) stat_stateEvent_FetchInv_IB->addData(1);
            break;
        case Command::AckPut:
            if (state == I) stat_stateEvent_AckPut_I->addData(1);
            break;
        case Command::FlushLine:
            if (state == I) stat_stateEvent_FlushLine_I->addData(1);
            else if (state == S) stat_stateEvent_FlushLine_S->addData(1);
            else if (state == E) stat_stateEvent_FlushLine_E->addData(1);
            else if (state == M) stat_stateEvent_FlushLine_M->addData(1);
            else if (state == IS) stat_stateEvent_FlushLine_IS->addData(1);
            else if (state == IM) stat_stateEvent_FlushLine_IM->addData(1);
            else if (state == SM) stat_stateEvent_FlushLine_SM->addData(1);
            else if (state == I_B) stat_stateEvent_FlushLine_IB->addData(1);
            else if (state == S_B) stat_stateEvent_FlushLine_SB->addData(1);
            break;
        case Command::FlushLineInv:
            if (state == I) stat_stateEvent_FlushLineInv_I->addData(1);
            else if (state == S) stat_stateEvent_FlushLineInv_S->addData(1);
            else if (state == E) stat_stateEvent_FlushLineInv_E->addData(1);
            else if (state == M) stat_stateEvent_FlushLineInv_M->addData(1);
            else if (state == IS) stat_stateEvent_FlushLineInv_IS->addData(1);
            else if (state == IM) stat_stateEvent_FlushLineInv_IM->addData(1);
            else if (state == SM) stat_stateEvent_FlushLineInv_SM->addData(1);
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


/* Record how many times each event type was sent down */
void L1CoherenceController::recordEventSentDown(Command cmd) {
    switch (cmd) {
        case Command::GetS:
            stat_eventSent_GetS->addData(1);
            break;
        case Command::GetX:
            stat_eventSent_GetX->addData(1);
            break;
        case Command::GetSX:
            stat_eventSent_GetSX->addData(1);
            break;
        case Command::PutS:
            stat_eventSent_PutS->addData(1);
            break;
        case Command::PutE:
            stat_eventSent_PutE->addData(1);
            break;
        case Command::PutM:
            stat_eventSent_PutM->addData(1);
            break;
        case Command::NACK:
            stat_eventSent_NACK_down->addData(1);
            break;
        case Command::FlushLine:
            stat_eventSent_FlushLine->addData(1);
            break;
        case Command::FlushLineInv:
            stat_eventSent_FlushLineInv->addData(1);
            break;
        case Command::FetchResp:
            stat_eventSent_FetchResp->addData(1);
            break;
        case Command::FetchXResp:
            stat_eventSent_FetchXResp->addData(1);
            break;
        case Command::AckInv:
            stat_eventSent_AckInv->addData(1);
            break;
        default:
            break;
    }
}


void L1CoherenceController::recordEventSentUp(Command cmd) {
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
        case Command::Inv:
            stat_eventSent_Inv->addData(1);
            break;
        default:
            break;
    }
}
