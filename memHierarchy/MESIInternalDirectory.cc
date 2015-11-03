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
CacheAction MESIInternalDirectory::handleEviction(CacheLine* replacementLine, uint32_t groupId, string origRqstr, bool fromDataCache) {
    State state = replacementLine->getState();
    setGroupId(groupId);
    
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
                evictionRequiredInv_++;
                return STALL;
            }
            if (!isCached && !collision) d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
            if (fromDataCache && replacementLine->numSharers() > 0) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
            if (isCached) sendWritebackFromCache(PutS, replacementLine, origRqstr);
            else sendWritebackFromMSHR(PutS, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
            inc_EvictionPUTSReqSent();
            if (replacementLine->numSharers() == 0) replacementLine->setState(I); 
            if (!LL_) mshr_->insertWriteback(wbBaseAddr);
            return DONE;
        case E:
            if (replacementLine->numSharers() > 0 && !fromDataCache) { // May or may not be cached
                if (isCached || collision) invalidateAllSharers(replacementLine, name_, false);
                else invalidateAllSharersAndFetch(replacementLine, name_, false);
                replacementLine->setState(EI);
                evictionRequiredInv_++;
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) { // Not cached
                sendFetchInv(replacementLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(EI);
                evictionRequiredInv_++;
                return STALL;
            } else { // Must be cached
                if (!isCached && !collision) 
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(PutE, replacementLine, origRqstr);
                else sendWritebackFromMSHR(PutE, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
                inc_EvictionPUTEReqSent();
                if (replacementLine->numSharers() == 0 && !replacementLine->ownerExists()) replacementLine->setState(I);
                if (!LL_) mshr_->insertWriteback(wbBaseAddr);
                return DONE;
            }
        case M:
            if (replacementLine->numSharers() > 0 && !fromDataCache) {
                if (isCached || collision) invalidateAllSharers(replacementLine, name_, false);
                else invalidateAllSharersAndFetch(replacementLine, name_, false);
                replacementLine->setState(MI);
                evictionRequiredInv_++;
                return STALL;
            } else if (replacementLine->ownerExists() && !fromDataCache) {
                sendFetchInv(replacementLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                replacementLine->setState(MI);
                evictionRequiredInv_++;
                return STALL;
            } else {
                if (!isCached && !collision) 
                    d_->fatal(CALL_INFO, -1, "%s (dir), Error: evicting uncached block with no sharers or owner. Addr = 0x%" PRIx64 ", State = %s\n", name_.c_str(), replacementLine->getBaseAddr(), StateString[state]);
                if (fromDataCache && (replacementLine->numSharers() > 0 || replacementLine->ownerExists())) return DONE; // lazy deallocation - we don't need to do anything if the block exists elsewhere
                if (isCached) sendWritebackFromCache(PutM, replacementLine, origRqstr);
                else sendWritebackFromMSHR(PutM, replacementLine, origRqstr, mshr_->getTempData(wbBaseAddr));
                inc_EvictionPUTMReqSent();
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
    setGroupId(event->getGroupId());
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
    setGroupId(event->getGroupId());
    Command cmd = event->getCmd();
    switch (cmd) {
        case PutS:
            inc_PUTSReqsReceived();
            return handlePutSRequest(event, dirLine, reqEvent);
        case PutE:
            inc_PUTEReqsReceived();
            return handlePutMRequest(event, dirLine, reqEvent);
        case PutM:
            inc_PUTMReqsReceived();
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
    
    setGroupId(event->getGroupId());

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
            stateStats_[respEvent->getCmd()][I]++;
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            return DONE;    // Retry any events that were stalled for ack
        default:
            d_->fatal(CALL_INFO, -1, "%s (dir), Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;    // Eliminate compiler warning
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
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);    
    bool isCached = dirLine->getDataLine() != NULL;

    switch (state) {
        case I:
            forwardMessage(event, dirLine->getBaseAddr(), lineSize_, NULL);
            inc_GETSMissIS(event);
            dirLine->setState(IS);
            return STALL;
        case S:
            inc_GETSHit(event);
            if (!shouldRespond) return DONE;
            if (isCached) {
                dirLine->addSharer(event->getSrc());
                sendResponseUp(event, S, dirLine->getDataLine()->getData(), replay);
                return DONE;
            } 
            sendFetch(dirLine, event->getRqstr(), replay);
            mshr_->incrementAcksNeeded(event->getBaseAddr());
            dirLine->setState(S_D);     // Fetch in progress, block incoming invalidates/fetches/etc.
            return STALL;
        case E:
        case M:
            inc_GETSHit(event);
            if (!shouldRespond) return DONE;
            if (dirLine->ownerExists()) {
                sendFetchInvX(dirLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) dirLine->setState(E_InvX);
                else dirLine->setState(M_InvX);
                return STALL;
            } else if (isCached) {
                if (protocol_ && dirLine->numSharers() == 0) {
                    sendResponseUp(event, E, dirLine->getDataLine()->getData(), replay);
                    dirLine->setOwner(event->getSrc());
                } else {
                    sendResponseUp(event, S, dirLine->getDataLine()->getData(), replay);
                    dirLine->addSharer(event->getSrc());
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
    stateStats_[cmd][state]++;
    if (state != SM) recordStateEventCount(event->getCmd(), state);    
    
    bool isCached = dirLine->getDataLine() != NULL;

    switch (state) {
        case I:
            inc_GETXMissIM(event);
            forwardMessage(event, dirLine->getBaseAddr(), lineSize_, &event->getPayload());
            dirLine->setState(IM);
            return STALL;
        case S:
            inc_GETXMissSM(event);
            forwardMessage(event, dirLine->getBaseAddr(), lineSize_, &event->getPayload());
            if (invalidateSharersExceptRequestor(dirLine, event->getSrc(), event->getRqstr(), replay, false)) {
                dirLine->setState(SM_Inv);
            } else {
                dirLine->setState(SM);
            }
            return STALL;
        case E:
            dirLine->setState(M);
        case M:
            if (cmd == GetSEx) inc_GetSExReqsReceived(replay);
            inc_GETXHit(event);

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
            if (isCached) sendResponseUp(event, M, dirLine->getDataLine()->getData(), replay);  // is an upgrade request, requestor has data already
            else sendResponseUp(event, M, NULL, replay);
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
    stateStats_[event->getCmd()][state]++;
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
                inc_EvictionPUTSReqSent();
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case EI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), &event->getPayload());
                inc_EvictionPUTEReqSent();
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case MI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutM, dirLine, reqEvent->getRqstr(), &event->getPayload());
                inc_EvictionPUTMReqSent();
                if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                dirLine->setState(I);
            }
            return action;
        case S_Inv: // PutS raced with Inv request
            if (action == DONE) {
                if (reqEvent->getCmd() == Inv) {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                    inc_InvalidatePUTSReqSent();
                } else {
                    sendResponseDownFromMSHR(event, false);
                    inc_FetchInvReqSent();
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
                        inc_EvictionPUTSReqSent();
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {    // GetS
                    inc_GETSHit(reqEvent);
                    dirLine->addSharer(reqEvent->getSrc());
                    sendResponseUp(reqEvent, S, &event->getPayload(), true);
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
                    inc_FetchInvReqSent();
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
                        inc_EvictionPUTEReqSent();
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {
                    inc_GETSHit(reqEvent);
                    if (dirLine->numSharers() == 0) {
                        dirLine->setOwner(reqEvent->getSrc());
                        sendResponseUp(reqEvent, E, &event->getPayload(), true);
                    } else {
                        dirLine->addSharer(reqEvent->getSrc());
                        sendResponseUp(reqEvent, S, &event->getPayload(), true);
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
                        inc_EvictionPUTEReqSent();
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
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, dirLine, &event->getPayload(), true, true);
                    dirLine->setState(I);
                } else {
                    if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                    inc_GETXHit(reqEvent);
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendResponseUp(reqEvent, M, &event->getPayload(), true);
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
                        inc_EvictionPUTMReqSent();
                        dirLine->setState(I);
                    } else {
                        sendResponseDownFromMSHR(event, false);
                    }
                } else if (reqEvent->getCmd() == GetS) {
                    inc_GETSHit(reqEvent);
                    if (dirLine->numSharers() == 0) {
                        dirLine->setOwner(reqEvent->getSrc());
                        sendResponseUp(reqEvent, E, &event->getPayload(), true);
                    } else {
                        dirLine->addSharer(reqEvent->getSrc());
                        sendResponseUp(reqEvent, S, &event->getPayload(), true);
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
                    inc_InvalidatePUTSReqSent();
                    dirLine->setState(IM);
                } else if (reqEvent->getCmd() == FetchInv) {
                    if (dirLine->numSharers() > 0) {
                        invalidateAllSharers(dirLine, event->getRqstr(), true);
                        return IGNORE;
                    }
                    sendResponseDownFromMSHR(event, false);
                    inc_FetchInvReqSent();
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
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);

    bool isCached = dirLine->getDataLine() != NULL;
    if (isCached) dirLine->getDataLine()->setData(event->getPayload(), event);
    else if (mshr_->isHit(dirLine->getBaseAddr())) mshr_->setTempData(dirLine->getBaseAddr(), event->getPayload());

    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());

    switch (state) {
        case E:
            if (event->getDirty()) dirLine->setState(M);
        case M:
            dirLine->clearOwner();
            sendWritebackAck(event);
            if (!isCached) {
                sendWritebackFromMSHR(((dirLine->getState() == E) ? PutE : PutM), dirLine, event->getRqstr(), &event->getPayload());
                (dirLine->getState() == E) ? inc_EvictionPUTEReqSent() : inc_EvictionPUTMReqSent();
                if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
                dirLine->setState(I);
            }
            break;
        case EI:    // Evicting this block anyways
            if (event->getDirty()) dirLine->setState(MI);
        case MI:
            dirLine->clearOwner();
            sendWritebackFromMSHR(((dirLine->getState() == EI) ? PutE : PutM), dirLine, name_, &event->getPayload());
            (dirLine->getState() == EI) ? inc_EvictionPUTEReqSent() : inc_EvictionPUTMReqSent();
            if (!LL_) mshr_->insertWriteback(dirLine->getBaseAddr());
            dirLine->setState(I);
            break;
        case E_InvX:
            dirLine->clearOwner();
            if (reqEvent->getCmd() == FetchInvX) {
                if (!isCached) {
                    sendWritebackFromMSHR(event->getDirty() ? PutM : PutE, dirLine, event->getRqstr(), &event->getPayload());
                    (event->getDirty()) ? inc_EvictionPUTMReqSent() : inc_EvictionPUTEReqSent();
                    dirLine->setState(I);
                    if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    inc_FetchInvXReqSent();
                    sendResponseDownFromMSHR(event, (event->getCmd() == PutM));
                    dirLine->setState(S);
                }
            } else {
                inc_GETSHit(reqEvent);
                if (protocol_) {
                    sendResponseUp(reqEvent, E, &event->getPayload(), true);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendResponseUp(reqEvent, S, &event->getPayload(), true);
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
                    inc_EvictionPUTMReqSent();
                    dirLine->setState(I);
                    if (!LL_) mshr_->insertWriteback(event->getBaseAddr());
                } else {
                    inc_FetchInvXReqSent();
                    sendResponseDownFromMSHR(event, true);
                    dirLine->setState(S);
                }
            } else {
                inc_GETSHit(reqEvent);
                dirLine->setState(M);
                if (protocol_) {
                    sendResponseUp(reqEvent, E, &event->getPayload(), true);
                    dirLine->setOwner(reqEvent->getSrc());
                } else {
                    sendResponseUp(reqEvent, S, &event->getPayload(), true);
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
                if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                inc_GETXHit(reqEvent);
                dirLine->setState(M);
                sendResponseUp(reqEvent, M, &event->getPayload(), true);
                dirLine->setOwner(reqEvent->getSrc());
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(&event->getPayload(), false);
            } else { /* Cmd == Fetch */
                inc_FetchInvReqSent();
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
    stateStats_[event->getCmd()][state]++;
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
            inc_InvalidatePUTSReqSent();
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
            inc_InvalidatePUTSReqSent();
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
    stateStats_[event->getCmd()][state]++;
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
    stateStats_[event->getCmd()][state]++;
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
    stateStats_[event->getCmd()][state]++;
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
    stateStats_[responseEvent->getCmd()][state]++;
    recordStateEventCount(responseEvent->getCmd(), state);    
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    bool isCached = dirLine->getDataLine() != NULL;
    
    switch (state) {
        case IS:
            if (responseEvent->getGrantedState() == E) dirLine->setState(E);
            else dirLine->setState(S);
            inc_GETSHit(origRequest);
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), responseEvent);
            if (!shouldRespond) return DONE;
            if (dirLine->getState() == E) dirLine->setOwner(origRequest->getSrc());
            else dirLine->addSharer(origRequest->getSrc());
            sendResponseUp(origRequest, dirLine->getState(), &responseEvent->getPayload(), true);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(&responseEvent->getPayload(), false);
            return DONE;
        case IM:
            if (isCached) dirLine->getDataLine()->setData(responseEvent->getPayload(), responseEvent);
        case SM:
            dirLine->setState(M);
            dirLine->setOwner(origRequest->getSrc());
            if (dirLine->isSharer(origRequest->getSrc())) dirLine->removeSharer(origRequest->getSrc());
            sendResponseUp(origRequest, M, (isCached ? dirLine->getDataLine()->getData() : &responseEvent->getPayload()), true);
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
    stateStats_[responseEvent->getCmd()][state]++;
    recordStateEventCount(responseEvent->getCmd(), state);
    
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
                inc_GETSHit(reqEvent);
                dirLine->addSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, S, &responseEvent->getPayload(), true);
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
                inc_EvictionPUTSReqSent();
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
                (dirLine->getState() == EI) ? inc_EvictionPUTEReqSent() : inc_EvictionPUTMReqSent();
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
                inc_FetchInvXReqSent();
                sendResponseDownFromMSHR(responseEvent, (state == M_InvX || responseEvent->getDirty()));
                dirLine->setState(S);
            } else {
                inc_GETSHit(reqEvent);
                dirLine->addSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, S, &responseEvent->getPayload(), true);
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
                    if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                    inc_GETXHit(reqEvent);
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    dirLine->setOwner(reqEvent->getSrc());
                    sendResponseUp(reqEvent, M, &responseEvent->getPayload(), true);
                    dirLine->setState(M);
                } else {
                    inc_FetchInvReqSent();
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
                inc_FetchInvReqSent();
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
    stateStats_[ack->getCmd()][state]++;
    recordStateEventCount(ack->getCmd(), state);

    if (dirLine->isSharer(ack->getSrc())) {
        dirLine->removeSharer(ack->getSrc());
    }
    if (DEBUG_ALL || DEBUG_ADDR == ack->getBaseAddr()) d_->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    bool isCached = dirLine->getDataLine() != NULL;
    vector<uint8_t> * data = isCached ? dirLine->getDataLine()->getData() : mshr_->getTempData(reqEvent->getBaseAddr());
    
    switch (state) {
        case S_Inv: // AckInv for Inv
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, dirLine, data, false, true);
                } else {
                    inc_InvalidatePUTSReqSent();
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                }
                dirLine->setState(I);
            }
            return action;
        case E_Inv: // AckInv for FetchInv, possibly waiting on FetchResp too
        case M_Inv: // AckInv for FetchInv or GetX, possibly on FetchResp or GetXResp too
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, dirLine, data, (state == E_Inv), true);
                    dirLine->setState(I);
                } else {
                    if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                    inc_GETXHit(reqEvent);
                    dirLine->setOwner(reqEvent->getSrc());
                    if (dirLine->isSharer(reqEvent->getSrc())) dirLine->removeSharer(reqEvent->getSrc());
                    sendResponseUp(reqEvent, M, data, true);
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
                    inc_InvalidatePUTSReqSent();
                    dirLine->setState(IM);
                } else if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
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
                inc_EvictionPUTSReqSent();
                if (!LL_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case EI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutE, dirLine, reqEvent->getRqstr(), data);
                inc_EvictionPUTEReqSent();
                if (!LL_) mshr_->insertWriteback(ack->getBaseAddr());
                dirLine->setState(I);
            }
        case MI:
            if (action == DONE) {
                sendWritebackFromMSHR(PutM, dirLine, reqEvent->getRqstr(), data);
                inc_EvictionPUTMReqSent();
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
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv = new MemEvent((Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
    
        uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(dirLine->getBaseAddr());

        if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                dirLine->getBaseAddr(), (*it).c_str(), deliveryTime);
    }
}


void MESIInternalDirectory::invalidateAllSharersAndFetch(CacheLine * cacheLine, string rqstr, bool replay) {
    set<std::string> * sharers = cacheLine->getSharers();
    bool fetched = false;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv;
        if (fetched) inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        else {
            inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
            fetched = true;
        }
        inv->setDst(*it);
        inv->setRqstr(rqstr);
    
        uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
    }
}


/*
 * If checkFetch is true -> block is not cached
 * Then, if requestor is not already a sharer, we need data!
 */
bool MESIInternalDirectory::invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay, bool uncached) {
    bool sentInv = false;
    set<std::string> * sharers = cacheLine->getSharers();
    bool needFetch = uncached && (sharers->find(rqstr) == sharers->end());
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

        uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);
        sentInv = true;
        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
    }
    return sentInv;
}


void MESIInternalDirectory::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
    if (!(cacheLine->getOwner()).empty()) fetch->setDst(cacheLine->getOwner());
    else fetch->setDst(*(cacheLine->getSharers()->begin()));
    fetch->setRqstr(rqstr);
    
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
   
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
}


void MESIInternalDirectory::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
}


void MESIInternalDirectory::sendFetch(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Fetch);
    fetch->setDst(*((cacheLine->getSharers())->begin()));
    fetch->setRqstr(rqstr);
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending Fetch: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
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

    uint64 deliveryTime = replay ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) { 
        d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
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
    
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
        d_->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}

void MESIInternalDirectory::sendAckInv(Addr baseAddr, string origRqstr) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, AckInv);
    ack->setDst(getDestination(baseAddr));
    ack->setRqstr(origRqstr);
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueue(resp);
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
}


void MESIInternalDirectory::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, event->getBaseAddr(), event->getBaseAddr(), AckPut);
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_, "Sending AckPut at cycle = %" PRIu64 "\n", deliveryTime);
}

void MESIInternalDirectory::sendWritebackFromCache(Command cmd, CacheLine * dirLine, string rqstr) {
    MemEvent * writeback = new MemEvent((SST::Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setSize(dirLine->getSize());
        writeback->setPayload(*(dirLine->getDataLine()->getData()));
    }
    writeback->setRqstr(rqstr);
    if (cmd == PutM) writeback->setDirty(true);
    uint64_t deliveryTime = timestamp_ + accessLatency_;
    Response resp = {writeback, deliveryTime, false};
    addToOutgoingQueue(resp);
    if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From cache\n", deliveryTime, CommandString[cmd]);
}

void MESIInternalDirectory::sendWritebackFromMSHR(Command cmd, CacheLine * dirLine, string rqstr, vector<uint8_t> * data) {
    MemEvent * writeback = new MemEvent((SST::Component*)owner_, dirLine->getBaseAddr(), dirLine->getBaseAddr(), cmd);
    writeback->setDst(getDestination(dirLine->getBaseAddr()));
    if (cmd == PutM || writebackCleanBlocks_) {
        writeback->setSize(dirLine->getSize());
        writeback->setPayload(*data);
    }
    writeback->setRqstr(rqstr);
    if (cmd == PutM) writeback->setDirty(true);
    uint64_t deliveryTime = timestamp_ + accessLatency_;
    Response resp = {writeback, deliveryTime, false};
    addToOutgoingQueue(resp);
    if (DEBUG_ALL || DEBUG_ADDR == dirLine->getBaseAddr()) d_->debug(_L3_, "Sending writeback at cycle = %" PRIu64 ", Cmd = %s. From MSHR\n", deliveryTime, CommandString[cmd]);
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


/*
 *  Print stats
 */
void MESIInternalDirectory::printStats(int statLoc, vector<int> groupIds, map<int, CtrlStats> ctrlStats, uint64_t upgradeLatency, 
        uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM,
        uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)statLoc);
    dbg->output(CALL_INFO,"\n------------------------------------------------------------------------\n");
    dbg->output(CALL_INFO,"--- Cache Stats\n");
    dbg->output(CALL_INFO,"--- Name: %s\n", name_.c_str());
    dbg->output(CALL_INFO,"--- Overall Statistics\n");
    dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");

    for(unsigned int i = 0; i < groupIds.size(); i++){
        uint64_t totalMisses =  ctrlStats[groupIds[i]].newReqGetSMisses_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ +
                                ctrlStats[groupIds[i]].blockedReqGetSMisses_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_;
        uint64_t totalHits =    ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetSExHits_ +
                                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetSExHits_;

        uint64_t totalRequests = totalHits + totalMisses;
        double hitRatio = ((double)totalHits / ( totalHits + totalMisses)) * 100;
        
        if(i != 0){
            dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");
            dbg->output(CALL_INFO,"--- Cache Stats\n");
            dbg->output(CALL_INFO,"--- Name: %s\n", name_.c_str());
            dbg->output(CALL_INFO,"--- Group Statistics, Group ID = %i\n", groupIds[i]);
            dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");
        }
        dbg->output(CALL_INFO,"- Total data requests:                           %" PRIu64 "\n", totalRequests);
        dbg->output(CALL_INFO,"     GetS:                                       %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetSMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetSMisses_);                                  
        dbg->output(CALL_INFO,"     GetX:                                       %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_);                                  
        dbg->output(CALL_INFO,"     GetSEx:                                     %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetSExHits_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSExHits_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_);                                  
        dbg->output(CALL_INFO,"- Total misses:                                  %" PRIu64 "\n", totalMisses);
        // Report misses at the time a request was handled -> "blocked" indicates request was blocked by another pending request before being handled
        dbg->output(CALL_INFO,"     GetS, miss on arrival:                      %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSMisses_);
        dbg->output(CALL_INFO,"     GetS, miss after being blocked:             %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSMisses_);
        dbg->output(CALL_INFO,"     GetX, miss on arrival:                      %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetXMisses_);
        dbg->output(CALL_INFO,"     GetX, miss after being blocked:             %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetXMisses_);
        dbg->output(CALL_INFO,"     GetSEx, miss on arrival:                    %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSExMisses_);
        dbg->output(CALL_INFO,"     GetSEx, miss after being blocked:           %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSExMisses_);
        dbg->output(CALL_INFO,"- Total hits:                                    %" PRIu64 "\n", totalHits);
        dbg->output(CALL_INFO,"     GetS, hit on arrival:                       %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSHits_);
        dbg->output(CALL_INFO,"     GetS, hit after being blocked:              %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSHits_);
        dbg->output(CALL_INFO,"     GetX, hit on arrival:                       %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetXHits_);
        dbg->output(CALL_INFO,"     GetX, hit after being blocked:              %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetXHits_);
        dbg->output(CALL_INFO,"     GetSEx, hit on arrival:                     %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSExHits_);
        dbg->output(CALL_INFO,"     GetSEx, hit after being blocked:            %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSExHits_);
        dbg->output(CALL_INFO,"- Hit ratio:                                     %.3f%%\n", hitRatio);
        dbg->output(CALL_INFO,"- Miss ratio:                                    %.3f%%\n", 100 - hitRatio);
        dbg->output(CALL_INFO,"------------ Coherence transitions for misses -------------\n");
        dbg->output(CALL_INFO,"- GetS   I->S:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetS_IS);
        dbg->output(CALL_INFO,"- GetS   M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetS_M);
        dbg->output(CALL_INFO,"- GetX   I->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetX_IM);
        dbg->output(CALL_INFO,"- GetX   S->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetX_SM);
        dbg->output(CALL_INFO,"- GetX   M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetX_M);
        dbg->output(CALL_INFO,"- GetSEx I->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetSE_IM);
        dbg->output(CALL_INFO,"- GetSEx S->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetSE_SM);
        dbg->output(CALL_INFO,"- GetSEx M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetSE_M);
        dbg->output(CALL_INFO,"------------ Replacements and evictions -------------------\n");
        dbg->output(CALL_INFO,"- PutS received:                                 %" PRIu64 "\n", stats_[groupIds[i]].PUTSReqsReceived_);
        dbg->output(CALL_INFO,"- PutM received:                                 %" PRIu64 "\n", stats_[groupIds[i]].PUTMReqsReceived_);
        dbg->output(CALL_INFO,"- PutS sent due to eviction:                     %" PRIu64 "\n", stats_[groupIds[i]].EvictionPUTSReqSent_);
        dbg->output(CALL_INFO,"- PutE sent due to eviction:                     %" PRIu64 "\n", stats_[groupIds[i]].EvictionPUTEReqSent_);
        dbg->output(CALL_INFO,"- PutM sent due to eviction:                     %" PRIu64 "\n", stats_[groupIds[i]].EvictionPUTMReqSent_);
        dbg->output(CALL_INFO,"------------ Other stats ----------------------------------\n");
        dbg->output(CALL_INFO,"- Inv stalled because LOCK held:                 %" PRIu64 "\n", ctrlStats[groupIds[i]].InvWaitingForUserLock_);
        dbg->output(CALL_INFO,"- Requests received (incl coherence traffic):    %" PRIu64 "\n", ctrlStats[groupIds[i]].TotalRequestsReceived_);
        dbg->output(CALL_INFO,"- Requests handled by MSHR (MSHR hits):          %" PRIu64 "\n", ctrlStats[groupIds[i]].TotalMSHRHits_);
        dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, Down):                  %" PRIu64 "\n", stats_[groupIds[i]].NACKsSentDown_);
        dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, Up):                    %" PRIu64 "\n", stats_[groupIds[i]].NACKsSentUp_);
        dbg->output(CALL_INFO,"------------ Latency stats --------------------------------\n");
        dbg->output(CALL_INFO,"- Avg Miss Latency (cyc):                        %" PRIu64 "\n", upgradeLatency);
        if (ctrlStats[groupIds[0]].GetS_IS > 0) 
            dbg->output(CALL_INFO,"- Latency GetS   I->S:                           %" PRIu64 "\n", (lat_GetS_IS / ctrlStats[groupIds[0]].GetS_IS));
        if (ctrlStats[groupIds[0]].GetS_M > 0) 
            dbg->output(CALL_INFO,"- Latency GetS   M:                              %" PRIu64 "\n", (lat_GetS_M / ctrlStats[groupIds[0]].GetS_M));
        if (ctrlStats[groupIds[0]].GetX_IM > 0)
            dbg->output(CALL_INFO,"- Latency GetX   I->M:                           %" PRIu64 "\n", (lat_GetX_IM / ctrlStats[groupIds[0]].GetX_IM));
        if (ctrlStats[groupIds[0]].GetX_SM > 0)
            dbg->output(CALL_INFO,"- Latency GetX   S->M:                           %" PRIu64 "\n", (lat_GetX_SM / ctrlStats[groupIds[0]].GetX_SM));
        if (ctrlStats[groupIds[0]].GetX_M > 0) 
            dbg->output(CALL_INFO,"- Latency GetX   M:                              %" PRIu64 "\n", (lat_GetX_M / ctrlStats[groupIds[0]].GetX_M));
        if (ctrlStats[groupIds[0]].GetSE_IM > 0)
            dbg->output(CALL_INFO,"- Latency GetSEx I->M:                           %" PRIu64 "\n", (lat_GetSEx_IM / ctrlStats[groupIds[0]].GetSE_IM));
        if (ctrlStats[groupIds[0]].GetSE_SM > 0)
            dbg->output(CALL_INFO,"- Latency GetSEx S->M:                           %" PRIu64 "\n", (lat_GetSEx_SM / ctrlStats[groupIds[0]].GetSE_SM));
        if (ctrlStats[groupIds[0]].GetSE_M > 0)
            dbg->output(CALL_INFO,"- Latency GetSEx M:                              %" PRIu64 "\n", (lat_GetSEx_M / ctrlStats[groupIds[0]].GetSE_M));
    }
    dbg->output(CALL_INFO,"------------ State and event stats ---------------------------\n");
    for (int i = 0; i < LAST_CMD; i++) {
        for (int j = 0; j < LAST_CMD; j++) {
            if (stateStats_[i][j] == 0) continue;
            dbg->output(CALL_INFO,"%s, %s:        %" PRIu64 "\n", CommandString[i], StateString[j], stateStats_[i][j]);
        }
    }    
}

void MESIInternalDirectory::printStatsForMacSim(int statLocation, vector<int> groupIds, map<int, CtrlStats> ctrlStats, uint64_t upgradeLatency, 
        uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM,
        uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M){
    stringstream ss;
    ss << name_.c_str() << ".stat.out";
    string filename = ss.str();

    ofstream ofs;
    ofs.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filename.c_str(), std::ios_base::out);

    for (unsigned int i = 0; i < groupIds.size(); ++i) {
        uint64_t totalMisses =  ctrlStats[groupIds[i]].newReqGetSMisses_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ +
                                ctrlStats[groupIds[i]].blockedReqGetSMisses_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_;
        uint64_t totalHits =    ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetSExHits_ +
                                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetSExHits_;

        uint64_t totalRequests = totalHits + totalMisses;
        double hitRatio = ((double)totalHits / ( totalHits + totalMisses)) * 100;

        writeTo(ofs, name_, string("Total_data_requests"), totalRequests);
        writeTo(ofs, name_, string("GetS"), 
                ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetSMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetSMisses_);
        writeTo(ofs, name_, string("GetX"), 
                ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_);
        writeTo(ofs, name_, string("GetSEx"), 
                ctrlStats[groupIds[i]].newReqGetSExHits_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSExHits_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_);

        writeTo(ofs, name_, string("Total_misses"), totalMisses);
        // Report misses at the time a request was handled -> "blocked" indicates request was blocked by another pending request before being handled
        writeTo(ofs, name_, string("GetS_miss_on_arrival"),            ctrlStats[groupIds[i]].newReqGetSMisses_);
        writeTo(ofs, name_, string("GetS_miss_after_being_blocked"),   ctrlStats[groupIds[i]].blockedReqGetSMisses_);
        writeTo(ofs, name_, string("GetX_miss_on_arrival"),            ctrlStats[groupIds[i]].newReqGetXMisses_);
        writeTo(ofs, name_, string("GetX_miss_after_being_blocked"),   ctrlStats[groupIds[i]].blockedReqGetXMisses_);
        writeTo(ofs, name_, string("GetSEx_miss_on_arrival"),          ctrlStats[groupIds[i]].newReqGetSExMisses_);
        writeTo(ofs, name_, string("GetSEx_miss_after_being_blocked"), ctrlStats[groupIds[i]].blockedReqGetSExMisses_);

        writeTo(ofs, name_, string("Total_hits"), totalHits);
        writeTo(ofs, name_, string("GetS_hit_on_arrival"),            ctrlStats[groupIds[i]].newReqGetSHits_);
        writeTo(ofs, name_, string("GetS_hit_after_being_blocked"),   ctrlStats[groupIds[i]].blockedReqGetSHits_);
        writeTo(ofs, name_, string("GetX_hit_on_arrival"),            ctrlStats[groupIds[i]].newReqGetXHits_);
        writeTo(ofs, name_, string("GetX_hit_after_being_blocked"),   ctrlStats[groupIds[i]].blockedReqGetXHits_);
        writeTo(ofs, name_, string("GetSEx_hit_on_arrival"),          ctrlStats[groupIds[i]].newReqGetSExHits_);
        writeTo(ofs, name_, string("GetSEx_hit_after_being_blocked"), ctrlStats[groupIds[i]].blockedReqGetSExHits_);

        writeTo(ofs, name_, string("Hit_ratio"), hitRatio);
        writeTo(ofs, name_, string("Miss_ratio"), 100 - hitRatio);

        // Coherence transitions for misses 
        writeTo(ofs, name_, string("GetS_I->S"),                           ctrlStats[groupIds[i]].GetS_IS);
        writeTo(ofs, name_, string("GetS_M(present_at_another_cache)"),    ctrlStats[groupIds[i]].GetS_M);
        writeTo(ofs, name_, string("GetX_I->M"),                           ctrlStats[groupIds[i]].GetX_IM);
        writeTo(ofs, name_, string("GetX_S->M"),                           ctrlStats[groupIds[i]].GetX_SM);
        writeTo(ofs, name_, string("GetX_M(present_at_another_cache)"),    ctrlStats[groupIds[i]].GetX_M);
        writeTo(ofs, name_, string("GetSEx_I->M"),                         ctrlStats[groupIds[i]].GetSE_IM);
        writeTo(ofs, name_, string("GetSEx_S->M"),                         ctrlStats[groupIds[i]].GetSE_SM);
        writeTo(ofs, name_, string("GetSEx_M(present_at_another_cache)"),  ctrlStats[groupIds[i]].GetSE_M);

        // Replacements and evictions
        writeTo(ofs, name_, string("PutS_received"),               stats_[groupIds[i]].PUTSReqsReceived_);
        writeTo(ofs, name_, string("PutM_received"),               stats_[groupIds[i]].PUTMReqsReceived_);
        writeTo(ofs, name_, string("PutS_sent_due_to_eviction"),   stats_[groupIds[i]].EvictionPUTSReqSent_);
        writeTo(ofs, name_, string("PutE_sent_due_to_eviction"),   stats_[groupIds[i]].EvictionPUTEReqSent_);
        writeTo(ofs, name_, string("PutM_sent_due_to_eviction"),   stats_[groupIds[i]].EvictionPUTMReqSent_);

        // Other stats
        writeTo(ofs, name_, string("Inv_stalled_because_LOCK_held"),               ctrlStats[groupIds[i]].InvWaitingForUserLock_);
        writeTo(ofs, name_, string("Requests_received_(incl_coherence_traffic)"),  ctrlStats[groupIds[i]].TotalRequestsReceived_);
        writeTo(ofs, name_, string("Requests_handled_by_MSHR_(MSHR_hits)"),        ctrlStats[groupIds[i]].TotalMSHRHits_);
        writeTo(ofs, name_, string("NACKs_sent_(MSHR_Full,Down)"),                 stats_[groupIds[i]].NACKsSentDown_);
        writeTo(ofs, name_, string("NACKs_sent_(MSHR_Full,Up)"),                   stats_[groupIds[i]].NACKsSentUp_);

        // Latency stats
        writeTo(ofs, name_, string("Avg_Miss_Latency_(cyc)"), upgradeLatency);
        if (ctrlStats[groupIds[0]].GetS_IS  > 0) writeTo(ofs, name_, string("Latency_GetS_I->S"),   (lat_GetS_IS / ctrlStats[groupIds[0]].GetS_IS));
        if (ctrlStats[groupIds[0]].GetS_M   > 0) writeTo(ofs, name_, string("Latency_GetS_M"),      (lat_GetS_M / ctrlStats[groupIds[0]].GetS_M));
        if (ctrlStats[groupIds[0]].GetX_IM  > 0) writeTo(ofs, name_, string("Latency_GetX_I->M"),   (lat_GetX_IM / ctrlStats[groupIds[0]].GetX_IM));
        if (ctrlStats[groupIds[0]].GetX_SM  > 0) writeTo(ofs, name_, string("Latency_GetX_S->M"),   (lat_GetX_SM / ctrlStats[groupIds[0]].GetX_SM));
        if (ctrlStats[groupIds[0]].GetX_M   > 0) writeTo(ofs, name_, string("Latency_GetX_M"),      (lat_GetX_M / ctrlStats[groupIds[0]].GetX_M));
        if (ctrlStats[groupIds[0]].GetSE_IM > 0) writeTo(ofs, name_, string("Latency_GetSEx_I->M"), (lat_GetSEx_IM / ctrlStats[groupIds[0]].GetSE_IM));
        if (ctrlStats[groupIds[0]].GetSE_SM > 0) writeTo(ofs, name_, string("Latency_GetSEx_S->M"), (lat_GetSEx_SM / ctrlStats[groupIds[0]].GetSE_SM));
        if (ctrlStats[groupIds[0]].GetSE_M  > 0) writeTo(ofs, name_, string("Latency_GetSEx_M"),    (lat_GetSEx_M / ctrlStats[groupIds[0]].GetSE_M));
    }

    // State and event stats
#include <boost/format.hpp>
    for (int i = 0; i < LAST_CMD; i++) {
        for (int j = 0; j < LAST_CMD; j++) {
            if (stateStats_[i][j] == 0) continue;
            writeTo(ofs, name_, str(boost::format("%1%_%2%") % CommandString[i] % StateString[j]), stateStats_[i][j]);
        }
    }
    ofs.close();
}

