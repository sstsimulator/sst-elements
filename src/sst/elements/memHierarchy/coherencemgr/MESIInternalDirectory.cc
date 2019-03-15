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
#include "coherencemgr/MESIInternalDirectory.h"

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
 * MESI Internal Directory for Non-inclusive Caches with Multiple Children 
 * (e.g., a non-inclusive private L2 with split L1I/D above or a non-inclusive shared LLC)
 * *All* non-inclusive caches with multiple children must either use this directory OR implement snooping
 * The directory holds information about all blocks, this coherence entity handles both locally cached and uncached blocks
 *---------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------
 *  External interface functions for routing events from cache controller to appropriate coherence handlers
 *---------------------------------------------------------------------------------------------------------------------*/

/**
 *  Evict block from directory (fromDataCache=false) or from cache (fromDataCache=true)
 *  Directory evictions will also trigger a cache eviction if the block is locally cached
 *  Return whether the eviction is complete (DONE) or not (STALL)
 */
CacheAction MESIInternalDirectory::handleEviction(CacheLine* replacementLine, string origRqstr, bool fromDataCache) {
    State state = replacementLine->getState();
    
    recordEvictionState(state);
    
    Addr wbBaseAddr = replacementLine->getBaseAddr();
    bool isCached = replacementLine->getDataLine() != NULL;


    // Check if there is a stalled replacement to the block we are attempting to replace. 
    // If so, we should handle it immediately to avoid deadlocks (A waiting for B to evict, B waiting for A to handle its eviction)
    MemEvent * waitingEvent = (mshr_->isHit(wbBaseAddr)) ? mshr_->lookupFront(wbBaseAddr) : NULL;
    bool collision = (waitingEvent != NULL && (waitingEvent->getCmd() == Command::PutS || waitingEvent->getCmd() == Command::PutE || waitingEvent->getCmd() == Command::PutM));
    if (collision) {    // Note that 'collision' and 'fromDataCache' cannot both be true, don't need to handle that case
        if (state == E && waitingEvent->getDirty()) replacementLine->setState(M);
        if (replacementLine->isSharer(waitingEvent->getSrc())) replacementLine->removeSharer(waitingEvent->getSrc());
        else if (replacementLine->ownerExists()) replacementLine->clearOwner();
        mshr_->setDataBuffer(waitingEvent->getBaseAddr(), waitingEvent->getPayload());
        mshr_->removeFront(waitingEvent->getBaseAddr());
        delete waitingEvent;
    }
    switch (state) {
        case I:
            return DONE;
        case S:
            if (replacementLine->getPrefetch()) {
                replacementLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (replacementLine->numSharers() > 0 && !fromDataCache) {
                if (isCached || collision) invalidateAllSharers(replacementLine, parent->getName(), false);
                else invalidateAllSharersAndFetch(replacementLine, parent->getName(), false);    // Fetch needed for PutS
                replacementLine->setState(SI);
                return STALL;
            }
            if (!isCached && !collision) debug->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers. Addr = 0x%" PRIx64 ", State = %s\n", parent->getName().c_str(), replacementLine->getBaseAddr(), StateString[state]);
            if (fromDataCache && replacementLine->numSharers() > 0) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
            if (isCached) sendWritebackFromCache(Command::PutS, replacementLine, origRqstr);
            else sendWritebackFromMSHR(Command::PutS, replacementLine, origRqstr, mshr_->getDataBuffer(wbBaseAddr));
            if (replacementLine->numSharers() == 0) replacementLine->setState(I); 
            if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
            return DONE;
        case E:
            if (replacementLine->getPrefetch()) {
                replacementLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (replacementLine->numSharers() > 0 && !fromDataCache) { // May or may not be cached
                if (isCached || collision) invalidateAllSharers(replacementLine, parent->getName(), false);
                else invalidateAllSharersAndFetch(replacementLine, parent->getName(), false);
                replacementLine->setState(EI);
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) { // Not cached
                sendFetchInv(replacementLine, parent->getName(), false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(EI);
                return STALL;
            } else { // Must be cached
                if (!isCached && !collision) 
                    debug->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", parent->getName().c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(Command::PutE, replacementLine, origRqstr);
                else sendWritebackFromMSHR(Command::PutE, replacementLine, origRqstr, mshr_->getDataBuffer(wbBaseAddr));
                if (replacementLine->numSharers() == 0 && !replacementLine->ownerExists()) replacementLine->setState(I);
                if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
                return DONE;
            }
        case M:
            if (replacementLine->getPrefetch()) {
                replacementLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (replacementLine->numSharers() > 0 && !fromDataCache) {
                if (isCached || collision) invalidateAllSharers(replacementLine, parent->getName(), false);
                else invalidateAllSharersAndFetch(replacementLine, parent->getName(), false);
                replacementLine->setState(MI);
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) {
                sendFetchInv(replacementLine, parent->getName(), false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(MI);
                return STALL;
            } else {
                if (!isCached && !collision) 
                    debug->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", parent->getName().c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(Command::PutM, replacementLine, origRqstr);
                else sendWritebackFromMSHR(Command::PutM, replacementLine, origRqstr, mshr_->getDataBuffer(wbBaseAddr));
                if (replacementLine->numSharers() == 0 && !replacementLine->ownerExists()) replacementLine->setState(I);
                if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
                return DONE;
            }
        case SI:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
        case E_InvX:
        case S_B:
        case I_B:
            return STALL;
        default:
	    debug->fatal(CALL_INFO,-1,"%s (dir), Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), StateString[state], replacementLine->getBaseAddr(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/** Handle data requests */
CacheAction MESIInternalDirectory::handleRequest(MemEvent * event, CacheLine * dirLine, bool replay) {
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::GetS:
            return handleGetSRequest(event, dirLine, replay);
        case Command::GetX:
        case Command::GetSX:
            return handleGetXRequest(event, dirLine, replay);
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Errror: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning

}


/**
 *  Handle replacement (Put*) requests
 */
CacheAction MESIInternalDirectory::handleReplacement(MemEvent* event, CacheLine* dirLine, MemEvent * reqEvent, bool replay) {
    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::PutS:
            return handlePutSRequest(event, dirLine, reqEvent);
        case Command::PutE:
        case Command::PutM:
            return handlePutMRequest(event, dirLine, reqEvent);
        case Command::FlushLineInv:
            return handleFlushLineInvRequest(event, dirLine, reqEvent, replay);
        case Command::FlushLine:
            return handleFlushLineRequest(event, dirLine, reqEvent, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received an unrecognized replacement: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    
    return DONE;    // Eliminate compiler warning
}


/**
 *  Handle invalidations (Inv, FetchInv, FetchInvX) by sending to the appropriate handler
 *  Return whether the invalidation completed (DONE), completed but a pending request in the MSHR should not be 
 *  replayed immediately (IGNORE), is on-going (STALL), or must wait for an existing request to complete (BLOCK)
 *
 *  Special case for when an inv races with a replacement -> 
 *  treat Inv request as the AckPut (the Put request will likewise be treated as the AckInv/FetchResp)
 */
CacheAction MESIInternalDirectory::handleInvalidationRequest(MemEvent * event, CacheLine * dirLine, MemEvent * collisionEvent, bool replay) {
    

    bool collision = false;
    if (mshr_->pendingWriteback(event->getBaseAddr())) {
        // Case 1: Inv raced with a Put -> treat Inv as the AckPut
        mshr_->removeWriteback(event->getBaseAddr());
        return DONE;
    } else {
        collision = (collisionEvent != NULL && (collisionEvent->getCmd() == Command::PutS || collisionEvent->getCmd() == Command::PutE || collisionEvent->getCmd() == Command::PutM));
    }

    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::Inv: 
            return handleInv(event, dirLine, replay, collisionEvent);
        case Command::Fetch:
            return handleFetch(event, dirLine, replay, collision ? collisionEvent : NULL);
        case Command::FetchInv:
            return handleFetchInv(event, dirLine, replay, collisionEvent);
        case Command::FetchInvX:
            return handleFetchInvX(event, dirLine, replay, collisionEvent);
        case Command::ForceInv:
            return handleForceInv(event, dirLine, replay, collisionEvent);
        default:
	    debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received an unrecognized invalidation: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // eliminate compiler warning
}


/**
 *  Handle responses to outstanding requests by sending to the appropriate handler
 *  Return whether the request completed as a result of this response (DONE),
 *  whether it should continue waiting for more responses (STALL or IGNORE)
 *
 */
CacheAction MESIInternalDirectory::handleResponse(MemEvent * respEvent, CacheLine * dirLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            return handleDataResponse(respEvent, dirLine, reqEvent);
        case Command::FetchResp:
        case Command::FetchXResp:
            return handleFetchResp(respEvent, dirLine, reqEvent);
            break;
        case Command::AckInv:
            return handleAckInv(respEvent, dirLine, reqEvent);
        case Command::AckPut:
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            return DONE;    // Retry any events that were stalled for ack
        case Command::FlushLineResp:
            recordStateEventCount(respEvent->getCmd(), dirLine ? dirLine->getState() : I);
            sendFlushResponse(reqEvent, respEvent->success());
            if (dirLine && dirLine->getState() == S_B) dirLine->setState(S);
            else if (dirLine) dirLine->setState(I);
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return DONE;    // Eliminate compiler warning
}



bool MESIInternalDirectory::isRetryNeeded(MemEvent * event, CacheLine * dirLine) {
    Command cmd = event->getCmd();
    State state = dirLine ? dirLine->getState() : I;
    
    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            return true;
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
            if (expectWritebackAck_ && !mshr_->pendingWriteback(event->getBaseAddr())) return false;
            return true;
        case Command::FetchInvX:
            if (state == I) return false;
            if (dirLine->getOwner() != event->getDst()) return false;
            return true;
        case Command::FetchInv:
            if (state == I) return false;
            if ((dirLine->getOwner() != event->getDst()) && !dirLine->isSharer(event->getDst())) return false;
            return true;
        case Command::Fetch:
        case Command::Inv:
            if (state == I) return false;
            if (!dirLine->isSharer(event->getDst())) return false;
            return true;
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received NACK for unrecognized event: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return true;
}


/*
 *  Return type of miss. Used by cacheController for profiling incoming events
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request)
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition
 */
int MESIInternalDirectory::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
    Command cmd = event->getCmd();
    State state = cacheLine->getState();
    if (cmd == Command::GetSX) cmd = Command::GetX;  // for our purposes these are equal

    if (state == I) return 1;
    if (event->isPrefetch() && event->getRqstr() == parent->getName()) return 0;
    if (state == S && lastLevel_) state = M;
    switch (state) {
        case S:
            if (cmd == Command::GetS) return 0;
            return 2;
        case E:
        case M:
            if (cacheLine->ownerExists()) return 3;
            if (cmd == Command::GetS) return 0; 
            if (cmd == Command::GetX) {
                if (cacheLine->isShareless() || (cacheLine->isSharer(event->getSrc()) && cacheLine->numSharers() == 1)) return 0; // Hit
            }
            return 3;
        case IS:
        case IM:
        case SM:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
        case E_InvX:
        case M_InvX:
            return 3;
        default:
            return 0;   // this is profiling so don't die on unhandled state
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Internal event handlers
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Handle GetS requests. 
 *  Non-inclusive so GetS hits don't deallocate the locally cached block.
 */
CacheAction MESIInternalDirectory::handleGetSRequest(MemEvent* event, CacheLine* dirLine, bool replay) {
    State state = dirLine->getState();
    
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == parent->getName());
    recordStateEventCount(event->getCmd(), state);    
    bool isCached = dirLine->getDataLine() != NULL;
    uint64_t sendTime = 0;
    switch (state) {
        case I:
            sendTime = forwardMessage(event, dirLine->getBaseAddr(), lineSize_, 0, NULL);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            dirLine->setState(IS);
            dirLine->setTimestamp(sendTime);
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                return DONE;
            }
            if (dirLine->getPrefetch()) { /* Since prefetch gets unset if data replaced, we shouldn't have an issue with isCached=false */
                dirLine->setPrefetch(false);
                statPrefetchHit->addData(1);
            }
            
            if (isCached) {
                dirLine->addSharer(event->getSrc());
                sendTime = sendResponseUp(event, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                return DONE;
            } 
            sendFetch(dirLine, event->getRqstr(), replay);
            mshr_->incrementAcksNeeded(event->getBaseAddr());
            dirLine->setState(S_D);     // Fetch in progress, block incoming invalidates/fetches/etc.
            return STALL;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                return DONE;
            }
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchHit->addData(1);
            }
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) dirLine->setState(E_InvX);
                else dirLine->setState(M_InvX);
                return STALL;
            } else if (isCached) {
                if (protocol_ && dirLine->numSharers() == 0) {
                    sendTime = sendResponseUp(event, Command::GetXResp, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
                    dirLine->setOwner(event->getSrc());
                    dirLine->setTimestamp(sendTime);
                } else {
                    sendTime = sendResponseUp(event, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
                    dirLine->addSharer(event->getSrc());
                    dirLine->setTimestamp(sendTime);
                }
                return DONE;
            } else {
                sendFetch(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) dirLine->setState(E_D);
                else dirLine->setState(M_D);
                return STALL;
            }
        default:
            debug->fatal(CALL_INFO,-1,"%s (dir), Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], getCurrentSimTimeNano());

    }
    return STALL;    // eliminate compiler warning
}


/**
 *  Handle GetX and GetSX (read-lock) requests
 *  Deallocate on hits
 */
CacheAction MESIInternalDirectory::handleGetXRequest(MemEvent* event, CacheLine* dirLine, bool replay) {
    State state = dirLine->getState();
    Command cmd = event->getCmd();
    if (state != SM) recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;
    uint64_t sendTime = 0;
    
    /* Special case - if this is the last coherence level (e.g., just mem below), 
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        dirLine->setState(M);
    }
    

    switch (state) {
        case I:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            sendTime = forwardMessage(event, dirLine->getBaseAddr(), lineSize_, 0, &event->getPayload());
            dirLine->setState(IM);
            dirLine->setTimestamp(sendTime);
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchUpgradeMiss->addData(1);
            }
            sendTime = forwardMessage(event, dirLine->getBaseAddr(), lineSize_, dirLine->getTimestamp(), &event->getPayload());
            if (invalidateSharersExceptRequestor(dirLine, event->getSrc(), event->getRqstr(), replay, false)) {
                dirLine->setState(SM_Inv);
            } else {
                dirLine->setState(SM);
                dirLine->setTimestamp(sendTime);
            }
            return STALL;
        case E:
            dirLine->setState(M);
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchHit->addData(1);
            }

            if (invalidateSharersExceptRequestor(dirLine, event->getSrc(), event->getRqstr(), replay, !isCached)) {
                dirLine->setState(M_Inv);
                return STALL;
            }
            if (dirLine->ownerExists()) {
                sendFetchInv(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                dirLine->setState(M_Inv);
                return STALL;
            }
            dirLine->setOwner(event->getSrc());
            if (dirLine->isSharer(event->getSrc())) dirLine->removeSharer(event->getSrc());
            if (isCached) sendTime = sendResponseUp(event, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());  // is an upgrade request, requestor has data already
            else sendTime = sendResponseUp(event, NULL, replay, dirLine->getTimestamp());
            dirLine->setTimestamp(sendTime);
            // TODO DEALLOCATE dataline
            return DONE;
        case SM:
            return STALL;   // retried this request too soon (TODO fix so we don't even attempt retry)!
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/* Handle PutS at the cache 
 * ReqEvent is only populated if this replacement raced with another request
 */
CacheAction MESIInternalDirectory::handlePutSRequest(MemEvent * event, CacheLine * dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);

    if (state == S_D || state == E_D || state == SM_D || state == M_D) {
        if (*(dirLine->getSharers()->begin()) == event->getSrc()) {    // Put raced with Fetch
            mshr_->decrementAcksNeeded(event->getBaseAddr());
        }
    } else if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());

    if (dirLine->isSharer(event->getSrc())) {
        dirLine->removeSharer(event->getSrc());
    }
    // Set data, either to cache or to MSHR
    if (dirLine->getDataLine() != NULL) {
        dirLine->getDataLine()->setData(event->getPayload(), 0);
        printData(dirLine->getDataLine()->getData(), true);
    } else if (mshr_->isHit(dirLine->getBaseAddr())) mshr_->setDataBuffer(dirLine->getBaseAddr(), event->getPayload());
    
    uint64_t sendTime = 0;

    CacheAction action = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) ? DONE : IGNORE;
    if (action == IGNORE) return action;
    switch(state) {
        case I:
        case S:
        case E:
        case M:
        case S_B:
            sendWritebackAck(event);
            return DONE;
        case SI:
            sendWritebackFromMSHR(Command::PutS, dirLine, reqEvent->getRqstr(), &event->getPayload());
            if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
            dirLine->setState(I);
            return DONE;
        case EI:
            sendWritebackFromMSHR(Command::PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
            if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
            dirLine->setState(I);
            return DONE;
        case MI:
            sendWritebackFromMSHR(Command::PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
            if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
            dirLine->setState(I);
            return DONE;
        case S_Inv: // PutS raced with Inv or FetchInv request
            if (reqEvent->getCmd() == Command::Inv) {
                sendAckInv(reqEvent);
            } else {
                sendResponseDownFromMSHR(event, false);
            }
            dirLine->setState(I);
            return DONE;
        case SB_Inv:
            sendAckInv(reqEvent);
            dirLine->setState(I_B);
            return DONE;
        case S_D: // PutS raced with Fetch
            dirLine->setState(S);
            if (reqEvent->getCmd() == Command::Fetch) {
                if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                    sendWritebackFromMSHR(Command::PutS, dirLine, reqEvent->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                } else {
                    sendResponseDownFromMSHR(event, false);
                }
            } else if (reqEvent->getCmd() == Command::GetS) {    // GetS
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (is_debug_event(event)) printData(&event->getPayload(), false);
            } else {
                debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], event->getBaseAddr(), getCurrentSimTimeNano());
            }
            return DONE;
        case E_Inv:
            if (reqEvent->getCmd() == Command::FetchInv) {
                sendResponseDown(reqEvent, dirLine, &event->getPayload(), event->getDirty(), true);
                dirLine->setState(I);
            }
            return DONE;
        case E_D: // PutS raced with Fetch from GetS
            dirLine->setState(E);
            if (reqEvent->getCmd() == Command::Fetch) {
                if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                    sendWritebackFromMSHR(Command::PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                } else {
                    sendResponseDownFromMSHR(event, false);
                }
            } else if (reqEvent->getCmd() == Command::GetS) {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                if (dirLine->numSharers() == 0) {
                    dirLine->setOwner(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                } else {
                    dirLine->addSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                }
                if (is_debug_event(event)) printData(&event->getPayload(), false);
            } else {
                debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], event->getBaseAddr(), getCurrentSimTimeNano());
            }
            return DONE;
        case E_InvX: // PutS raced with Fetch from FetchInvX
            dirLine->setState(S);
            if (reqEvent->getCmd() == Command::FetchInvX) {
                if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                    sendWritebackFromMSHR(Command::PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                } else {
                    sendResponseDownFromMSHR(event, false);
                }
            } else {
                debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], event->getBaseAddr(), getCurrentSimTimeNano());
            }
            return DONE;
        case M_Inv: // PutS raced with AckInv from GetX, PutS raced with AckInv from FetchInv
            if (reqEvent->getCmd() == Command::FetchInv) {
                sendResponseDown(reqEvent, dirLine, &event->getPayload(), true, true);
                dirLine->setState(I);
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                dirLine->setOwner(reqEvent->getSrc());
                if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (is_debug_event(reqEvent)) printData(&event->getPayload(), false);
                dirLine->setState(M);
            }
            return DONE;
        case M_D:   // PutS raced with Fetch from GetS
            dirLine->setState(M);
            if (reqEvent->getCmd() == Command::Fetch) {
                if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                    sendWritebackFromMSHR(Command::PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                } else {
                    sendResponseDownFromMSHR(event, false);
                }
            } else if (reqEvent->getCmd() == Command::GetS) {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                if (dirLine->numSharers() == 0) {
                    dirLine->setOwner(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                } else {
                    dirLine->addSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                }
                if (is_debug_event(event)) printData(&event->getPayload(), false);
            } else {
                debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], event->getBaseAddr(), getCurrentSimTimeNano());
            }
            return DONE;
        case SM_Inv:
            if (reqEvent->getCmd() == Command::Inv) {    // Completed Inv so handle
                if (dirLine->numSharers() > 0) {
                    invalidateAllSharers(dirLine, event->getRqstr(), true);
                    return IGNORE;
                }
                sendAckInv(reqEvent);
                dirLine->setState(IM);
            } else if (reqEvent->getCmd() == Command::FetchInv) {
                if (dirLine->numSharers() > 0) {
                    invalidateAllSharers(dirLine, event->getRqstr(), true);
                    return IGNORE;
                }
                sendResponseDownFromMSHR(event, false);
                dirLine->setState(IM);
            } else {    // Waiting on data for upgrade
                dirLine->setState(SM);
                action = IGNORE;
            }
            return DONE;
        case SM_D:
            if (reqEvent->getCmd() == Command::Fetch) {
                sendResponseDownFromMSHR(event, false);
                dirLine->setState(SM);
            } 
            return DONE;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received PutS in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(),
                    StateString[state], getCurrentSimTimeNano());
    }
    return action; // eliminate compiler warning
}


/* CacheAction return value indicates whether the racing action completed (reqEvent). PutMs always complete! */
CacheAction MESIInternalDirectory::handlePutMRequest(MemEvent * event, CacheLine * dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);

    bool isCached = dirLine->getDataLine() != NULL;
    if (isCached) dirLine->getDataLine()->setData(event->getPayload(), 0);
    else if (mshr_->isHit(dirLine->getBaseAddr())) mshr_->setDataBuffer(dirLine->getBaseAddr(), event->getPayload());

    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());

    uint64_t sendTime = 0;

    switch (state) {
        case E:
            if (event->getDirty()) dirLine->setState(M);
        case M:
            dirLine->clearOwner();
            sendWritebackAck(event);
            if (!isCached) {
                sendWritebackFromMSHR(((dirLine->getState() == E) ? Command::PutE : Command::PutM), dirLine, event->getRqstr(), &event->getPayload());
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case EI:    // Evicting this block anyways
            if (event->getDirty()) dirLine->setState(MI);
        case MI:
            dirLine->clearOwner();
            sendWritebackFromMSHR(((dirLine->getState() == EI) ? Command::PutE : Command::PutM), dirLine, parent->getName(), &event->getPayload());
            if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
            dirLine->setState(I);
            break;
        case E_InvX:
            dirLine->clearOwner();
            if (reqEvent->getCmd() == Command::FetchInvX) {
                if (!isCached) {
                    sendWritebackFromMSHR(event->getDirty() ? Command::PutM : Command::PutE, dirLine, event->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                    if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    sendResponseDownFromMSHR(event, (event->getCmd() == Command::PutM));
                    dirLine->setState(S);
                }
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                if (protocol_) {
                    sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->addSharer(reqEvent->getSrc());
                }
                if (is_debug_event(event)) printData(&event->getPayload(), false);
                if (event->getDirty()) dirLine->setState(M);
                else dirLine->setState(E);
            }
            return DONE;
        case M_InvX:
            dirLine->clearOwner();
            if (reqEvent->getCmd() == Command::FetchInvX) {
                if (!isCached) {
                    sendWritebackFromMSHR(Command::PutM, dirLine, event->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                    if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    sendResponseDownFromMSHR(event, true);
                    dirLine->setState(S);
                }
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->setState(M);
                if (protocol_) {
                    sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->addSharer(reqEvent->getSrc());
                }
                if (is_debug_event(event)) printData(&event->getPayload(), false);
            }
            return DONE;
        case E_Inv:
            if (event->getCmd() == Command::PutM) dirLine->setState(M_Inv);
        case M_Inv: // PutM raced with FetchInv to owner
            dirLine->clearOwner();
            if (reqEvent->getCmd() == Command::GetX || reqEvent->getCmd() == Command::GetSX) {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                dirLine->setState(M);
                sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                dirLine->setOwner(reqEvent->getSrc());
                if (is_debug_event(event)) printData(&event->getPayload(), false);
            } else { /* Cmd == Fetch */
                sendResponseDownFromMSHR(event, (dirLine->getState() == M_Inv));
                dirLine->setState(I);
            }
            return DONE;
        default:
    	    debug->fatal(CALL_INFO, -1, "%s, Error: Updating data but cache is not in E or M state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), event->getBaseAddr(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE;
}



CacheAction MESIInternalDirectory::handleFlushLineRequest(MemEvent * event, CacheLine * dirLine, MemEvent * reqEvent, bool replay) {
    State state = dirLine ? dirLine->getState() : I;
    if (!replay) recordStateEventCount(event->getCmd(), state);

    bool isCached = dirLine && dirLine->getDataLine() != NULL;
    if (event->getPayloadSize() != 0) {
        if (isCached) dirLine->getDataLine()->setData(event->getPayload(), 0);
        else if (mshr_->isHit(event->getBaseAddr())) mshr_->setDataBuffer(event->getBaseAddr(), event->getPayload());
    }

    CacheAction reqEventAction; // What to do with the reqEvent
    uint64_t sendTime = 0;
    // Handle flush at local level
    switch (state) {
        case I:
        case S:
        case I_B:
        case S_B:
            if (reqEvent != NULL) return STALL;
            break;
        case E:
        case M:
            if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                dirLine->addSharer(event->getSrc());
                if (event->getDirty()) {
                    dirLine->setState(M);
                }
            }
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                state == E ? dirLine->setState(E_InvX) : dirLine->setState(M_InvX);
                return STALL;
            }
            break;
        case IM:
        case IS:
        case SM:
            return STALL; // Wait for the Get* request to finish
        case SM_D:
        case S_D:
        case E_D:
        case M_D:
            return STALL; // Flush raced with Fetch 
        case S_Inv:
        case SI:
            return STALL; // Flush raced with Inv
        case SM_Inv:
            return STALL; // Flush raced with Inv
        case MI:
        case EI:
        case M_Inv:
        case E_Inv:
            if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                dirLine->addSharer(event->getSrc()); // Other cache will treat FetchInv as Inv
            }
            if (event->getDirty()) {
                if (state == EI) dirLine->setState(MI);
                else if (state == E_Inv) dirLine->setState(M_Inv);
            }
            return STALL;
        case M_InvX:
        case E_InvX:
            if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                dirLine->addSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
                if (event->getDirty()) {
                    dirLine->setState(M_InvX);
                    state = M_InvX;
                }
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInvX) {
                    sendResponseDownFromMSHR(event, state == M_InvX);
                    dirLine->setState(S);
                } else if (reqEvent->getCmd() == Command::FlushLine) {
                    dirLine->setState(NextState[state]);
                    return handleFlushLineRequest(reqEvent, dirLine, NULL, true);
                } else if (reqEvent->getCmd() == Command::FetchInv) {
                    dirLine->setState(NextState[state]);
                    return handleFetchInv(reqEvent, dirLine, true, NULL);
                } else {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    dirLine->addSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, (isCached ? dirLine->getDataLine()->getData() : mshr_->getDataBuffer(event->getBaseAddr())), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setState(NextState[state]);
                }
                return DONE;
            } else return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s in unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)event->getCmd()], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    forwardFlushLine(event, dirLine, dirLine && dirLine->getState() == M, Command::FlushLine);
    if (dirLine && dirLine->getState() != I) dirLine->setState(S_B);
    else if (dirLine) dirLine->setState(I_B);
    event->setInProgress(true);
    return STALL;   // wait for response
}


/**
 * Handler for cache line flush requests
 * Invalidate owner/sharers
 * Invalidate local
 * Forward
 */

CacheAction MESIInternalDirectory::handleFlushLineInvRequest(MemEvent * event, CacheLine * dirLine, MemEvent * reqEvent, bool replay) {
    State state = dirLine ? dirLine->getState() : I;
    if (!replay) recordStateEventCount(event->getCmd(), state);

    bool isCached = dirLine && dirLine->getDataLine() != NULL;
    if (event->getPayloadSize() != 0) {
        if (isCached) dirLine->getDataLine()->setData(event->getPayload(), 0);
        else if (mshr_->isHit(event->getBaseAddr())) mshr_->setDataBuffer(event->getBaseAddr(), event->getPayload());
    }

    // Apply incoming flush -> remove if owner
    if (state == M || state == E) {
        if (dirLine->getOwner() == event->getSrc()) {
            dirLine->clearOwner();
            if (event->getDirty()) {
                dirLine->setState(M);
                state = M;
            }
        }
    }

    CacheAction reqEventAction; // What to do with the reqEvent
    uint64_t sendTime = 0;
    // Handle flush at local level
    switch (state) {
        case I:
            if (reqEvent != NULL) return STALL;
            break;
        case S:
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }

            if (dirLine->isSharer(event->getSrc())) dirLine->removeSharer(event->getSrc());
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                dirLine->setState(S_Inv);
                return STALL;
            }
            break;
        case E:
        case M:
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }

            if (dirLine->isSharer(event->getSrc())) dirLine->removeSharer(event->getSrc());
            if (dirLine->ownerExists()) {
                sendFetchInv(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                state == E ? dirLine->setState(E_Inv) : dirLine->setState(M_Inv);
                return STALL;
            }
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                state == E ? dirLine->setState(E_Inv) : dirLine->setState(M_Inv);
                return STALL;
            }
            break;
        case IM:
        case IS:
        case SM:
            return STALL; // Wait for the Get* request to finish
        case SM_D:
            if (*(dirLine->getSharers()->begin()) == event->getSrc()) { // Flush raced with Fetch
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::Fetch) {
                    dirLine->setState(SM);
                    sendResponseDownFromMSHR(event, false);
                    return DONE;
                }
            } 
            return STALL;
        
        case S_D:
        case E_D:
        case M_D:
            if (*(dirLine->getSharers()->begin()) == event->getSrc()) {
                mshr_->decrementAcksNeeded(event->getBaseAddr()); 
            }
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                dirLine->setState(NextState[state]);
                if (reqEvent->getCmd() == Command::Fetch) {
                    if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                        if (state == M_D || event->getDirty()) sendWritebackFromMSHR(Command::PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        else if (state == E_D) sendWritebackFromMSHR(Command::PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        else if (state == S_D) sendWritebackFromMSHR(Command::PutS, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, (state == M_D || event->getDirty()) ? true : false);
                    }
                } else if (reqEvent->getCmd() == Command::GetS) {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    if (dirLine->numSharers() > 0 || state == S_D) {
                        dirLine->addSharer(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    } else {
                        dirLine->setOwner(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    }
                    if (is_debug_event(event)) printData(&event->getPayload(), false);
                } else {
                    debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received FlushLineInv in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                            parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], event->getBaseAddr(), getCurrentSimTimeNano());
                }
                return DONE;
            }
            return STALL;
        case S_Inv:
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            reqEventAction = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) ? DONE : STALL;
            if (reqEventAction == DONE) {
                if (reqEvent->getCmd() == Command::Inv) {
                    sendAckInv(reqEvent);
                    dirLine->setState(I);
                } else if (reqEvent->getCmd() == Command::Fetch || reqEvent->getCmd() == Command::FetchInv || reqEvent->getCmd() == Command::FetchInvX) {
                    sendResponseDownFromMSHR(event, false);
                    dirLine->setState(I);
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(reqEvent, dirLine, false, Command::FlushLineInv);
                    reqEventAction = STALL;
                    dirLine->setState(I_B);
                }
            }
            return reqEventAction;
        case SM_Inv:
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::Inv) {
                    if (dirLine->numSharers() > 0) {  // May not have invalidated GetX requestor -> cannot also be the FlushLine requestor since that one is in I and blocked on flush
                        invalidateAllSharers(dirLine, reqEvent->getRqstr(), true);
                        return STALL;
                    } else {
                        sendAckInv(reqEvent);
                        dirLine->setState(IM);
                        return DONE;
                    }
                } else if (reqEvent->getCmd() == Command::GetXResp) {
                    dirLine->setState(SM);
                    return STALL; // Waiting for GetXResp
                }
                debug->fatal(CALL_INFO, -1, "%s, Error: Received %s in state SM_Inv but case does not match an implemented handler. Addr = 0x%" PRIx64 ", Src = %s, OrigEvent = %s. Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), CommandString[(int)reqEvent->getCmd()], getCurrentSimTimeNano());
            }
            return STALL;
        case MI:
            if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            } else if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (isCached) sendWritebackFromCache(Command::PutM, dirLine, parent->getName());
                else sendWritebackFromMSHR(Command::PutM, dirLine, parent->getName(), mshr_->getDataBuffer(event->getBaseAddr()));
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(NextState[state]);
                return DONE;
            } else return STALL;
        case EI:
            if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            } else if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (event->getDirty()) dirLine->setState(MI);
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (isCached && event->getDirty()) sendWritebackFromCache(Command::PutM, dirLine, parent->getName());
                else if (isCached) sendWritebackFromCache(Command::PutE, dirLine, parent->getName());
                else if (event->getDirty()) sendWritebackFromMSHR(Command::PutM, dirLine, parent->getName(), mshr_->getDataBuffer(event->getBaseAddr()));
                else sendWritebackFromMSHR(Command::PutE, dirLine, parent->getName(), mshr_->getDataBuffer(event->getBaseAddr()));
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(NextState[state]);
                return DONE;
            } else return STALL;
        case SI:
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (isCached) sendWritebackFromCache(Command::PutS, dirLine, parent->getName());
                else sendWritebackFromMSHR(Command::PutS, dirLine, parent->getName(), mshr_->getDataBuffer(event->getBaseAddr()));
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
                return DONE;
            } else return STALL;
        case M_Inv:
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            } else if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, dirLine, &event->getPayload(), true, true);
                    dirLine->setState(I);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::GetX || reqEvent->getCmd() == Command::GetSX) {
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, (isCached ? dirLine->getDataLine()->getData() : mshr_->getDataBuffer(event->getBaseAddr())), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setState(M);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(reqEvent, dirLine, true, Command::FlushLineInv);
                    dirLine->setState(I_B);
                    return STALL;
                }
            } else return STALL;
        case E_Inv:
            if (dirLine->isSharer(event->getSrc())) {
                dirLine->removeSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            } else if (dirLine->getOwner() == event->getSrc()) {
                dirLine->clearOwner();
                mshr_->decrementAcksNeeded(event->getBaseAddr());
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, dirLine, &event->getPayload(), event->getDirty(), true);
                    dirLine->setState(I);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(reqEvent, dirLine, reqEvent->getDirty(), Command::FlushLineInv);
                    dirLine->setState(I_B);
                    return STALL;

                }
            } else return STALL;
        case M_InvX:
        case E_InvX:
            if (dirLine->getPrefetch()) {
                dirLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (dirLine->getOwner() == event->getSrc()) {
                mshr_->decrementAcksNeeded(event->getBaseAddr());
                dirLine->clearOwner();
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInvX) {
                    if (!isCached) {
                        sendWritebackFromMSHR((event->getDirty() || state == M_InvX) ? Command::PutM : Command::PutE, dirLine, event->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                        if (expectWritebackAck_) mshr_->insertWriteback(event->getBaseAddr());
                    } else {
                        sendResponseDownFromMSHR(event, (state == M_InvX || event->getDirty()));
                        dirLine->setState(S);
                    }   
                } else {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    if (protocol_) {
                        sendTime = sendResponseUp(reqEvent, Command::GetXResp, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                        dirLine->addSharer(reqEvent->getSrc());
                    } else {
                        sendTime = sendResponseUp(reqEvent, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                        dirLine->addSharer(reqEvent->getSrc());
                    }
                    if (is_debug_event(event)) printData(&event->getPayload(), false);
                }

                (state == M_InvX || event->getDirty()) ? dirLine->setState(M) : dirLine->setState(E);
                return DONE;
            } else return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received %s in unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), CommandString[(int)event->getCmd()], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), getCurrentSimTimeNano());
    }

    forwardFlushLine(event, dirLine, dirLine && dirLine->getState() == M, Command::FlushLineInv);
    if (dirLine) dirLine->setState(I_B);
    return STALL;   // wait for response
}


/**
 *  Handler for 'Inv' requests
 *  Invalidate sharers if needed
 *  Send AckInv if no sharers exist
 */
CacheAction MESIInternalDirectory::handleInv(MemEvent* event, CacheLine* dirLine, bool replay, MemEvent * collisionEvent) {
    
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    if (dirLine->getPrefetch()) {
        dirLine->setPrefetch(false);
        statPrefetchInv->addData(1);
    }

    switch(state) {
        case I_B:   // Already forwarded our flush
            return DONE;
        case S_B:
        case S:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                state == S_B ? dirLine->setState(SB_Inv) : dirLine->setState(S_Inv);
                // Resolve races with waiting PutS requests
                while (collisionEvent != NULL) {
                    if (collisionEvent->getCmd() == Command::PutS) {
                        dirLine->removeSharer(collisionEvent->getSrc());
                        mshr_->decrementAcksNeeded(event->getBaseAddr());
                        mshr_->removeElement(event->getBaseAddr(), collisionEvent);   
                        delete collisionEvent;
                        collisionEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
                        if (collisionEvent && collisionEvent->getCmd() != Command::PutS) collisionEvent = NULL;
                    } else collisionEvent = NULL;
                }
                if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) return STALL;
            }
            sendAckInv(event);
            state == S_B ? dirLine->setState(I_B) : dirLine->setState(I);
            return DONE;
        case SM:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                dirLine->setState(SM_Inv);
                while (collisionEvent != NULL) {
                    if (collisionEvent->getCmd() == Command::PutS) {
                        dirLine->removeSharer(collisionEvent->getSrc());
                        mshr_->decrementAcksNeeded(event->getBaseAddr());
                        mshr_->removeFront(event->getBaseAddr());   // We've sent an inv to them so no need for AckPut
                        delete collisionEvent;
                        collisionEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
                        if (collisionEvent == NULL || collisionEvent->getCmd() != Command::PutS) collisionEvent = NULL;
                    } else collisionEvent = NULL;
                }
                if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) return STALL;
            }
            sendAckInv(event);
            dirLine->setState(IM);
            return DONE;
        case SI:
        case S_Inv: // PutS in progress, stall this Inv for that
        case S_D:   // Waiting for a GetS to resolve, stall until it does
            return BLOCK;
        case SM_Inv:    // Waiting on GetSResp, stall this Inv until invacks come back
            return STALL;
        default:
	    debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received an invalidation in an unhandled state: %s. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    parent->getName().c_str(), CommandString[(int)event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL;
}


/**
 * Handle ForceInv requests
 * Invalidate block regardless of whether it is dirty or not and send an ack
 * Do not forward dirty data with ack
 */
CacheAction MESIInternalDirectory::handleForceInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;
    
    if (dirLine->getPrefetch()) {
        dirLine->setPrefetch(false);
        statPrefetchInv->addData(1);
    }

    /* Handle mshr collisions with replacements - treat as having already occured, however AckPut needs to get returned */
    while (collisionEvent && collisionEvent->isWriteback()) {
        if (dirLine->isSharer(collisionEvent->getSrc())) dirLine->removeSharer(collisionEvent->getSrc());
        if (dirLine->ownerExists()) dirLine->clearOwner();
        sendWritebackAck(collisionEvent);
        mshr_->removeFront(dirLine->getBaseAddr());
        delete collisionEvent;
        collisionEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : nullptr;
    }

    switch(state) {
        /* Already sent some message indicating invalid to the next level so that will serve as AckInv */
        case I:
        case IS:
        case IM:
        case I_B:
            return IGNORE; 
        /* Cases where we are in shared - send any invalidations that need to go out */
        case S:
        case S_B:
        case SM:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                if (state == S) dirLine->setState(S_Inv);
                else if (state == S_B) dirLine->setState(SB_Inv);
                else dirLine->setState(SM_Inv);
                if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) return STALL;
            }
            sendAckInv(event);
            if (state == S) dirLine->setState(I);
            else if (state == S_B) dirLine->setState(I_B);
            else dirLine->setState(IM);
            return DONE;
        case E:
        case M:
            if (dirLine->ownerExists()) {
                sendForceInv(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                state == E ? dirLine->setState(E_Inv) : dirLine->setState(M_Inv);
                return STALL;
            }
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                state == E ? dirLine->setState(E_Inv) : dirLine->setState(M_Inv);
                return STALL;
            }
            sendAckInv(event);
            dirLine->setState(I);
            return DONE;
        case SI:
            dirLine->setState(S_Inv);
            return STALL;
        case EI:
            dirLine->setState(E_Inv);
            return STALL;
        case MI:
            dirLine->setState(M_Inv);
            return STALL;
        case S_D:
        case E_D:
        case M_D:
        case SM_D:
        case E_InvX:
        case M_InvX:
        case M_Inv:
        case S_Inv:
        case E_Inv:
        case SM_Inv:
        case SB_Inv:
            if (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv) return STALL;
            return BLOCK;
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns.\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL;
}



/**
 *  Handler for Fetch requests
 *  Forward to sharer with data
 */
CacheAction MESIInternalDirectory::handleFetch(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    

    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case S:
        case SM:
            if (dirLine->getDataLine() != NULL) {
                sendResponseDown(event, dirLine, dirLine->getDataLine()->getData(), false, replay);
                return DONE;
            }
            if (collisionEvent != NULL) {
                sendResponseDown(event, dirLine, &collisionEvent->getPayload(), false, replay);
                return DONE;
            }
            sendFetch(dirLine, event->getRqstr(), replay);
            mshr_->incrementAcksNeeded(event->getBaseAddr());
            if (state == S) dirLine->setState(S_D);
            else dirLine->setState(SM_D);
            return STALL;
        case S_Inv:
        case SI:
        case S_D:
            return BLOCK; // Block while current request completes
        default:
            debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received Fetch but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handler for FetchInv requests
 *  Invalidate owner and/or sharers if needed
 *  Send FetchResp if no further invalidations are needed
 *  Collision can be a Put waiting for an AckPut or a Flush waiting for a FlushResp
 */
CacheAction MESIInternalDirectory::handleFetchInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    if (dirLine->getPrefetch()) {
        dirLine->setPrefetch(false);
        statPrefetchInv->addData(1);
    }

    bool isCached = dirLine->getDataLine() != NULL;
    bool collision = false;
    // If colliding event is a replacement, treat the replacement as if it had aleady occured/raced with an earlier FetchInv
    if (collisionEvent && collisionEvent->isWriteback()) {
        collision = true;
        if (dirLine->isSharer(collisionEvent->getSrc())) dirLine->removeSharer(collisionEvent->getSrc());
        if (dirLine->ownerExists()) dirLine->clearOwner();
        mshr_->setDataBuffer(collisionEvent->getBaseAddr(), collisionEvent->getPayload());
        if (state == E && collisionEvent->getDirty()) dirLine->setState(M);
        state = M;
        sendWritebackAck(collisionEvent);
        mshr_->removeFront(dirLine->getBaseAddr());
        delete collisionEvent;
    } 

    switch (state) {
        case I:
        case IS:
        case IM:
        case I_B:
            return IGNORE;
        case S:
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(S_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL && !collision) debug->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    parent->getName().c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getDataBuffer(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
            dirLine->setState(I);
            return DONE;
        case SM:
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(SM_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL) debug->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    parent->getName().c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getDataBuffer(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
            dirLine->setState(IM);
            return DONE;
        case S_B:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                dirLine->setState(SB_Inv);
                return STALL;
            }
            sendAckInv(event);
            dirLine->setState(I_B);
            return DONE;
        case E:
            if (dirLine->ownerExists()) {
                sendFetchInv(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                dirLine->setState(E_Inv);
                return STALL;
            }
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(E_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL && !collision) debug->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    parent->getName().c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getDataBuffer(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
            dirLine->setState(I);
            return DONE;
        case M:
            if (dirLine->ownerExists()) {
                sendFetchInv(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                dirLine->setState(M_Inv);
                return STALL;
            }
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(M_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL && !collision) debug->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    parent->getName().c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getDataBuffer(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), true, replay);
            dirLine->setState(I);
            return DONE;
        case EI:
            dirLine->setState(E_Inv);
            return STALL;
        case MI:
            dirLine->setState(M_Inv);
            return STALL;
        case S_D:
        case E_D:
        case M_D:
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            // Handle incoming Inv before a pending flushline to avoid deadlock
            if (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv) return STALL;
            return BLOCK;
        default:
            debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 *  Handler for FetchInvX requests
 *  Downgrade owner if needed
 *  Send FetchXResp if no further downgrades are needed
 */
CacheAction MESIInternalDirectory::handleFetchInvX(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;
    bool collision = collisionEvent && collisionEvent->isWriteback();
    if (collision) {   // Treat the replacement as if it had already occured/raced with an earlier FetchInv
        if (state == E && collisionEvent->getDirty()) dirLine->setState(M);
        state = M;
    }

    switch (state) {
        case I:
        case IS:
        case IM:
        case I_B:
        case S_B:
            return IGNORE;
        case E:
            if (collision) {
                if (dirLine->ownerExists()) {
                    dirLine->clearOwner();
                    dirLine->addSharer(collisionEvent->getSrc());
                    collisionEvent->setCmd(Command::PutS);   // TODO there's probably a cleaner way to do this...and a safer/better way!
                }
                dirLine->setState(S);
                sendResponseDown(event, dirLine, &collisionEvent->getPayload(), collisionEvent->getDirty(), replay);
                return DONE;
            }
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                dirLine->setState(E_InvX);
                return STALL;
            }
            if (isCached) {
                sendResponseDown(event, dirLine, dirLine->getDataLine()->getData(), false, replay);
                dirLine->setState(S);
                return DONE;
            }
            // Otherwise shared and not cached
            sendFetch(dirLine, event->getRqstr(), replay);
            mshr_->incrementAcksNeeded(event->getBaseAddr());
            dirLine->setState(E_InvX);
            return STALL;
        case M:
           if (collision) {
                if (dirLine->ownerExists()) {
                    dirLine->clearOwner();
                    dirLine->addSharer(collisionEvent->getSrc());
                    collisionEvent->setCmd(Command::PutS);   // TODO there's probably a cleaner way to do this...and a safer/better way!
                }
                dirLine->setState(S);
                sendResponseDown(event, dirLine, &collisionEvent->getPayload(), true, replay);
                return DONE;
            }
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                dirLine->setState(M_InvX);
                return STALL;
            }
            if (isCached) {
                sendResponseDown(event, dirLine, dirLine->getDataLine()->getData(), true, replay);
                dirLine->setState(S);
                return DONE;
            }
            // Otherwise shared and not cached
            sendFetch(dirLine, event->getRqstr(), replay);
            mshr_->incrementAcksNeeded(event->getBaseAddr());
            dirLine->setState(M_InvX);
            return STALL;
        case E_D:
        case M_D:
        case EI:
        case MI:
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            // Handle incoming Inv before FlushLine to avoid deadlock
            if (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv) return STALL;
            return BLOCK;
        default:
            debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received FetchInvX but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        parent->getName().c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 *  Handle Get responses
 *  Update coherence state and forward response to requestor, if any
 *  (Prefetch requests originated by this entity do not get forwarded)
 */
CacheAction MESIInternalDirectory::handleDataResponse(MemEvent* responseEvent, CacheLine* dirLine, MemEvent* origRequest) {
    
    State state = dirLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);    
    
    origRequest->setMemFlags(responseEvent->getMemFlags());

    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == parent->getName());
    bool isCached = dirLine->getDataLine() != NULL;
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            if (responseEvent->getCmd() == Command::GetXResp && protocol_) dirLine->setState(E);
            else dirLine->setState(S);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), 0);
            if (localPrefetch) {
                dirLine->setPrefetch(true);
                return DONE;
            }
            if (dirLine->getState() == E) {
                dirLine->setOwner(origRequest->getSrc());
                sendTime = sendResponseUp(origRequest, Command::GetXResp, &responseEvent->getPayload(), true, dirLine->getTimestamp());
            } else {
                dirLine->addSharer(origRequest->getSrc());
                sendTime = sendResponseUp(origRequest, &responseEvent->getPayload(), true, dirLine->getTimestamp());
            }
            dirLine->setTimestamp(sendTime);
            if (is_debug_event(responseEvent)) printData(&responseEvent->getPayload(), false);
            return DONE;
        case IM:
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), 0);
        case SM:
            dirLine->setState(M);
            dirLine->setOwner(origRequest->getSrc());
            if (dirLine->isSharer(origRequest->getSrc())) dirLine->removeSharer(origRequest->getSrc());
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            sendTime = sendResponseUp(origRequest, (isCached ? dirLine->getDataLine()->getData() : &responseEvent->getPayload()), true, dirLine->getTimestamp());
            dirLine->setTimestamp(sendTime);
            if (is_debug_event(responseEvent)) printData(&responseEvent->getPayload(), false);
            return DONE;
        case SM_Inv:
            mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());  // TODO this might be a problem if we try to use it
            dirLine->setState(M_Inv);
            return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


CacheAction MESIInternalDirectory::handleFetchResp(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    
    // Check acks needed
    if (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(responseEvent->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) == 0) ? DONE : IGNORE;
    
    bool isCached = dirLine->getDataLine() != NULL;
    if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), 0);    // Update local data if needed
    recordStateEventCount(responseEvent->getCmd(), state);
    
    uint64_t sendTime = 0; 
    switch (state) {
        case S_D:
            dirLine->setState(S);
        case SM_D:
            if (state == SM_D) dirLine->setState(SM);
        case E_D:
            if (state == E_D) dirLine->setState(E);
        case M_D:
            if (state == M_D) dirLine->setState(M);
            if (reqEvent->getCmd() == Command::Fetch) {
                sendResponseDownFromMSHR(responseEvent, (state == M));
            } else if (reqEvent->getCmd() == Command::GetS) {    // GetS
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (is_debug_event(responseEvent)) printData(&responseEvent->getPayload(), false);
            } else {
                debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received FetchResp in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        parent->getName().c_str(), StateString[state], CommandString[(int)reqEvent->getCmd()], responseEvent->getBaseAddr(), getCurrentSimTimeNano());
            }
            break;
        case SI:
            dirLine->removeSharer(responseEvent->getSrc());
            mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());
            if (action == DONE) {
                sendWritebackFromMSHR(Command::PutS, dirLine, reqEvent->getRqstr(), &responseEvent->getPayload());
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case EI:
            if (responseEvent->getDirty()) dirLine->setState(MI);
        case MI:
            if (dirLine->getOwner() == responseEvent->getSrc()) dirLine->clearOwner();
            if (dirLine->isSharer(responseEvent->getSrc())) dirLine->removeSharer(responseEvent->getSrc());
            if (action == DONE) {
                sendWritebackFromMSHR(((dirLine->getState() == EI) ? Command::PutE : Command::PutM), dirLine, parent->getName(), &responseEvent->getPayload());
                if (expectWritebackAck_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case E_InvX:    // FetchXResp for a GetS, FetchInvX, or FlushLine
        case M_InvX:    // FetchXResp for a GetS, FetchInvX, or FlushLine
            if (dirLine->getOwner() == responseEvent->getSrc()) {
                dirLine->clearOwner();
                dirLine->addSharer(responseEvent->getSrc());
            }
            if (!isCached) mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());
            if (reqEvent->getCmd() == Command::FetchInvX) {
                sendResponseDownFromMSHR(responseEvent, (state == M_InvX || responseEvent->getDirty()));
                dirLine->setState(S);
            } else if (reqEvent->getCmd() == Command::FetchInv) {    // External FetchInv raced with our FlushLine, handle it first
                if (dirLine->numSharers() > 0) {
                    invalidateAllSharers(dirLine, reqEvent->getRqstr(), true);
                    (state == M_InvX || responseEvent->getDirty())?  dirLine->setState(M_Inv) : dirLine->setState(E_Inv);
                    return STALL;
                }
                (state == M_InvX || responseEvent->getDirty()) ? dirLine->setState(M) : dirLine->setState(E);
                sendResponseDownFromMSHR(responseEvent, (state == M_InvX || responseEvent->getDirty()));
            } else if (reqEvent->getCmd() == Command::FlushLine) {
                (state == M_InvX || responseEvent->getDirty()) ? dirLine->setState(M) : dirLine->setState(E);
                action = handleFlushLineRequest(reqEvent, dirLine, NULL, true);
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (is_debug_event(responseEvent)) printData(&responseEvent->getPayload(), false);
                if (responseEvent->getDirty() || state == M_InvX) dirLine->setState(M);
                else dirLine->setState(E);
            }
            break;
        case E_Inv: // FetchResp for FetchInv/flush, may also be waiting for acks
        case M_Inv: // FetchResp for FetchInv/flush or GetX, may also be waiting for acks
            if (dirLine->isSharer(responseEvent->getSrc())) dirLine->removeSharer(responseEvent->getSrc());
            if (dirLine->getOwner() == responseEvent->getSrc()) dirLine->clearOwner();
            if (action != DONE) {
                if (responseEvent->getDirty()) dirLine->setState(M_Inv);
                mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());
            } else {
                if (reqEvent->getCmd() == Command::GetX || reqEvent->getCmd() == Command::GetSX) {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    dirLine->setOwner(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setState(M);
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    if (responseEvent->getDirty()) {
                        if (dirLine->getDataLine() != NULL) dirLine->getDataLine()->setData(responseEvent->getPayload(), 0);
                        else mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());
                    }
                    if (responseEvent->getDirty() || state == M_Inv) dirLine->setState(M);
                    else dirLine->setState(E);
                    if (action != DONE) { // Sanity check...
                        debug->fatal(CALL_INFO, -1, "%s, Error: Received a FetchResp to a FlushLineInv but still waiting on more acks. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s. Time = %" PRIu64 "ns\n",
                            parent->getName().c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], responseEvent->getSrc().c_str(), getCurrentSimTimeNano()); 
                    }
                    action = handleFlushLineInvRequest(reqEvent, dirLine, NULL, true);
                } else {
                    sendResponseDownFromMSHR(responseEvent, (state == M_Inv || responseEvent->getDirty()));
                    dirLine->setState(I);
                }
            }
            break;
        case S_Inv:     // Received a FetchInv in S state
        case SM_Inv:    // Received a FetchInv in SM state
            if (dirLine->isSharer(responseEvent->getSrc())) dirLine->removeSharer(responseEvent->getSrc());
            if (action != DONE) {
                mshr_->setDataBuffer(responseEvent->getBaseAddr(), responseEvent->getPayload());
            } else {
                sendResponseDownFromMSHR(responseEvent, false);
                (state == S_Inv) ? dirLine->setState(I) : dirLine->setState(IM);
            }
            break;

        default:
            debug->fatal(CALL_INFO, -1, "%s (dir), Error: Received a FetchResp and state is unhandled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), responseEvent->getBaseAddr(), CommandString[(int)responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], getCurrentSimTimeNano());
    }
    return action;
}


CacheAction MESIInternalDirectory::handleAckInv(MemEvent * ack, CacheLine * dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    recordStateEventCount(ack->getCmd(), state);

    if (dirLine->isSharer(ack->getSrc())) {
        dirLine->removeSharer(ack->getSrc());
    }
    if (is_debug_event(ack)) debug->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    bool isCached = dirLine->getDataLine() != NULL;
    vector<uint8_t> * data = isCached ? dirLine->getDataLine()->getData() : mshr_->getDataBuffer(reqEvent->getBaseAddr());
    uint64_t sendTime = 0; 
    switch (state) {
        case S_Inv: // AckInv for Inv
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, false, true);
                } else {
                    sendAckInv(reqEvent);
                }
                dirLine->setState(I);
            }
            return action;
        case E_Inv: // AckInv for FetchInv, possibly waiting on FetchResp too
        case M_Inv: // AckInv for FetchInv or GetX, possibly on FetchResp or GetXResp too
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, (state == E_Inv), true);
                    dirLine->setState(I);
                } else if (reqEvent->getCmd() == Command::ForceInv) {
                    sendAckInv(reqEvent);
                    dirLine->setState(I);
                } else {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, data, true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    if (is_debug_event(reqEvent)) printData(data, false);
                    dirLine->setState(M);
                }
                mshr_->clearDataBuffer(reqEvent->getBaseAddr());
            }
            return action;
        case SM_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::Inv || reqEvent->getCmd() == Command::ForceInv) {    // Completed Inv so handle
                    if (dirLine->numSharers() > 0) {
                        invalidateAllSharers(dirLine, reqEvent->getRqstr(), true);
                        return STALL;
                    }
                    sendAckInv(reqEvent);
                    dirLine->setState(IM);
                } else if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, false, true);
                    dirLine->setState(IM);
                } else { // Waiting on data for upgrade
                    dirLine->setState(SM);
                    action = IGNORE;
                }
            }
            return action;  
        case SB_Inv:
            if (action == DONE) {
                if (dirLine->numSharers() > 0) {
                    invalidateAllSharers(dirLine, reqEvent->getRqstr(), true);
                    return IGNORE;
                }
                sendAckInv(reqEvent);
                dirLine->setState(I_B);
            }
            return action;
        case SI:
            if (action == DONE) {
                sendWritebackFromMSHR(Command::PutS, dirLine, reqEvent->getRqstr(), data);
                if (expectWritebackAck_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case EI:
            if (action == DONE) {
                sendWritebackFromMSHR(Command::PutE, dirLine, reqEvent->getRqstr(), data);
                if (expectWritebackAck_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case MI:
            if (action == DONE) {
                sendWritebackFromMSHR(Command::PutM, dirLine, reqEvent->getRqstr(), data);
                if (expectWritebackAck_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        default:
            debug->fatal(CALL_INFO,-1,"%s (dir), Error: Received AckInv in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    parent->getName().c_str(), ack->getBaseAddr(), CommandString[(int)ack->getCmd()], ack->getSrc().c_str(), 
                    StateString[state], getCurrentSimTimeNano());

    }
    return action;    // eliminate compiler warning
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface (public)
 *---------------------------------------------------------------------------------------------------------------------*/


void MESIInternalDirectory::invalidateAllSharers(CacheLine * dirLine, string rqstr, bool replay) {
    set<std::string> * sharers = dirLine->getSharers();
    
    uint64_t baseTime = (timestamp_ > dirLine->getTimestamp()) ? timestamp_ : dirLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    bool invSent = false;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv = new MemEvent(parent, dirLine->getBaseAddr(), dirLine->getBaseAddr(), Command::Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
    
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(dirLine->getBaseAddr());
        invSent = true;
        if (is_debug_addr(dirLine->getBaseAddr())) {
            debug->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                dirLine->getBaseAddr(), (*it).c_str(), deliveryTime);
        }
    }
    if (invSent) dirLine->setTimestamp(deliveryTime);
}


void MESIInternalDirectory::invalidateAllSharersAndFetch(CacheLine * cacheLine, string rqstr, bool replay) {
    set<std::string> * sharers = cacheLine->getSharers();
    bool fetched = false;
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    bool invSent = false;

    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv;
        if (fetched) inv = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::Inv);
        else {
            inv = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInv);
            fetched = true;
        }
        inv->setDst(*it);
        inv->setRqstr(rqstr);
        inv->setSize(cacheLine->getSize());
    
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);
        invSent = true;

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

        if (is_debug_addr(cacheLine->getBaseAddr())) {
            debug->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
        }
    }
    
    if (invSent) cacheLine->setTimestamp(deliveryTime);
}


/*
 * If checkFetch is true -> block is not cached
 * Then, if requestor is not already a sharer, we need data!
 */
bool MESIInternalDirectory::invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay, bool uncached) {
    bool sentInv = false;
    set<std::string> * sharers = cacheLine->getSharers();
    bool needFetch = uncached && (sharers->find(rqstr) == sharers->end());
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        if (*it == rqstr) continue;
        MemEvent * inv;
        if (needFetch) {
            inv = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInv);
            needFetch = false;
        } else {
            inv = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::Inv);
        }
        inv->setDst(*it);
        inv->setRqstr(origRqstr);
        inv->setSize(cacheLine->getSize());

        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);
        sentInv = true;

        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
        if (is_debug_addr(cacheLine->getBaseAddr())) {
            debug->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
        }
    }
    if (sentInv) cacheLine->setTimestamp(deliveryTime);
    return sentInv;
}


void MESIInternalDirectory::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInv);
    if (!(cacheLine->getOwner()).empty()) fetch->setDst(cacheLine->getOwner());
    else fetch->setDst(*(cacheLine->getSharers()->begin()));
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
   
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


void MESIInternalDirectory::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


void MESIInternalDirectory::sendFetch(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::Fetch);
    fetch->setDst(*((cacheLine->getSharers())->begin()));
    fetch->setRqstr(rqstr);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending Fetch: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


void MESIInternalDirectory::sendForceInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * inv = new MemEvent(parent, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::ForceInv);
    inv->setDst(cacheLine->getOwner());
    inv->setRqstr(rqstr);
    inv->setSize(cacheLine->getSize());

    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {inv, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending ForceInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload  
 */
void MESIInternalDirectory::sendResponseDown(MemEvent* event, CacheLine * cacheLine, std::vector<uint8_t>* data, bool dirty, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*data);
    if (is_debug_event(event)) printData(data, false);
    responseEvent->setSize(data->size());

    responseEvent->setDirty(dirty);

    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = replay ? baseTime + mshrLatency_ : baseTime + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_event(event)) { 
        debug->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}


void MESIInternalDirectory::sendResponseDownFromMSHR(MemEvent * event, bool dirty) {
    MemEvent * requestEvent = mshr_->lookupFront(event->getBaseAddr());
    MemEvent * responseEvent = requestEvent->makeResponse();
    responseEvent->setPayload(event->getPayload());
    responseEvent->setSize(event->getSize());
    responseEvent->setDirty(dirty);

    uint64_t deliveryTime = timestamp_ + mshrLatency_;
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueue(resp);
    
    if (is_debug_event(event)) {
        debug->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}

void MESIInternalDirectory::sendAckInv(MemEvent * event) {
    MemEvent * ack = event->makeResponse();
    ack->setCmd(Command::AckInv); // Just in case this wasn't an Inv/ForceInv/etc.
    ack->setDst(getDestination(event->getBaseAddr()));
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueue(resp);
    
    if (is_debug_event(ack)) debug->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
}


void MESIInternalDirectory::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = new MemEvent(parent, event->getBaseAddr(), event->getBaseAddr(), Command::AckPut);
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    
    if (is_debug_event(event)) debug->debug(_L3_, "Sending AckPut at cycle = %" PRIu64 "\n", deliveryTime);
}

void MESIInternalDirectory::sendWritebackFromCache(Command cmd, CacheLine * dirLine, string rqstr) {
    MemEvent * writeback = new MemEvent(parent, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    writeback->setSize(dirLine->getSize());
    if (cmd == Command::PutM || writebackCleanBlocks_) {
        writeback->setPayload(*(dirLine->getDataLine()->getData()));
    }
    writeback->setRqstr(rqstr);
    if (cmd == Command::PutM) writeback->setDirty(true);
    uint64_t baseTime = (timestamp_ > dirLine->getTimestamp()) ? timestamp_ : dirLine->getTimestamp();
    uint64_t deliveryTime = baseTime + accessLatency_;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    dirLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(dirLine->getBaseAddr())) debug->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From cache\n", deliveryTime, CommandString[(int)cmd]);
}

void MESIInternalDirectory::sendWritebackFromMSHR(Command cmd, CacheLine * dirLine, string rqstr, vector<uint8_t> * data) {
    MemEvent * writeback = new MemEvent(parent, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    writeback->setSize(dirLine->getSize());
    if (cmd == Command::PutM || writebackCleanBlocks_) {
        writeback->setPayload(*data);
    }
    writeback->setRqstr(rqstr);
    if (cmd == Command::PutM) writeback->setDirty(true);
    uint64_t deliveryTime = timestamp_ + accessLatency_;
    Response resp = {writeback, deliveryTime, packetHeaderBytes + writeback->getPayloadSize()};
    addToOutgoingQueue(resp);
    
    if (is_debug_addr(dirLine->getBaseAddr())) debug->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From MSHR\n", deliveryTime, CommandString[(int)cmd]);
}

void MESIInternalDirectory::sendFlushResponse(MemEvent * requestEvent, bool success) {
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

/**
 *  Forward a flush line request, with or without data
 */
void MESIInternalDirectory::forwardFlushLine(MemEvent * origFlush, CacheLine * dirLine, bool dirty, Command cmd) {
    MemEvent * flush = new MemEvent(parent, origFlush->getBaseAddr(), origFlush->getBaseAddr(), cmd);
    flush->setDst(getDestination(origFlush->getBaseAddr()));
    flush->setRqstr(origFlush->getRqstr());
    flush->setSize(lineSize_);
    uint64_t latency = tagLatency_;
    if (dirty) flush->setDirty(true);
    // Always forward data if available
    if (dirLine) {
        if (dirLine->getDataLine() != NULL) flush->setPayload(*dirLine->getDataLine()->getData());
        else if (mshr_->isHit(origFlush->getBaseAddr())) flush->setPayload(*mshr_->getDataBuffer(origFlush->getBaseAddr()));
        else if (origFlush->getPayloadSize() != 0) flush->setPayload(origFlush->getPayload());
    }
    uint64_t baseTime = timestamp_;
    if (dirLine && dirLine->getTimestamp() > baseTime) baseTime = dirLine->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (dirLine) dirLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_event(origFlush)) {
        debug->debug(_L3_,"Forwarding %s at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", CommandString[(int)cmd], deliveryTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str());
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIInternalDirectory::addToOutgoingQueue(Response& resp) {
    CoherenceController::addToOutgoingQueue(resp);
    recordEventSentDown(resp.event->getCmd());
}

void MESIInternalDirectory::addToOutgoingQueueUp(Response& resp) {
    CoherenceController::addToOutgoingQueueUp(resp);
    recordEventSentUp(resp.event->getCmd());
}

/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/


void MESIInternalDirectory::printData(vector<uint8_t> * data, bool set) {
/*    if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());
    
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");
*/ 
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Statistics recording
 *---------------------------------------------------------------------------------------------------------------------*/


/* Record state of a line at attempted eviction */
void MESIInternalDirectory::recordEvictionState(State state) {
    switch (state) {
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
        case S_Inv:
            stat_evict_SInv->addData(1); 
            break;
        case E_Inv:
            stat_evict_EInv->addData(1); 
            break;
        case M_Inv:
            stat_evict_MInv->addData(1); 
            break;
        case SM_Inv:
            stat_evict_SMInv->addData(1); 
            break;
        case E_InvX:
            stat_evict_EInvX->addData(1); 
            break;
        case M_InvX:
            stat_evict_MInvX->addData(1); 
            break;
        case SI:
            stat_evict_SI->addData(1); 
            break;
        case I_B:
            stat_evict_IB->addData(1);
            break;
        case S_B:
            stat_evict_SB->addData(1);
            break;
        default:
            break; // No error, statistic handling
    }
}


void MESIInternalDirectory::recordStateEventCount(Command cmd, State state) {
    switch (cmd) {
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
            //else if (state == SM) Only because we retried too early
            break;
        case Command::GetSX:
            if (state == I) stat_stateEvent_GetSX_I->addData(1);
            else if (state == S) stat_stateEvent_GetSX_S->addData(1);
            else if (state == E) stat_stateEvent_GetSX_E->addData(1);
            else if (state == M) stat_stateEvent_GetSX_M->addData(1);
            //else if (state == SM) Only because we retried too early
            break;
        case Command::GetSResp:
            if (state == IS) stat_stateEvent_GetSResp_IS->addData(1);
            break;
        case Command::GetXResp:
            if (state == IS) stat_stateEvent_GetXResp_IS->addData(1);
            else if (state == IM) stat_stateEvent_GetXResp_IM->addData(1);
            else if (state == SM) stat_stateEvent_GetXResp_SM->addData(1);
            else if (state == SM_Inv) stat_stateEvent_GetXResp_SMInv->addData(1);
            break;
        case Command::PutS:
            if (state == I) stat_stateEvent_PutS_I->addData(1);
            else if (state == S) stat_stateEvent_PutS_S->addData(1);
            else if (state == E) stat_stateEvent_PutS_E->addData(1);
            else if (state == M) stat_stateEvent_PutS_M->addData(1);
            else if (state == M_Inv) stat_stateEvent_PutS_MInv->addData(1);
            else if (state == E_Inv) stat_stateEvent_PutS_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_PutS_EInvX->addData(1);
            else if (state == S_Inv) stat_stateEvent_PutS_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_PutS_SMInv->addData(1);
            else if (state == MI) stat_stateEvent_PutS_MI->addData(1);
            else if (state == EI) stat_stateEvent_PutS_EI->addData(1);
            else if (state == SI) stat_stateEvent_PutS_SI->addData(1);
            else if (state == I_B) stat_stateEvent_PutS_IB->addData(1);
            else if (state == S_B) stat_stateEvent_PutS_SB->addData(1);
            else if (state == SB_Inv) stat_stateEvent_PutS_SBInv->addData(1);
            else if (state == S_D) stat_stateEvent_PutS_SD->addData(1);
            else if (state == E_D) stat_stateEvent_PutS_ED->addData(1);
            else if (state == M_D) stat_stateEvent_PutS_MD->addData(1);
            else if (state == SM_D) stat_stateEvent_PutS_SMD->addData(1);
            break;
        case Command::PutE:
            if (state == I) stat_stateEvent_PutE_I->addData(1);
            else if (state == E) stat_stateEvent_PutE_E->addData(1);
            else if (state == M) stat_stateEvent_PutE_M->addData(1);
            else if (state == M_Inv) stat_stateEvent_PutE_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_PutE_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_PutE_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_PutE_EInvX->addData(1);
            else if (state == MI) stat_stateEvent_PutE_MI->addData(1);
            else if (state == EI) stat_stateEvent_PutE_EI->addData(1);
            break;
        case Command::PutM:
            if (state == I) stat_stateEvent_PutM_I->addData(1);
            else if (state == E) stat_stateEvent_PutM_E->addData(1);
            else if (state == M) stat_stateEvent_PutM_M->addData(1);
            else if (state == M_Inv) stat_stateEvent_PutM_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_PutM_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_PutM_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_PutM_EInvX->addData(1);
            else if (state == MI) stat_stateEvent_PutM_MI->addData(1);
            else if (state == EI) stat_stateEvent_PutM_EI->addData(1);
            break;
        case Command::Inv:
            if (state == I) stat_stateEvent_Inv_I->addData(1);
            else if (state == S) stat_stateEvent_Inv_S->addData(1);
            else if (state == SM) stat_stateEvent_Inv_SM->addData(1);
            else if (state == M_Inv) stat_stateEvent_Inv_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_Inv_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_Inv_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_Inv_EInvX->addData(1);
            else if (state == S_Inv) stat_stateEvent_Inv_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_Inv_SMInv->addData(1);
            else if (state == SI) stat_stateEvent_Inv_SI->addData(1);
            else if (state == EI) stat_stateEvent_Inv_EI->addData(1);
            else if (state == MI) stat_stateEvent_Inv_MI->addData(1);
            else if (state == S_B) stat_stateEvent_Inv_SB->addData(1);
            else if (state == I_B) stat_stateEvent_Inv_IB->addData(1);
            else if (state == S_D) stat_stateEvent_Inv_SD->addData(1);
            else if (state == E_D) stat_stateEvent_Inv_ED->addData(1);
            else if (state == M_D) stat_stateEvent_Inv_MD->addData(1);
            break;
        case Command::FetchInvX:
            if (state == I) stat_stateEvent_FetchInvX_I->addData(1);
            else if (state == E) stat_stateEvent_FetchInvX_E->addData(1);
            else if (state == M) stat_stateEvent_FetchInvX_M->addData(1);
            else if (state == IS) stat_stateEvent_FetchInvX_IS->addData(1);
            else if (state == IM) stat_stateEvent_FetchInvX_IM->addData(1);
            else if (state == I_B) stat_stateEvent_FetchInvX_IB->addData(1);
            else if (state == S_B) stat_stateEvent_FetchInvX_SB->addData(1);
            break;
        case Command::Fetch:
            if (state == I) stat_stateEvent_Fetch_I->addData(1);
            else if (state == S) stat_stateEvent_Fetch_S->addData(1);
            else if (state == IS) stat_stateEvent_Fetch_IS->addData(1);
            else if (state == IM) stat_stateEvent_Fetch_IM->addData(1);
            else if (state == SM) stat_stateEvent_Fetch_SM->addData(1);
            else if (state == S_D) stat_stateEvent_Fetch_SD->addData(1);
            else if (state == S_Inv) stat_stateEvent_Fetch_SInv->addData(1);
            else if (state == SI) stat_stateEvent_Fetch_SI->addData(1);
            break;
        case Command::FetchInv:
            if (state == I) stat_stateEvent_FetchInv_I->addData(1);
            else if (state == E) stat_stateEvent_FetchInv_E->addData(1);
            else if (state == M) stat_stateEvent_FetchInv_M->addData(1);
            else if (state == IS) stat_stateEvent_FetchInv_IS->addData(1);
            else if (state == IM) stat_stateEvent_FetchInv_IM->addData(1);
            else if (state == M_Inv) stat_stateEvent_FetchInv_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_FetchInv_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_FetchInv_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_FetchInv_EInvX->addData(1);
            else if (state == MI) stat_stateEvent_FetchInv_MI->addData(1);
            else if (state == EI) stat_stateEvent_FetchInv_EI->addData(1);
            else if (state == I_B) stat_stateEvent_FetchInv_IB->addData(1);
            else if (state == S_D) stat_stateEvent_FetchInv_SD->addData(1);
            else if (state == E_D) stat_stateEvent_FetchInv_ED->addData(1);
            else if (state == M_D) stat_stateEvent_FetchInv_MD->addData(1);
            break;
        case Command::FetchResp:
            if (state == M_Inv) stat_stateEvent_FetchResp_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_FetchResp_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_FetchResp_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_FetchResp_EInvX->addData(1);
            else if (state == S_D) stat_stateEvent_FetchResp_SD->addData(1);
            else if (state == E_D) stat_stateEvent_FetchResp_ED->addData(1);
            else if (state == M_D) stat_stateEvent_FetchResp_MD->addData(1);
            else if (state == SM_D) stat_stateEvent_FetchResp_SMD->addData(1);
            else if (state == S_Inv) stat_stateEvent_FetchResp_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_FetchResp_SMInv->addData(1);
            else if (state == MI) stat_stateEvent_FetchResp_MI->addData(1);
            else if (state == EI) stat_stateEvent_FetchResp_EI->addData(1);
            else if (state == SI) stat_stateEvent_FetchResp_SI->addData(1);
            break;
        case Command::FetchXResp:
            if (state == M_Inv) stat_stateEvent_FetchXResp_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_FetchXResp_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_FetchXResp_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_FetchXResp_EInvX->addData(1);
            else if (state == S_D) stat_stateEvent_FetchXResp_SD->addData(1);
            else if (state == E_D) stat_stateEvent_FetchXResp_ED->addData(1);
            else if (state == M_D) stat_stateEvent_FetchXResp_MD->addData(1);
            else if (state == SM_D) stat_stateEvent_FetchXResp_SMD->addData(1);
            else if (state == S_Inv) stat_stateEvent_FetchXResp_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_FetchXResp_SMInv->addData(1);
            else if (state == MI) stat_stateEvent_FetchXResp_MI->addData(1);
            else if (state == EI) stat_stateEvent_FetchXResp_EI->addData(1);
            else if (state == SI) stat_stateEvent_FetchXResp_SI->addData(1);
            break;
        case Command::AckInv:
            if (state == I) stat_stateEvent_AckInv_I->addData(1);
            else if (state == M_Inv) stat_stateEvent_AckInv_MInv->addData(1);
            else if (state == E_Inv) stat_stateEvent_AckInv_EInv->addData(1);
            else if (state == S_Inv) stat_stateEvent_AckInv_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_AckInv_SMInv->addData(1);
            else if (state == MI) stat_stateEvent_AckInv_MI->addData(1);
            else if (state == EI) stat_stateEvent_AckInv_EI->addData(1);
            else if (state == SI) stat_stateEvent_AckInv_SI->addData(1);
            else if (state == SB_Inv) stat_stateEvent_AckInv_SBInv->addData(1);
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
            else if (state == M_Inv) stat_stateEvent_FlushLine_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_FlushLine_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_FlushLine_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_FlushLine_EInvX->addData(1);
            else if (state == S_Inv) stat_stateEvent_FlushLine_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_FlushLine_SMInv->addData(1);
            else if (state == S_D) stat_stateEvent_FlushLine_SD->addData(1);
            else if (state == E_D) stat_stateEvent_FlushLine_ED->addData(1);
            else if (state == M_D) stat_stateEvent_FlushLine_MD->addData(1);
            else if (state == SM_D) stat_stateEvent_FlushLine_SMD->addData(1);
            else if (state == MI) stat_stateEvent_FlushLine_MI->addData(1);
            else if (state == EI) stat_stateEvent_FlushLine_EI->addData(1);
            else if (state == SI) stat_stateEvent_FlushLine_SI->addData(1);
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
            else if (state == M_Inv) stat_stateEvent_FlushLineInv_MInv->addData(1);
            else if (state == M_InvX) stat_stateEvent_FlushLineInv_MInvX->addData(1);
            else if (state == E_Inv) stat_stateEvent_FlushLineInv_EInv->addData(1);
            else if (state == E_InvX) stat_stateEvent_FlushLineInv_EInvX->addData(1);
            else if (state == S_Inv) stat_stateEvent_FlushLineInv_SInv->addData(1);
            else if (state == SM_Inv) stat_stateEvent_FlushLineInv_SMInv->addData(1);
            else if (state == S_D) stat_stateEvent_FlushLineInv_SD->addData(1);
            else if (state == E_D) stat_stateEvent_FlushLineInv_ED->addData(1);
            else if (state == M_D) stat_stateEvent_FlushLineInv_MD->addData(1);
            else if (state == SM_D) stat_stateEvent_FlushLineInv_SMD->addData(1);
            else if (state == MI) stat_stateEvent_FlushLineInv_MI->addData(1);
            else if (state == EI) stat_stateEvent_FlushLineInv_EI->addData(1);
            else if (state == SI) stat_stateEvent_FlushLineInv_SI->addData(1);
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

void MESIInternalDirectory::recordEventSentDown(Command cmd) {
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
        case Command::PutS:
            stat_eventSent_PutS->addData(1);
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
        case Command::FetchResp:
            stat_eventSent_FetchResp->addData(1);
            break;
        case Command::FetchXResp:
            stat_eventSent_FetchXResp->addData(1);
            break;
        case Command::AckInv:
            stat_eventSent_AckInv->addData(1);
            break;
        case Command::NACK:
            stat_eventSent_NACK_down->addData(1);
            break;
        default:
            break;
    }
}


void MESIInternalDirectory::recordEventSentUp(Command cmd) {
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
        case Command::Fetch:
            stat_eventSent_Fetch->addData(1);
            break;
        case Command::FetchInv:
            stat_eventSent_FetchInv->addData(1);
            break;
        case Command::FetchInvX:
            stat_eventSent_FetchInvX->addData(1);
            break;
        case Command::AckPut:
            stat_eventSent_AckPut->addData(1);
            break;
        case Command::NACK:
            stat_eventSent_NACK_up->addData(1);
            break;
        default:
            break;
    }
}
