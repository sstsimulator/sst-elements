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
                    ownerName_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction L1CoherenceController::handleRequest(MemEvent* event, bool replay) {
    Addr addr = event->getBaseAddr();
    
    CacheLine * cacheLine = cacheArray_->lookup(addr, !replay); 
    
    if (!cacheLine && (is_debug_addr(addr))) debug->debug(_L3_, "-- Miss --\n");

    if (cacheLine && cacheLine->inTransition()) {
        allocateMSHR(addr, event);
        return STALL;
    }

    if (!cacheLine) {
        if (!allocateLine(addr, event)) {
            allocateMSHR(addr, event);
            recordMiss(event->getID());
            return STALL;
        }
        cacheLine = cacheArray_->lookup(addr, false);
    }
    
    Command cmd = event->getCmd();

    switch(cmd) {
        case Command::GetS:
            return handleGetSRequest(event, cacheLine, replay);
        case Command::GetX:
        case Command::GetSX:
            return handleGetXRequest(event, cacheLine, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), CommandString[(int)cmd], addr, event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}



/**
 *  Handle replacement - Not relevant for L1s but required to implement 
 */
CacheAction L1CoherenceController::handleReplacement(MemEvent* event, bool replay) {
    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            ownerName_.c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());

    return IGNORE;
}
CacheAction L1CoherenceController::handleFlush(MemEvent* event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay) {
    CacheAction action = IGNORE;
    if (event->getCmd() == Command::FlushLineInv) {    
        action = handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
    } else if (event->getCmd() == Command::FlushLine) {
        action = handleFlushLineRequest(event, cacheLine, reqEvent, replay);
    } else {
        debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                ownerName_.c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    return action;
}


/**
 *  Handle invalidation - Inv, FetchInv, or FetchInvX
 *  Return: whether Inv was successful (true) or we are waiting on further actions (false). L1 returns true (no sharers/owners).
 */
CacheAction L1CoherenceController::handleInvalidationRequest(MemEvent * event, bool replay) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* cacheLine = cacheArray_->lookup(bAddr, false);

    if (is_debug_addr(bAddr))
        printLine(bAddr);

    // Handle case where an inv raced with a replacement -> assumes non-silent replacements
    if (cacheLine == NULL) {
        recordStateEventCount(event->getCmd(), I);
        if (mshr_->pendingWriteback(bAddr)) {
            if (is_debug_event(event)) debug->debug(_L8_, "Treating Inv as AckPut, not sending AckInv\n");
            
            mshr_->removeWriteback(bAddr);
            return DONE;
        } else {
            return IGNORE;
        }
    }

    /* L1 specific code for gem5 integration */
    if (snoopL1Invs_) {
        MemEvent* snoop = new MemEvent(ownerName_, event->getAddr(), bAddr, Command::Inv);
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
    CacheAction action = STALL;
    switch (cmd) {
        case Command::Inv: 
            cacheLine->atomicEnd();
            action = handleInv(event, cacheLine, replay);
            break;
        case Command::Fetch:
            action = handleFetch(event, cacheLine, replay);
            break;
        case Command::FetchInv:
            cacheLine->atomicEnd();
            action = handleFetchInv(event, cacheLine, replay);
            break;
        case Command::FetchInvX:
            action = handleFetchInvX(event, cacheLine, replay);
            break;
        case Command::ForceInv:
            cacheLine->atomicEnd();
            action = handleForceInv(event, cacheLine, replay);
            break;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized invalidation: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), CommandString[(int)cmd], bAddr, event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(bAddr))
        printLine(bAddr);

    delete event;

    return action;
}


CacheAction L1CoherenceController::handleCacheResponse(MemEvent * event, bool inMSHR) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* line = cacheArray_->lookup(bAddr, false);
    MemEvent* reqEvent = mshr_->lookupFront(bAddr);

    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            handleDataResponse(event, line, reqEvent);
            break;
        case Command::FlushLineResp:
            recordStateEventCount(event->getCmd(), line ? line->getState() : I);
            if (line && line->getState() == S_B) line->setState(S);
            else if (line && line->getState() == I_B) {
                line->setState(I);
                line->atomicEnd();
            }
            sendFlushResponse(reqEvent, event->success(), timestamp_, true);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], bAddr, event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    mshr_->removeFront(bAddr);

    if (is_debug_addr(bAddr))
        printLine(bAddr);

    delete event;
    delete reqEvent;

    return DONE;
}

CacheAction L1CoherenceController::handleFetchResponse(MemEvent * event, bool inMSHR) {
    Addr bAddr = event->getBaseAddr();
    
    CacheLine* line = nullptr;
    if (is_debug_addr(bAddr)) {
        line = cacheArray_->lookup(bAddr, false);
        printLine(bAddr);
    }

    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::AckPut:
            recordStateEventCount(event->getCmd(), I);
            mshr_->removeWriteback(bAddr);
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], bAddr, event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    delete event;
    
    if (is_debug_addr(bAddr))
        printLine(bAddr);

    return DONE;
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
    Addr addr = event->getBaseAddr();
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == ownerName_);
    recordStateEventCount(event->getCmd(), state);
    uint64_t sendTime = 0;
    switch (state) {
        case I:
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            cacheLine->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::MISS);
            allocateMSHR(addr, event); 
            return STALL;
        case S:
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);  // This prefetch was an immediate hit
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            if (event->isLoadLink()) cacheLine->atomicStart();
            sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime-1);
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns.\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL;    // eliminate compiler warning
}


/*
 *  Return: whether event was handled or is waiting for further responses
 */
CacheAction L1CoherenceController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Addr addr = event->getBaseAddr();
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
            recordLatencyType(event->getID(), LatType::MISS);
            allocateMSHR(addr, event); 
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
            recordLatencyType(event->getID(), LatType::UPGRADE);
            allocateMSHR(addr, event); 
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
                                ownerName_.c_str(), event->getBaseAddr(), getCurrentSimTimeNano());
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
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
         default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
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

    if (reqEvent != nullptr) 
        return STALL;

    // If line is locked, return failure
    if (state != I && cacheLine->isLocked()) {
        sendFlushResponse(event, false, cacheLine->getTimestamp(), replay);
        recordLatencyType(event->getID(), LatType::MISS);
        return DONE;
    }
    
    forwardFlushLine(event->getBaseAddr(), Command::FlushLine, event->getRqstr(), cacheLine);
    recordLatencyType(event->getID(), LatType::HIT);
    
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

    if (reqEvent != nullptr) 
        return STALL;

    // If line is locked, return failure
    if (state != I && cacheLine->isLocked()) {
        sendFlushResponse(event, false, cacheLine->getTimestamp(), replay);
        recordLatencyType(event->getID(), LatType::MISS);
        return DONE;
    }
    
    if (cacheLine && cacheLine->getPrefetch()) {
        statPrefetchEvict->addData(1);
        cacheLine->setPrefetch(false);
    }

    forwardFlushLine(event->getBaseAddr(), Command::FlushLineInv, event->getRqstr(), cacheLine);
    if (cacheLine != NULL) cacheLine->setState(I_B);
    recordLatencyType(event->getID(), LatType::HIT);
    return STALL;   // wait for response
}


/**
 *  Handle a GetXResp or GetSResp from a lower level in the hierarchy
 */
void L1CoherenceController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == ownerName_);
    
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
            if (localPrefetch) {
                cacheLine->setPrefetch(true);
                recordPrefetchLatency(origRequest->getID(), LatType::MISS);
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
                                ownerName_.c_str(), origRequest->getBaseAddr(), getCurrentSimTimeNano());
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
            
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
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
                    ownerName_.c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
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
                    ownerName_.c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
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
                        ownerName_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());    

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
                    ownerName_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
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
                    ownerName_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL;
}

// L1 always resends NACKed events
bool L1CoherenceController::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    CacheLine* line = cacheArray_->lookup(nackedEvent->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (is_debug_addr(nackedEvent->getBaseAddr()))
        debug->debug(_L5_, "\tNACK received for: %s\n", nackedEvent->getBriefString().c_str());
    
    bool resend = false;
    Command cmd = nackedEvent->getCmd();
    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLine:
        case Command::FlushLineInv:
            resend = true;
            break;
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
            if (expectWritebackAck_ && !mshr_->pendingWriteback(nackedEvent->getBaseAddr()))
                resend = false;   // The Put was resolved (probably raced with Inv)
            else
                resend = true;
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: NACKed event is unrecognized: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64"ns\n",
                    ownerName_.c_str(), CommandString[(int)cmd], nackedEvent->getBaseAddr(), nackedEvent->getSrc().c_str(), getCurrentSimTimeNano());
    }
    if (resend) {
        resendEvent(nackedEvent, false);
    } else {
        delete nackedEvent;
    }

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for managing data structures
 *---------------------------------------------------------------------------------------------------------------------*/
bool L1CoherenceController::allocateLine(Addr addr, MemEvent* event) {
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
void L1CoherenceController::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return; // Never set a hit/miss status
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
            stat_latencyFlushLine[type]->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latencyFlushLineInv[type]->addData(latency);
            break;
        default:
            break;
    }
}

bool L1CoherenceController::isCacheHit(MemEvent* event) {
    CacheLine* line = cacheArray_->lookup(event->getBaseAddr(), false);
    Command cmd = event->getCmd();
    State state = line ? line->getState() : I;
    if (cmd == Command::GetSX) cmd = Command::GetX;  // for our purposes these are equal

    switch (state) {
        case I:
            return false;
        case S:
            if (cmd == Command::GetS || lastLevel_) return true;
            return false;
        case E:
        case M:
            return true;
        case IS:
        case IM:
        case SM:
            return false;
        default:
            return true;
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
    MemEvent* writeback = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
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
    MemEvent * flush = new MemEvent(ownerName_, baseAddr, baseAddr, cmd);
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
    recordEventSent(resp.event->getCmd());
}

void L1CoherenceController::addToOutgoingQueueUp(Response& resp) {
    CoherenceController::addToOutgoingQueueUp(resp);
    recordEventSent(resp.event->getCmd());
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

void L1CoherenceController::printLine(Addr addr, CacheLine* line) {
    State state = (line == nullptr) ? NP : line->getState();
    debug->debug(_L8_, "0x%" PRIx64 ": %s\n", addr, StateString[state]);
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
    stat_eventState[(int)cmd][state]->addData(1);
}


/* Record how many times each event type was sent down */
void L1CoherenceController::recordEventSent(Command cmd) {
    stat_eventSent[(int)cmd]->addData(1);
}

void L1CoherenceController::printLine(Addr addr) {
    if (!is_debug_addr(addr))
        return;

    CacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : NP;
    debug->debug(_L8_, "0x%" PRIx64 ": %s\n", addr, StateString[state]);
}
