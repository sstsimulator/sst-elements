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
#include "MESIInternalDirectory.h"
using namespace SST;
using namespace SST::MemHierarchy;

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
    bool collision = (waitingEvent != NULL && (waitingEvent->getCmd() == PutS || waitingEvent->getCmd() == PutE || waitingEvent->getCmd() == PutM));
    if (collision) {    // Note that 'collision' and 'fromDataCache' cannot both be true, don't need to handle that case
        if (state == E && waitingEvent->getDirty()) replacementLine->setState(M);
        if (replacementLine->isSharer(waitingEvent->getSrc())) replacementLine->removeSharer(waitingEvent->getSrc());
        else if (replacementLine->ownerExists()) replacementLine->clearOwner();
        mshr_->setTempData(waitingEvent->getBaseAddr(), waitingEvent->getPayload());
        mshr_->removeFront(waitingEvent->getBaseAddr());
        delete waitingEvent;
    }
    switch (state) {
        case I:
            return DONE;
        case S:
            if (replacementLine->numSharers() > 0 && !fromDataCache) {
                if (isCached || collision) invalidateAllSharers(replacementLine, name_, false);
                else invalidateAllSharersAndFetch(replacementLine, name_, false);    // Fetch needed for PutS
                replacementLine->setState(SI);
                return STALL;
            }
            if (!isCached && !collision) d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
            if (fromDataCache && replacementLine->numSharers() > 0) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
            if (isCached) sendWritebackFromCache(PutS, replacementLine, origRqstr);
            else sendWritebackFromMSHR(PutS, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
            if (replacementLine->numSharers() == 0) replacementLine->setState(I); 
            if (!LL_) mshr_->insertWriteback(wbBaseAddr);
            return DONE;
        case E:
            if (replacementLine->numSharers() > 0 && !fromDataCache) { // May or may not be cached
                if (isCached || collision) invalidateAllSharers(replacementLine, name_, false);
                else invalidateAllSharersAndFetch(replacementLine, name_, false);
                replacementLine->setState(EI);
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) { // Not cached
                sendFetchInv(replacementLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(EI);
                return STALL;
            } else { // Must be cached
                if (!isCached && !collision) 
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(PutE, replacementLine, origRqstr);
                else sendWritebackFromMSHR(PutE, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
                if (replacementLine->numSharers() == 0 && !replacementLine->ownerExists()) replacementLine->setState(I);
                if (!LL_) mshr_->insertWriteback(wbBaseAddr);
                return DONE;
            }
        case M:
            if (replacementLine->numSharers() > 0 && !fromDataCache) {
                if (isCached || collision) invalidateAllSharers(replacementLine, name_, false);
                else invalidateAllSharersAndFetch(replacementLine, name_, false);
                replacementLine->setState(MI);
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) {
                sendFetchInv(replacementLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(MI);
                return STALL;
            } else {
                if (!isCached && !collision) 
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(PutM, replacementLine, origRqstr);
                else sendWritebackFromMSHR(PutM, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
                if (replacementLine->numSharers() == 0 && !replacementLine->ownerExists()) replacementLine->setState(I);
                if (!LL_) mshr_->insertWriteback(wbBaseAddr);
                return DONE;
            }
        default:
	    d_->fatal(CALL_INFO,-1,"%s (dir), Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    name_.c_str(), StateString[state], replacementLine->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/** Handle data requests */
CacheAction MESIInternalDirectory::handleRequest(MemEvent * event, CacheLine * dirLine, bool replay) {
    Command cmd = event->getCmd();
    switch(cmd) {
        case GetS:
            return handleGetSRequest(event, dirLine, replay);
        case GetX:
        case GetSEx:
            return handleGetXRequest(event, dirLine, replay);
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Errror: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning

}


/**
 *  Handle replacement (Put*) requests
 */
CacheAction MESIInternalDirectory::handleReplacement(MemEvent* event, CacheLine* dirLine, MemEvent * reqEvent, bool replay) {
    Command cmd = event->getCmd();
    switch (cmd) {
        case PutS:
            return handlePutSRequest(event, dirLine, reqEvent);
        case PutE:
        case PutM:
            return handlePutMRequest(event, dirLine, reqEvent);
        default:
	    d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received an unrecognized replacement: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
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
CacheAction MESIInternalDirectory::handleInvalidationRequest(MemEvent * event, CacheLine * dirLine, bool replay) {
    

    MemEvent * waitingEvent;
    bool collision = false;
    if (mshr_->pendingWriteback(event->getBaseAddr())) {
        // Case 1: Inv raced with a Put -> treat Inv as the AckPut
        mshr_->removeWriteback(event->getBaseAddr());
        return DONE;
    } else {
        waitingEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
        collision = (waitingEvent != NULL && (waitingEvent->getCmd() == PutS || waitingEvent->getCmd() == PutE || waitingEvent->getCmd() == PutM));
    }

    Command cmd = event->getCmd();
    switch (cmd) {
        case Inv: 
            return handleInv(event, dirLine, replay, collision ? waitingEvent : NULL);
        case Fetch:
            return handleFetch(event, dirLine, replay, collision ? waitingEvent : NULL);
        case FetchInv:
            return handleFetchInv(event, dirLine, replay, collision ? waitingEvent : NULL);
        case FetchInvX:
            return handleFetchInvX(event, dirLine, replay, collision ? waitingEvent : NULL);
        default:
	    d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received an unrecognized invalidation: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
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
        case GetSResp:
        case GetXResp:
            return handleDataResponse(respEvent, dirLine, reqEvent);
        case FetchResp:
        case FetchXResp:
            return handleFetchResp(respEvent, dirLine, reqEvent);
            break;
        case AckInv:
            return handleAckInv(respEvent, dirLine, reqEvent);
        case AckPut:
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            return DONE;    // Retry any events that were stalled for ack
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;    // Eliminate compiler warning
}



bool MESIInternalDirectory::isRetryNeeded(MemEvent * event, CacheLine * dirLine) {
    Command cmd = event->getCmd();
    State state = dirLine ? dirLine->getState() : I;
    
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
            return true;
        case PutS:
        case PutE:
        case PutM:
            if (!LL_ && !mshr_->pendingWriteback(event->getBaseAddr())) return false;
            return true;
        case FetchInvX:
            if (state == I) return false;
            if (dirLine->getOwner() != event->getDst()) return false;
            return true;
        case FetchInv:
            if (state == I) return false;
            if ((dirLine->getOwner() != event->getDst()) && !dirLine->isSharer(event->getDst())) return false;
            return true;
        case Fetch:
        case Inv:
            if (state == I) return false;
            if (!dirLine->isSharer(event->getDst())) return false;
            return true;
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received NACK for unrecognized event: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
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
    if (cmd == GetSEx) cmd = GetX;  // for our purposes these are equal

    if (state == I) return 1;
    if (event->isPrefetch() && event->getRqstr() == name_) return 0;
    
    switch (state) {
        case S:
            if (cmd == GetS) return 0;
            return 2;
        case E:
        case M:
            if (cacheLine->ownerExists()) return 3;
            if (cmd == GetS) return 0; 
            if (cmd == GetX) {
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
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == name_));
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
            if (!shouldRespond) return DONE;
            if (isCached) {
                dirLine->addSharer(event->getSrc());
                sendTime = sendResponseUp(event, S, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
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
            if (!shouldRespond) return DONE;
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) dirLine->setState(E_InvX);
                else dirLine->setState(M_InvX);
                return STALL;
            } else if (isCached) {
                if (protocol_ && dirLine->numSharers() == 0) {
                    sendTime = sendResponseUp(event, E, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
                    dirLine->setOwner(event->getSrc());
                    dirLine->setTimestamp(sendTime);
                } else {
                    sendTime = sendResponseUp(event, S, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());
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
            d_->fatal(CALL_INFO,-1,"%s (dir), Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

    }
    return STALL;    // eliminate compiler warning
}


/**
 *  Handle GetX and GetSEx (read-lock) requests
 *  Deallocate on hits
 */
CacheAction MESIInternalDirectory::handleGetXRequest(MemEvent* event, CacheLine* dirLine, bool replay) {
    State state = dirLine->getState();
    Command cmd = event->getCmd();
    if (state != SM) recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;
    uint64_t sendTime = 0;
    switch (state) {
        case I:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
            sendTime = forwardMessage(event, dirLine->getBaseAddr(), lineSize_, 0, &event->getPayload());
            dirLine->setState(IM);
            dirLine->setTimestamp(sendTime);
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
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
            if (isCached) sendTime = sendResponseUp(event, M, dirLine->getDataLine()->getData(), replay, dirLine->getTimestamp());  // is an upgrade request, requestor has data already
            else sendTime = sendResponseUp(event, M, NULL, replay, dirLine->getTimestamp());
            dirLine->setTimestamp(sendTime);
            // TODO DEALLOCATE dataline
            return DONE;
        case SM:
            return STALL;   // retried this request too soon (TODO fix so we don't even attempt retry)!
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
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
        dirLine->getDataLine()->setData(event->getPayload(), event);
        printData(dirLine->getDataLine()->getData(), true);
    } else if (mshr_->isHit(dirLine->getBaseAddr())) mshr_->setTempData(dirLine->getBaseAddr(), event->getPayload());
    
    uint64_t sendTime = 0;

    CacheAction action = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) ? DONE : IGNORE;
    switch(state) {
        case I:
        case S:
        case E:
        case M:
            sendWritebackAck(event);
            return action;
        case SI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutS, dirLine, reqEvent->getRqstr(), &event->getPayload());
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case EI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case MI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case S_Inv: // PutS raced with Inv request
            if (action == DONE) {
                if (reqEvent->getCmd() == Inv) {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                } else {
                    sendResponseDownFromMSHR(event, false);
                }
                dirLine->setState(I);
            }
            return action;
        case S_D: // PutS raced with Fetch
            if (action == DONE) {
                dirLine->setState(S);
                if (reqEvent->getCmd() == Fetch) {
                    if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                        sendWritebackFromMSHR(PutS, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {    // GetS
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    dirLine->addSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, S, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
                } else {
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                            name_.c_str(), StateString[state], CommandString[reqEvent->getCmd()], event->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
                }
            }
            return action;
        case E_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, dirLine, &event->getPayload(), event->getDirty(), true);
                    dirLine->setState(I);
                }
            }
            return action;
        case E_D: // PutS raced with Fetch from GetS
            if (action == DONE) {
                dirLine->setState(E);
                if (reqEvent->getCmd() == Fetch) {
                    if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                        sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    if (dirLine->numSharers() == 0) {
                        dirLine->setOwner(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, E, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    } else {
                        dirLine->addSharer(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, S, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    }
                    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
                } else {
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                            name_.c_str(), StateString[state], CommandString[reqEvent->getCmd()], event->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
                }
            }
            return action;
        case E_InvX: // PutS raced with Fetch from FetchInvX
            if (action == DONE) {
                dirLine->setState(S);
                if (reqEvent->getCmd() == FetchInvX) {
                    if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                        sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else {
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                            name_.c_str(), StateString[state], CommandString[reqEvent->getCmd()], event->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
                }
            }
            return action;
        case M_Inv: // PutS raced with AckInv from GetX, PutS raced with AckInv from FetchInv
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, dirLine, &event->getPayload(), true, true);
                    dirLine->setState(I);
                } else {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, M, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(&event->getPayload(), false);
                    dirLine->setState(M);
                }
            }
            return action;
        case M_D:   // PutS raced with Fetch from GetS
            if (action == DONE) {
                dirLine->setState(M);
                if (reqEvent->getCmd() == Fetch) {
                    if (dirLine->getDataLine() == NULL && dirLine->numSharers() == 0) {
                        sendWritebackFromMSHR(PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                    if (dirLine->numSharers() == 0) {
                        dirLine->setOwner(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, E, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    } else {
                        dirLine->addSharer(reqEvent->getSrc());
                        sendTime = sendResponseUp(reqEvent, S, &event->getPayload(), true, dirLine->getTimestamp());
                        dirLine->setTimestamp(sendTime);
                    }
                    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
                } else {
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received PutS in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                            name_.c_str(), StateString[state], CommandString[reqEvent->getCmd()], event->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
                }
            }
            return action;
        case SM_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Inv) {    // Completed Inv so handle
                    if (dirLine->numSharers() > 0) {
                        invalidateAllSharers(dirLine, event->getRqstr(), true);
                        return IGNORE;
                    }
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                    dirLine->setState(IM);
                } else if (reqEvent->getCmd() == FetchInv) {
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
            }
            return action;  
        case SM_D:
            if (action == DONE && reqEvent->getCmd() == Fetch) {
                sendResponseDownFromMSHR(event, false);
                dirLine->setState(SM);
            } 
            return action;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received PutS in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(),
                    StateString[state], ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return action; // eliminate compiler warning
}


/* CacheAction return value indicates whether the racing action completed (reqEvent). PutMs always complete! */
CacheAction MESIInternalDirectory::handlePutMRequest(MemEvent * event, CacheLine * dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);

    bool isCached = dirLine->getDataLine() != NULL;
    if (isCached) dirLine->getDataLine()->setData(event->getPayload(), event);
    else if (mshr_->isHit(dirLine->getBaseAddr())) mshr_->setTempData(dirLine->getBaseAddr(), event->getPayload());

    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());

    uint64_t sendTime = 0;

    switch (state) {
        case E:
            if (event->getDirty()) dirLine->setState(M);
        case M:
            dirLine->clearOwner();
            sendWritebackAck(event);
            if (!isCached) {
                sendWritebackFromMSHR(((dirLine->getState() == E) ? PutE : PutM), dirLine, event->getRqstr(), &event->getPayload());
                if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case EI:    // Evicting this block anyways
            if (event->getDirty()) dirLine->setState(MI);
        case MI:
            dirLine->clearOwner();
            sendWritebackFromMSHR(((dirLine->getState() == EI) ? PutE : PutM), dirLine, name_, &event->getPayload());
            if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
            dirLine->setState(I);
            break;
        case E_InvX:
            dirLine->clearOwner();
            if (reqEvent->getCmd() == FetchInvX) {
                if (!isCached) {
                    sendWritebackFromMSHR(event->getDirty() ? PutM : PutE, dirLine, event->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                    if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    sendResponseDownFromMSHR(event, (event->getCmd() == PutM));
                    dirLine->setState(S);
                }
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                if (protocol_) {
                    sendTime = sendResponseUp(reqEvent, E, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendTime = sendResponseUp(reqEvent, S, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->addSharer(reqEvent->getSrc());
                }
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
                if (event->getDirty()) dirLine->setState(M);
                else dirLine->setState(E);
            }
            return DONE;
        case M_InvX:
            dirLine->clearOwner();
            if (reqEvent->getCmd() == FetchInvX) {
                if (!isCached) {
                    sendWritebackFromMSHR(PutM, dirLine, event->getRqstr(), &event->getPayload());
                    dirLine->setState(I);
                    if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    sendResponseDownFromMSHR(event, true);
                    dirLine->setState(S);
                }
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->setState(M);
                if (protocol_) {
                    sendTime = sendResponseUp(reqEvent, E, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendTime = sendResponseUp(reqEvent, S, &event->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->addSharer(reqEvent->getSrc());
                }
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
            }
            return DONE;
        case E_Inv:
            if (event->getCmd() == PutM) dirLine->setState(M_Inv);
        case M_Inv: // PutM raced with FetchInv to owner
            dirLine->clearOwner();
            if (reqEvent->getCmd() == GetX || reqEvent->getCmd() == GetSEx) {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                dirLine->setState(M);
                sendTime = sendResponseUp(reqEvent, M, &event->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                dirLine->setOwner(reqEvent->getSrc());
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
            } else { /* Cmd == Fetch */
                sendResponseDownFromMSHR(event, (dirLine->getState() == M_Inv));
                dirLine->setState(I);
            }
            return DONE;
        default:
    	    d_->fatal(CALL_INFO, -1, "%s, Error: Updating data but cache is not in E or M state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 *  Handler for 'Inv' requests
 *  Invalidate sharers if needed
 *  Send AckInv if no sharers exist
 */
CacheAction MESIInternalDirectory::handleInv(MemEvent* event, CacheLine* dirLine, bool replay, MemEvent * collisionEvent) {
    
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    switch(state) {
        case S:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                dirLine->setState(S_Inv);
                while (collisionEvent != NULL) {
                    mshr_->removeFront(event->getBaseAddr());   // We've sent an inv to them so no need for AckPut
                    delete collisionEvent;
                    collisionEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
                    mshr_->decrementAcksNeeded(event->getBaseAddr());
                    if (collisionEvent == NULL || collisionEvent->getCmd() != PutS) collisionEvent = NULL;
                }
                if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) return STALL;
            }
            sendAckInv(event->getBaseAddr(), event->getRqstr());
            dirLine->setState(I);
            return DONE;
        case SM:
            if (dirLine->numSharers() > 0) {
                invalidateAllSharers(dirLine, event->getRqstr(), replay);
                dirLine->setState(SM_Inv);
                while (collisionEvent != NULL) {
                    mshr_->removeFront(event->getBaseAddr());   // We've sent an inv to them so no need for AckPut
                    delete collisionEvent;
                    collisionEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
                    mshr_->decrementAcksNeeded(event->getBaseAddr());
                    if (collisionEvent == NULL || collisionEvent->getCmd() != PutS) collisionEvent = NULL;
                }
                if (mshr_->getAcksNeeded(event->getBaseAddr())) return STALL;
            }
            sendAckInv(event->getBaseAddr(), event->getRqstr());
            dirLine->setState(IM);
            return DONE;
        case SI:
        case S_Inv: // PutS in progress, stall this Inv for that
        case S_D:   // Waiting for a GetS to resolve, stall until it does
            return BLOCK;
        case SM_Inv:    // Waiting on GetSResp, stall this Inv until invacks come back
            return STALL;
        default:
	    d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received an invalidation in an unhandled state: %s. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
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
            d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received Fetch but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handler for FetchInv requests
 *  Invalidate owner and/or sharers if needed
 *  Send FetchResp if no further invalidations are needed
 */
CacheAction MESIInternalDirectory::handleFetchInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent) {
    State state = dirLine->getState();
    recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;
    bool collision = collisionEvent != NULL;
    if (collision) {   // Treat the replacement as if it had already occured/raced with an earlier FetchInv
        if (dirLine->isSharer(collisionEvent->getSrc())) dirLine->removeSharer(collisionEvent->getSrc());
        if (dirLine->ownerExists()) dirLine->clearOwner();
        mshr_->setTempData(collisionEvent->getBaseAddr(), collisionEvent->getPayload());
        if (state == E && collisionEvent->getDirty()) dirLine->setState(M);
        state = M;
        mshr_->removeFront(dirLine->getBaseAddr());
        delete collisionEvent;
    }

    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case S:
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(S_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL && !collision) d_->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    name_.c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getTempData(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
            dirLine->setState(I);
            return DONE;
        case SM:
            if (dirLine->numSharers() > 0) {
                if (isCached || collision) invalidateAllSharers(dirLine, event->getRqstr(), replay);
                else invalidateAllSharersAndFetch(dirLine, event->getRqstr(), replay);
                dirLine->setState(SM_Inv);
                return STALL;
            }
            if (dirLine->getDataLine() == NULL && !collision) d_->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    name_.c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getTempData(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
            dirLine->setState(IM);
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
            if (dirLine->getDataLine() == NULL && !collision) d_->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    name_.c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getTempData(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), false, replay);
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
            if (dirLine->getDataLine() == NULL && !collision) d_->fatal(CALL_INFO, -1, "Error: (%s) An uncached block must have either owners or sharers. Addr = 0x%" PRIx64 ", detected at FetchInv, State = %s\n", 
                    name_.c_str(), event->getAddr(), StateString[state]);
            sendResponseDown(event, dirLine, collision ? mshr_->getTempData(dirLine->getBaseAddr()) : dirLine->getDataLine()->getData(), true, replay);
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
            return BLOCK;
        default:
            d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
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
    bool collision = collisionEvent != NULL;
    if (collision) {   // Treat the replacement as if it had already occured/raced with an earlier FetchInv
        if (state == E && collisionEvent->getDirty()) dirLine->setState(M);
        state = M;
    }

    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case E:
            if (collision) {
                if (dirLine->ownerExists()) {
                    dirLine->clearOwner();
                    dirLine->addSharer(collisionEvent->getSrc());
                    collisionEvent->setCmd(PutS);   // TODO there's probably a cleaner way to do this...and a safer/better way!
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
                    collisionEvent->setCmd(PutS);   // TODO there's probably a cleaner way to do this...and a safer/better way!
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
            return BLOCK;
        default:
            d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
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
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    bool isCached = dirLine->getDataLine() != NULL;
    uint64_t sendTime = 0;
    switch (state) {
        case IS:
            if (responseEvent->getGrantedState() == E) dirLine->setState(E);
            else dirLine->setState(S);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), responseEvent);
            if (!shouldRespond) return DONE;
            if (dirLine->getState() == E) dirLine->setOwner(origRequest->getSrc());
            else dirLine->addSharer(origRequest->getSrc());
            sendTime = sendResponseUp(origRequest, dirLine->getState(), &responseEvent->getPayload(), true, dirLine->getTimestamp());
            dirLine->setTimestamp(sendTime);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(&responseEvent->getPayload(), false);
            return DONE;
        case IM:
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), responseEvent);
        case SM:
            dirLine->setState(M);
            dirLine->setOwner(origRequest->getSrc());
            if (dirLine->isSharer(origRequest->getSrc())) dirLine->removeSharer(origRequest->getSrc());
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            sendTime = sendResponseUp(origRequest, M, (isCached ? dirLine->getDataLine()->getData() : &responseEvent->getPayload()), true, dirLine->getTimestamp());
            dirLine->setTimestamp(sendTime);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(&responseEvent->getPayload(), false);
            return DONE;
        case SM_Inv:
            mshr_->setTempData(responseEvent->getBaseAddr(), responseEvent->getPayload());  // TODO this might be a problem if we try to use it
            dirLine->setState(M_Inv);
            return STALL;
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


CacheAction MESIInternalDirectory::handleFetchResp(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    
    // Check acks needed
    if (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(responseEvent->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) == 0) ? DONE : IGNORE;
    bool isCached = dirLine->getDataLine() != NULL;
    if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), responseEvent);    // Update local data if needed
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
            if (reqEvent->getCmd() == Fetch) {
                sendResponseDownFromMSHR(responseEvent, (state == M));
            } else if (reqEvent->getCmd() == GetS) {    // GetS
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, S, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(&responseEvent->getPayload(), false);
            } else {
                d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received FetchResp in state %s but stalled request has command %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n",
                        name_.c_str(), StateString[state], CommandString[reqEvent->getCmd()], responseEvent->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
            }
            break;
        case SI:
            dirLine->removeSharer(responseEvent->getSrc());
            mshr_->setTempData(responseEvent->getBaseAddr(), responseEvent->getPayload());
            if (action == DONE) {
                sendWritebackFromMSHR(PutS, dirLine, reqEvent->getRqstr(), &responseEvent->getPayload());
                if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case EI:
            if (responseEvent->getDirty()) dirLine->setState(MI);
        case MI:
            if (dirLine->getOwner() == responseEvent->getSrc()) dirLine->clearOwner();
            if (dirLine->isSharer(responseEvent->getSrc())) dirLine->removeSharer(responseEvent->getSrc());
            if (action == DONE) {
                sendWritebackFromMSHR(((dirLine->getState() == EI) ? PutE : PutM), dirLine, name_, &responseEvent->getPayload());
                if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case E_InvX:    // FetchXResp for a GetS or FetchInvX
        case M_InvX:    // FetchXResp for FetchInvX or GetS
            if (dirLine->getOwner() == responseEvent->getSrc()) {
                dirLine->clearOwner();
                dirLine->addSharer(responseEvent->getSrc());
            }
            if (reqEvent->getCmd() == FetchInvX) {
                sendResponseDownFromMSHR(responseEvent, (state == M_InvX || responseEvent->getDirty()));
                dirLine->setState(S);
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                dirLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, S, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                dirLine->setTimestamp(sendTime);
                if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(&responseEvent->getPayload(), false);
                if (responseEvent->getDirty() || state == M_InvX) dirLine->setState(M);
                else dirLine->setState(E);
            }
            break;
        case E_Inv: // FetchResp for FetchInv, may also be waiting for acks
        case M_Inv: // FetchResp for FetchInv or GetX, may also be waiting for acks
            if (dirLine->isSharer(responseEvent->getSrc())) dirLine->removeSharer(responseEvent->getSrc());
            if (dirLine->getOwner() == responseEvent->getSrc()) dirLine->clearOwner();
            if (action != DONE) {
                if (responseEvent->getDirty()) dirLine->setState(M_Inv);
                mshr_->setTempData(responseEvent->getBaseAddr(), responseEvent->getPayload());
            } else {
                if (reqEvent->getCmd() == GetX || reqEvent->getCmd() == GetSEx) {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    dirLine->setOwner(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, M, &responseEvent->getPayload(), true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    dirLine->setState(M);
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
                mshr_->setTempData(responseEvent->getBaseAddr(), responseEvent->getPayload());
            } else {
                sendResponseDownFromMSHR(responseEvent, false);
                (state == S_Inv) ? dirLine->setState(I) : dirLine->setState(IM);
            }
            break;

        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received a FetchResp and state is unhandled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return action;
}


CacheAction MESIInternalDirectory::handleAckInv(MemEvent * ack, CacheLine * dirLine, MemEvent * reqEvent) {
    State state = dirLine->getState();
    recordStateEventCount(ack->getCmd(), state);

    if (dirLine->isSharer(ack->getSrc())) {
        dirLine->removeSharer(ack->getSrc());
    }
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == ack->getBaseAddr()) d_->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
#endif
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    bool isCached = dirLine->getDataLine() != NULL;
    vector<uint8_t> * data = isCached ? dirLine->getDataLine()->getData() : mshr_->getTempData(reqEvent->getBaseAddr());
    uint64_t sendTime = 0; 
    switch (state) {
        case S_Inv: // AckInv for Inv
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, false, true);
                } else {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                }
                dirLine->setState(I);
            }
            return action;
        case E_Inv: // AckInv for FetchInv, possibly waiting on FetchResp too
        case M_Inv: // AckInv for FetchInv or GetX, possibly on FetchResp or GetXResp too
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, (state == E_Inv), true);
                    dirLine->setState(I);
                } else {
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, M, data, true, dirLine->getTimestamp());
                    dirLine->setTimestamp(sendTime);
                    if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(data, false);
                    dirLine->setState(M);
                }
                mshr_->clearTempData(reqEvent->getBaseAddr());
            }
            return action;
        case SM_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Inv) {    // Completed Inv so handle
                    if (dirLine->numSharers() > 0) {
                        invalidateAllSharers(dirLine, reqEvent->getRqstr(), true);
                        return STALL;
                    }
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                    dirLine->setState(IM);
                } else if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, dirLine, data, false, true);
                    dirLine->setState(IM);
                } else { // Waiting on data for upgrade
                    dirLine->setState(SM);
                    action = IGNORE;
                }
            }
            return action;  
        case SI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutS, dirLine, reqEvent->getRqstr(), data);
                if (!LL_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case EI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), data);
                if (!LL_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case MI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutM, dirLine, reqEvent->getRqstr(), data);
                if (!LL_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        default:
            d_->fatal(CALL_INFO,-1,"%s (dir), Error: Received AckInv in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), ack->getBaseAddr(), CommandString[ack->getCmd()], ack->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

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
        MemEvent * inv = new MemEvent((Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
    
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(dirLine->getBaseAddr());
        invSent = true;
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                dirLine->getBaseAddr(), (*it).c_str(), deliveryTime);
#endif
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
        if (fetched) inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        else {
            inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
            fetched = true;
        }
        inv->setDst(*it);
        inv->setRqstr(rqstr);
        inv->setSize(cacheLine->getSize());
    
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);
        invSent = true;

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
#endif
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
            inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
            needFetch = false;
        } else {
            inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        }
        inv->setDst(*it);
        inv->setRqstr(origRqstr);
        inv->setSize(cacheLine->getSize());

        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);
        sentInv = true;

        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
#endif
    }
    if (sentInv) cacheLine->setTimestamp(deliveryTime);
    return sentInv;
}


void MESIInternalDirectory::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
    if (!(cacheLine->getOwner()).empty()) fetch->setDst(cacheLine->getOwner());
    else fetch->setDst(*(cacheLine->getSharers()->begin()));
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
   
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
#endif
}


void MESIInternalDirectory::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
#endif
}


void MESIInternalDirectory::sendFetch(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Fetch);
    fetch->setDst(*((cacheLine->getSharers())->begin()));
    fetch->setRqstr(rqstr);
    
    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending Fetch: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
#endif
}


/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload  
 */
void MESIInternalDirectory::sendResponseDown(MemEvent* event, CacheLine * cacheLine, std::vector<uint8_t>* data, bool dirty, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*data);
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(data, false);
    responseEvent->setSize(data->size());

    responseEvent->setDirty(dirty);

    uint64_t baseTime = (timestamp_ > cacheLine->getTimestamp()) ? timestamp_ : cacheLine->getTimestamp();
    uint64 deliveryTime = replay ? baseTime + mshrLatency_ : baseTime + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) { 
        d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
#endif
}


void MESIInternalDirectory::sendResponseDownFromMSHR(MemEvent * event, bool dirty) {
    MemEvent * requestEvent = mshr_->lookupFront(event->getBaseAddr());
    MemEvent * responseEvent = requestEvent->makeResponse();
    responseEvent->setPayload(event->getPayload());
    responseEvent->setSize(event->getSize());
    responseEvent->setDirty(dirty);

    uint64_t deliveryTime = timestamp_ + mshrLatency_;
    Response resp = {responseEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
        d_->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
#endif
}

void MESIInternalDirectory::sendAckInv(Addr baseAddr, string origRqstr) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, AckInv);
    ack->setDst(getDestination(baseAddr));
    ack->setRqstr(origRqstr);
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueue(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
#endif
}


void MESIInternalDirectory::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, event->getBaseAddr(), event->getBaseAddr(), AckPut);
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueueUp(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_, "Sending AckPut at cycle = %" PRIu64 "\n", deliveryTime);
#endif
}

void MESIInternalDirectory::sendWritebackFromCache(Command cmd, CacheLine * dirLine, string rqstr) {
    MemEvent * writeback = new MemEvent((SST::Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    writeback->setSize(dirLine->getSize());
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setPayload(*(dirLine->getDataLine()->getData()));
    }
    writeback->setRqstr(rqstr);
    if (cmd == PutM) writeback->setDirty(true);
    uint64_t baseTime = (timestamp_ > dirLine->getTimestamp()) ? timestamp_ : dirLine->getTimestamp();
    uint64_t deliveryTime = baseTime + accessLatency_;
    Response resp = {writeback, deliveryTime, false};
    addToOutgoingQueue(resp);
    dirLine->setTimestamp(deliveryTime);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From cache\n", deliveryTime, CommandString[cmd]);
#endif
}

void MESIInternalDirectory::sendWritebackFromMSHR(Command cmd, CacheLine * dirLine, string rqstr, vector<uint8_t> * data) {
    MemEvent * writeback = new MemEvent((SST::Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    writeback->setSize(dirLine->getSize());
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setPayload(*data);
    }
    writeback->setRqstr(rqstr);
    if (cmd == PutM) writeback->setDirty(true);
    uint64_t deliveryTime = timestamp_ + accessLatency_;
    Response resp = {writeback, deliveryTime, false};
    addToOutgoingQueue(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From MSHR\n", deliveryTime, CommandString[cmd]);
#endif
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

