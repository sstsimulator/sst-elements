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
#include "MESICoherenceController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * MESI Coherence Controller Implementation
 * Provides MESI & MSI coherence for all non-L1 caches
 * Supports both inclusive and non-inclusive caches (provided there is a single upper level cache, 
 * otherwise MESIInternalDirectory needs to be used)
 *---------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------
 *  External interface functions for routing events from cache controller to appropriate coherence handlers
 *---------------------------------------------------------------------------------------------------------------------*/	    

/**
 *  Handle evictions
 *  Block to be evicted is determined by cache controller
 *  Evictions to blocks in transition are stalled, otherwise we evict
 *  Writeback of clean data are required when writing back to an non-inclusive cache
 *  LLCs and caches writing back to non-inclusive caches wait for AckPuts before sending further events for the evicted address
 *  This prevents races if the writeback is NACKed.
 */
CacheAction MESIController::handleEviction(CacheLine* wbCacheLine, uint32_t groupId, string rqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
    setGroupId(groupId);
    recordEvictionState(state);

    Addr wbBaseAddr = wbCacheLine->getBaseAddr();
    if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L6_, "Handling eviction at cache for addr 0x%" PRIx64 " with index %d\n", wbBaseAddr, wbCacheLine->getIndex());
    switch(state) {
        case SI:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
        case E_InvX:
        case M_InvX:
            return STALL;
        case I:
            return DONE;
        case S:
            if (!inclusive_ && wbCacheLine->numSharers() > 0) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (wbCacheLine->numSharers() > 0) {
                invalidateAllSharers(wbCacheLine, name_, false); 
                wbCacheLine->setState(SI);
                evictionRequiredInv_++;
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
                return STALL;
            }
            inc_EvictionPUTSReqSent();
            sendWriteback(PutS, wbCacheLine, false, rqstr);
            wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbBaseAddr);
	    return DONE;
        case E:
            if (!inclusive_ && wbCacheLine->ownerExists()) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (wbCacheLine->numSharers() > 0) {
                invalidateAllSharers(wbCacheLine, name_, false); 
                wbCacheLine->setState(EI);
                evictionRequiredInv_++;
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
                return STALL;
            }
            if (wbCacheLine->ownerExists()) {
                sendFetchInv(wbCacheLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                wbCacheLine->setState(EI);
                evictionRequiredInv_++;
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating owner\n");
                return STALL;
            }
            inc_EvictionPUTEReqSent();
            sendWriteback(PutE, wbCacheLine, false, rqstr);
	    wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbBaseAddr);
	    return DONE;
        case M:
            if (!inclusive_ && wbCacheLine->ownerExists()) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (wbCacheLine->numSharers() > 0) {
                invalidateAllSharers(wbCacheLine, name_, false); 
                wbCacheLine->setState(MI);
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
                return STALL;
            }
            if (wbCacheLine->ownerExists()) {
                sendFetchInv(wbCacheLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                wbCacheLine->setState(MI);
                evictionRequiredInv_++;
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating owner\n");
                return STALL;
            }
            inc_EvictionPUTMReqSent();
	    sendWriteback(PutM, wbCacheLine, true, rqstr);
            wbCacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(wbBaseAddr);
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
 *  Handle request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction MESIController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    setGroupId(event->getGroupId());
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
 *  Handle replacement
 */
CacheAction MESIController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    setGroupId(event->getGroupId());
    Command cmd = event->getCmd();

    switch(cmd) {
        case PutS:
            inc_PUTSReqsReceived();
            return handlePutSRequest(event, cacheLine, reqEvent);
        case PutM:
            inc_PUTMReqsReceived();
            return handlePutMRequest(event, cacheLine, reqEvent);
            break;
        case PutE:
            inc_PUTEReqsReceived();
            return handlePutMRequest(event, cacheLine, reqEvent);
            break;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 *  Handle invalidations.
 *  Special cases exist when the invalidation races with a writeback that is queued in the MSHR.
 *  Non-inclusive caches which miss locally simply forward the request to the next higher cache
 */
CacheAction MESIController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, bool replay) {
    if (!cacheLine || cacheLine->getState() == I) { // Non-inclusive caches -> Inv may have raced with Put OR line is present in an upper level cache
        stateStats_[event->getCmd()][I]++;
        recordStateEventCount(event->getCmd(), I);

        if (!inclusive_) {
            if (mshr_->pendingWriteback(event->getBaseAddr())) {
            // Case 1: Inv raced with a Put -> treat Inv as the AckPut
                mshr_->removeWriteback(event->getBaseAddr());
                return DONE;
            } else {
                MemEvent * waitingEvent = (mshr_->isHit(event->getBaseAddr())) ? mshr_->lookupFront(event->getBaseAddr()) : NULL;
                if (waitingEvent != NULL && (waitingEvent->getCmd() == PutS || waitingEvent->getCmd() == PutE || waitingEvent->getCmd() == PutM)) {
                // Case 2: Inv arrived and Put is pending in the MSHR (waiting for an open cache line)
                    return BLOCK;
                } else {
                // Case 3: Inv arrived and we don't have block cached locally, check for block at upper levels
                    forwardMessageUp(event);
                    mshr_->setAcksNeeded(event->getBaseAddr(), 1);
                    event->setInProgress(true);
                    return STALL;
                }
            }
        } else {    // Inclusive caches -> Inv raced with Put
            if (mshr_->pendingWriteback(event->getBaseAddr())) {   // Treat Inv as AckPut
                mshr_->removeWriteback(event->getBaseAddr());
                return DONE;
            } else {
                return IGNORE;
            }
        }
    }

    setGroupId(event->getGroupId());

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


/**
 *  Handle responses including data (GetSResp, GetXResp), Inv/Fetch (FetchResp, FetchXResp, AckInv) and writeback acks (AckPut)
 */
CacheAction MESIController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case GetSResp:
        case GetXResp:
            return handleDataResponse(respEvent, cacheLine, reqEvent);
        case FetchResp:
        case FetchXResp:
            return handleFetchResp(respEvent, cacheLine, reqEvent);
            break;
        case AckInv:
            return handleAckInv(respEvent, cacheLine, reqEvent);
        case AckPut:
            stateStats_[respEvent->getCmd()][I]++;
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            return DONE;    // Retry any events that were stalled for ack
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}

/**
 *  Return type of miss. Used for profiling incoming requests at the cacheController
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request)
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition
 */
int MESIController::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
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
        case SI:
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


/** Handle GetS request */
CacheAction MESIController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == name_));
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);
    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), NULL);
            inc_GETSMissIS(event);
            cacheLine->setState(IS);
            return STALL;
        case S:
            inc_GETSHit(event);
            if (!shouldRespond) return DONE;
            cacheLine->addSharer(event->getSrc());
            sendResponseUp(event, S, data, replay);
            return DONE;
        case E:
        case M:
            inc_GETSHit(event);
            if (!shouldRespond) return DONE;
            
            // For Non-inclusive caches, we will deallocate this block so E/M permission needs to transfer!
            // Alternately could write back
            if (!inclusive_) {
                sendResponseUp(event, state, data, replay);
                cacheLine->setOwner(event->getSrc());
                return DONE;
            }

            if (cacheLine->isShareless() && !cacheLine->ownerExists() && protocol_) {
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "New owner: %s\n", event->getSrc().c_str());
                cacheLine->setOwner(event->getSrc());
                 sendResponseUp(event, E, data, replay);
                return DONE;
            }
            if (cacheLine->ownerExists()) {
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"GetS request but exclusive owner exists \n");
                sendFetchInvX(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) cacheLine->setState(E_InvX);
                else cacheLine->setState(M_InvX);
                return STALL;
            }
            cacheLine->addSharer(event->getSrc());
            sendResponseUp(event, S, data, replay);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

    }
    return STALL;    // eliminate compiler warning
}


/** Handle GetX or GetSEx (Read-lock) */
CacheAction MESIController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    stateStats_[cmd][state]++;
    if (state != SM) recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
            inc_GETXMissIM(event);
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), NULL);
            cacheLine->setState(IM);
            return STALL;
        case S:
            inc_GETXMissSM(event);
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), NULL);
            if (invalidateSharersExceptRequestor(cacheLine, event->getSrc(), event->getRqstr(), replay)) {
                cacheLine->setState(SM_Inv);
            } else {
                cacheLine->setState(SM);
            }
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            if (cmd == GetSEx) inc_GetSExReqsReceived(replay);
            inc_GETXHit(event);

            if (!cacheLine->isShareless()) {
                if (invalidateSharersExceptRequestor(cacheLine, event->getSrc(), event->getRqstr(), replay)) {
                    cacheLine->setState(M_Inv);
                    return STALL;
                }
            }
            if (cacheLine->ownerExists()) {
                sendFetchInv(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(M_Inv);
                return STALL;
            }
            cacheLine->setOwner(event->getSrc());
            if (cacheLine->isSharer(event->getSrc())) cacheLine->removeSharer(event->getSrc());
            sendResponseUp(event, M, cacheLine->getData(), replay);
            if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        case SM:
            return STALL;   // retried this request too soon because we were checking for waiting invalidations
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 * Handle PutS requests. 
 * Special cases for non-inclusive caches with racing events where we don't wait for a new line to be allocated if deadlock might occur
 */
CacheAction MESIController::handlePutSRequest(MemEvent* event, CacheLine* line, MemEvent * reqEvent) {
    State state = (line != NULL) ? line->getState() : I;
    stateStats_[event->getCmd()][state]++;    // Profile before we update anything
    recordStateEventCount(event->getCmd(), state);
    
    if (!inclusive_) {
        if (line == NULL) { // Means we couldn't allocate a cache line immediately but we have a reqEvent that we need to resolve
            if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
            //bool doneWithReq = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0);
            if (reqEvent->getCmd() == Fetch) {
                sendResponseDownFromMSHR(event, reqEvent, false);
                return STALL;
            } else {
                d_->fatal(CALL_INFO, -1, "%s, Error: Received PutS for an unallocated line but reqEvent cmd is unhandled. Addr = 0x%" PRIx64 ", Cmd = %s. Time = %" PRIu64 "ns\n",
                        name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], ((Component*)owner_)->getCurrentSimTimeNano());
            }
        }
        line->setData(event->getPayload(), event);
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(line->getData(), true);
        if (state == I) {
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                line->setState(S);
            } else {
                if (reqEvent->getCmd() == Fetch) {
                    line->setState(S_D);
                } else line->setState(S_Inv);   // cmd is FetchInv or Inv
            }
        }
    } 
    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());

    if (line->isSharer(event->getSrc())) {
        line->removeSharer(event->getSrc());
    }
    
    bool retry = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0);
    state = line->getState(); 
    if (!retry) return IGNORE;

    switch (state) {
        case I:
        case S:
        case E:
        case M:
            if (!inclusive_) sendWritebackAck(event);
            return DONE;
        /* Races with evictions */
        case SI:
            inc_EvictionPUTSReqSent();
            sendWriteback(PutS, line, false, name_);
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case EI:
            inc_EvictionPUTEReqSent();
            sendWriteback(PutE, line, false, name_);
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case MI:
            inc_EvictionPUTMReqSent();
            sendWriteback(PutM, line, true, name_);
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        /* Race with a fetch */
        case S_D:
            sendResponseDown(reqEvent, line, false, true);
            line->setState(S);
            return DONE;
        /* Races with Invs and FetchInvs from outside our sub-hierarchy; races with GetX from within our sub-hierarchy*/
        case S_Inv:
            if (reqEvent->getCmd() == Inv) {
                sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                inc_InvalidatePUTSReqSent();
                line->setState(I);
            } else {
                sendResponseDown(reqEvent, line, false, true);
                inc_FetchInvReqSent();
                line->setState(I);
            }
            return DONE;
        case E_Inv:
            inc_FetchInvReqSent();
            sendResponseDown(reqEvent, line, false, true);
            line->setState(I);
            return DONE;
        case M_Inv:
            if (reqEvent->getCmd() == FetchInv) {
                inc_FetchInvReqSent();
                sendResponseDown(reqEvent, line, true, true);
                line->setState(I);
            } else if (reqEvent->getCmd() == GetX || reqEvent->getCmd() == GetSEx) {
                line->setOwner(reqEvent->getSrc());
                if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, M, line->getData(), true);
                line->setState(M);
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(line->getData(), false);
            }
            return DONE;
        case SM_Inv:
            if (reqEvent->getCmd() == Inv) {
                if (line->numSharers() > 0) {
                    invalidateAllSharers(line, reqEvent->getRqstr(), true);
                    return IGNORE;
                } else {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                    inc_InvalidatePUTSReqSent();
                    line->setState(IM);
                    return DONE;
                }
            } else {
                line->setState(SM);
                return IGNORE; // Waiting for GetXResp
            }
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

    }
    return IGNORE;   // eliminate compiler warning
}


/**
 *  Handle PutM and PutE requests (i.e., dirty and clean non-inclusive replacements respectively)
 */
CacheAction MESIController::handlePutMRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent) {
    State state = (cacheLine == NULL) ? I : cacheLine->getState();

    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);
    
    if (!inclusive_) {
        if (cacheLine == NULL) {    // Didn't wait for allocation because a reqEvent is dependent on this replacement
            if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
            if (reqEvent->getCmd() == FetchInvX) {
                // Handle FetchInvX and keep stalling Put*
                inc_FetchInvXReqSent();
                sendResponseDownFromMSHR(event, reqEvent, event->getDirty());
                return STALL;
            } else if (reqEvent->getCmd() == FetchInv) {  
                // HandleFetchInv and retire Put*
                inc_FetchInvReqSent();
                sendResponseDownFromMSHR(event, reqEvent, event->getDirty());
                return IGNORE;
            } else {
                d_->fatal(CALL_INFO, -1, "%s, Error: Received %s for an unallocated line but conflicting event's command is unhandled. Addr = 0x%" PRIx64 ", Cmd = %s. Time = %" PRIu64 "ns\n",
                        name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), CommandString[reqEvent->getCmd()], ((Component*)owner_)->getCurrentSimTimeNano());
            }
        } else if (cacheLine->getState() == I) {
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                (event->getCmd() == PutM) ? cacheLine->setState(M) : cacheLine->setState(E);
            } else {
                if (reqEvent->getCmd() == FetchInvX) {
                    (event->getCmd() == PutM) ? cacheLine->setState(M_InvX) : cacheLine->setState(E_InvX);
                } else {
                    (event->getCmd() == PutM) ? cacheLine->setState(M_Inv) : cacheLine->setState(E_Inv);
                }
            }
            state = cacheLine->getState();
        }
    } 
    if (event->getDirty() || !inclusive_) {
        cacheLine->setData(event->getPayload(), event);
    }
            
    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
    
    switch (state) {
        case E:
            if (event->getDirty()) cacheLine->setState(M);
        case M:
            cacheLine->clearOwner();
            if (!inclusive_) sendWritebackAck(event);
            break;
        /* Races with evictions */
        case EI:
            if (event->getCmd() == PutM) {
                inc_EvictionPUTMReqSent();
                sendWriteback(PutM, cacheLine, true, name_);
            } else {
                inc_EvictionPUTEReqSent();
                sendWriteback(PutE, cacheLine, false, name_);
            }
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            inc_EvictionPUTMReqSent();
            sendWriteback(PutM, cacheLine, true, name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        /* Races with FetchInv from outside our sub-hierarchy; races with GetX from within our sub-hierarchy */
        case E_Inv:
            inc_FetchInvReqSent();
            sendResponseDown(reqEvent, cacheLine, event->getDirty(), true);
            cacheLine->setState(I);
            break;
        case M_Inv:
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == FetchInv) {
                inc_FetchInvReqSent();
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else {
                cacheLine->setState(M);
                if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                inc_GETXHit(reqEvent);
                cacheLine->setOwner(reqEvent->getSrc());
                sendResponseUp(reqEvent, M, cacheLine->getData(), true);
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            }
            break;
        /* Races from FetchInvX from outside our sub-hierarchy; races with GetS from within our sub-hierarchy */
        case E_InvX:
        case M_InvX:
            if (!event->getDirty() && state == E_InvX) {
                cacheLine->setState(E);
            } else {
                cacheLine->setState(M);
            }
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == FetchInvX) {
                inc_FetchInvXReqSent();
                sendResponseDown(reqEvent, cacheLine, (state == M_InvX || event->getDirty()), true);
                cacheLine->setState(S);
            } else if (!inclusive_) {
                sendResponseUp(reqEvent, state, cacheLine->getData(), true);
                cacheLine->setOwner(reqEvent->getSrc());
            } else if (protocol_) {
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "New owner: %s\n", reqEvent->getSrc().c_str());
                cacheLine->setOwner(reqEvent->getSrc());
                 sendResponseUp(reqEvent, E, cacheLine->getData(), true);
            } else {
                cacheLine->addSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, S, cacheLine->getData(), true);
            }
            break;
        default:
	    d_->fatal(CALL_INFO, -1, "%s, Error: Updating data but cache is not in E or M state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 *  Handle Inv requests.
 *  These can only be sent to caches in shared state.
 *  Special cases & return vaules:
 *      BLOCK: Cache is already invalidating, stall the incoming Inv for the in-progress request
 *      STALL: Cache is already invalidating, but handle this Inv before the in-progress request
 *      State=SI: Cache is evicting and waiting for AckInvs, handle this invalidation instead of completing eviction
 */
CacheAction MESIController::handleInv(MemEvent* event, CacheLine* cacheLine, bool replay) {
    
    State state = cacheLine->getState();
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);
    
    switch(state) {
        case I:
        case IS:
        case IM:
            return DONE;    // Eviction raced with Inv, IS/IM only happen if we don't use AckPuts
        case S:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(S_Inv);
                return STALL;
            }
            sendAckInv(event->getBaseAddr(), event->getRqstr());
            inc_InvalidatePUTSReqSent();
            cacheLine->setState(I);
            return DONE;
        case SM:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SM_Inv);
                return STALL;
            }
            sendAckInv(event->getBaseAddr(), event->getRqstr());
            inc_InvalidatePUTSReqSent();
            cacheLine->setState(IM);
            return DONE;
        case S_Inv: // PutS in progress, stall this Inv for that
            return BLOCK;
        case SI: // PutS in progress, turn it into an Inv in progress
            cacheLine->setState(S_Inv);
        case SM_Inv:    // Waiting on GetSResp & AckInvs, stall this Inv until invacks come back
            return STALL;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an invalidation in an unhandled state: %s. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;
}


/**
 *  Handle FetchInv requests
 *  FetchInvs are a request to get data (Fetch) and invalidate the local copy.
 *  Generally sent to caches with exclusive blocks but a non-inclusive cache may send to a sharer instead of an Inv 
 *  if the non-inclusive cache does not have a cached copy of the block
 */
CacheAction MESIController::handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case S: // Happens when there is a non-inclusive cache below us
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(S_Inv);
                return STALL;
            }
            break;  
        case SM:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SM_Inv);
                return STALL;
            }
            inc_FetchInvReqSent();
            sendResponseDown(event, cacheLine, false, replay);
            cacheLine->setState(IM);
            return DONE;
        case E:
            if (cacheLine->ownerExists()) {
                sendFetchInv(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(E_Inv);
                return STALL;
            }
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(E_Inv);
                return STALL;
            }
            break;
        case M:
            if (cacheLine->ownerExists()) {
                sendFetchInv(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(M_Inv);
                return STALL;
            }
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(M_Inv);
                return STALL;
            }
            break;
        case EI:
            cacheLine->setState(E_Inv);
            return STALL;
        case MI:
            cacheLine->setState(M_Inv);
            return STALL;
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            return BLOCK;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    inc_FetchInvReqSent();
    sendResponseDown(event, cacheLine, (state == M), replay);
    cacheLine->setState(I);
    return DONE;
}


/**
 *  Handle FetchInvX.
 *  A FetchInvX is a request to downgrade the block to Shared state (from E or M) and to include a copy of the block
 */
CacheAction MESIController::handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case E:
            if (cacheLine->ownerExists()) {
                sendFetchInvX(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(E_InvX);
                return STALL;
            }
            break;
        case M:
            if (cacheLine->ownerExists()) {
                sendFetchInvX(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(M_InvX);
                return STALL;
            }
            break;
        case EI:
        case MI:
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            return BLOCK;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    inc_FetchInvXReqSent();
    sendResponseDown(event, cacheLine, (state == M), replay);
    cacheLine->setState(S);
    return DONE;
}


/**
 *  Handle Fetch requests.
 *  A Fetch is a request for a copy of the block.
 *  These are only sent by non-inclusive caches that do not have a local copy of the block
 *  The request does not change coherence state.
 */
CacheAction MESIController::handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay) {
    State state = cacheLine->getState();
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
            return IGNORE;
        case SM:
        case S:
            sendResponseDown(event, cacheLine, false, replay);
            return DONE;
        case S_Inv:
        case SI:
            return BLOCK;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch but state is unhandled. Addr = 0x%" PRIx64 ", Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                        name_.c_str(), event->getAddr(), event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/**
 *  Handle data responses (GetSResp and GetXResp)
 *  Send message up if request was from an upper cache
 *  Simply cache data locally if request was a local prefetch
 */
CacheAction MESIController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    if (!inclusive_ && (cacheLine == NULL || cacheLine->getState() == I)) {
        sendResponseUp(origRequest, responseEvent->getGrantedState(), &responseEvent->getPayload(), true);
        return DONE;
    }

    State state = cacheLine->getState();
    stateStats_[responseEvent->getCmd()][state]++;
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    
    switch (state) {
        case IS:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);
            if (responseEvent->getGrantedState() == E) cacheLine->setState(E);
            else if (responseEvent->getGrantedState() == M) cacheLine->setState(M);
            else cacheLine->setState(S);
            inc_GETSHit(origRequest);
            if (!shouldRespond) return DONE;
            if (cacheLine->getState() == E || cacheLine->getState() == M) {
                cacheLine->setOwner(origRequest->getSrc());
            } else {
                cacheLine->addSharer(origRequest->getSrc());
            }
            sendResponseUp(origRequest, cacheLine->getState(), cacheLine->getData(), true);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        case IM:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);
        case SM:
            cacheLine->setState(M);
            cacheLine->setOwner(origRequest->getSrc());
            if (cacheLine->isSharer(origRequest->getSrc())) cacheLine->removeSharer(origRequest->getSrc());
            sendResponseUp(origRequest, M, cacheLine->getData(), true);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        case SM_Inv:
            cacheLine->setState(M_Inv);
            return STALL;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/**
 *  Handle Fetch responses (responses to Fetch, FetchInv, and FetchInvX)
 */
CacheAction MESIController::handleFetchResp(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent) {
    State state = (cacheLine == NULL) ? I : cacheLine->getState();
    
    // Check acks needed
    if (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(responseEvent->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) == 0) ? DONE : IGNORE;

    // Update data
    if (state != I) cacheLine->setData(responseEvent->getPayload(), responseEvent);
    if (state != I && (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr())) printData(cacheLine->getData(), true);

    stateStats_[responseEvent->getCmd()][state]++;
    recordStateEventCount(responseEvent->getCmd(), state);

    switch (state) {
        case I:
            // Sanity check that this is a non-inclusive cache!
            if (inclusive_) {
                d_->fatal(CALL_INFO, -1, "%s, Error: Inclusive cache received a FetchResp for a non-cached address. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
            } else if (action != DONE) {
                d_->fatal(CALL_INFO, -1, "%s, Error: Non-inclusive cache received a FetchResp for a non-cached address and is waiting for more acks. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
            }
            sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
            break;
        case EI:
            inc_EvictionPUTEReqSent();
            sendWriteback(responseEvent->getDirty() ? PutM : PutE, cacheLine, responseEvent->getDirty(), name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            inc_EvictionPUTEReqSent();
            sendWriteback(PutM, cacheLine, true, name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case E_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == FetchInvX) {
                inc_FetchInvXReqSent();
                sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
                cacheLine->setState(S);
            } else {
                inc_GETSHit(reqEvent);
                cacheLine->addSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, S, cacheLine->getData(), true);
                if (responseEvent->getDirty()) cacheLine->setState(M);
                else cacheLine->setState(E);
            }
            break;
        case E_Inv:
            if (cacheLine->isSharer(responseEvent->getSrc())) cacheLine->removeSharer(responseEvent->getSrc());
            if (cacheLine->getOwner() == responseEvent->getSrc()) cacheLine->clearOwner();
            inc_FetchInvReqSent();
            sendResponseDown(reqEvent, cacheLine, responseEvent->getDirty(), true);
            cacheLine->setState(I);
            break;
        case M_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == FetchInvX) {
                inc_FetchInvReqSent();
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(S);
            } else {    // reqEvent->getCmd() == GetS
                inc_GETSHit(reqEvent);
                cacheLine->addSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, S, cacheLine->getData(), true);
                cacheLine->setState(M);
            }
            break;
        case M_Inv:
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == FetchInv) {
                inc_FetchInvReqSent();
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else {    // reqEvent->getCmd() == GetX
                if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                inc_GETXHit(reqEvent);
                cacheLine->setOwner(reqEvent->getSrc());
                if (cacheLine->isSharer(reqEvent->getSrc())) cacheLine->removeSharer(reqEvent->getSrc());
                sendResponseUp(reqEvent, M, cacheLine->getData(), true);
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(cacheLine->getData(), false);
                cacheLine->setState(M);
            }
            break;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received a FetchResp and state is unhandled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return action;
}


/**
 *  Handle AckInvs (response to Inv requests)
 */
CacheAction MESIController::handleAckInv(MemEvent * ack, CacheLine * line, MemEvent * reqEvent) {
    State state = (line == NULL) ? I : line->getState();
    
    stateStats_[ack->getCmd()][state]++;
    recordStateEventCount(ack->getCmd(), state);

    if (line && line->isSharer(ack->getSrc())) {
        line->removeSharer(ack->getSrc());
    }
    if (DEBUG_ALL || DEBUG_ADDR == ack->getBaseAddr()) d_->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    
    switch (state) {
        case I:
            if (action == DONE) {
                sendAckInv(ack->getBaseAddr(), ack->getRqstr());
                inc_InvalidatePUTSReqSent();
            }
            return action;
        case S_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(I);
                } else {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                    inc_InvalidatePUTSReqSent();
                    line->setState(I);
                }
            }
            return action;
        case SM_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Inv) {
                    if (line->numSharers() > 0) {
                        invalidateAllSharers(line, reqEvent->getRqstr(), true);
                        return IGNORE;
                    } else {
                        sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
                        inc_InvalidatePUTSReqSent();
                        line->setState(IM);
                    }
                } else if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(IM);
                } else {
                    line->setState(SM);
                    return IGNORE;
                }
            }
            return action;

        case SI:
            if (action == DONE) {
                inc_EvictionPUTSReqSent();
                sendWriteback(PutS, line, false, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case EI:
            if (action == DONE) {
                inc_EvictionPUTSReqSent();
                sendWriteback(PutE, line, false, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case MI:
            if (action == DONE) {
                inc_EvictionPUTSReqSent();
                sendWriteback(PutM, line, true, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case E_Inv:
            if (action == DONE) {
                inc_FetchInvReqSent();
                sendResponseDown(reqEvent, line, false, true);
                line->setState(I);
            }
            return action;
        case M_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    inc_FetchInvReqSent();
                    sendResponseDown(reqEvent, line, true, true);
                    line->setState(I);
                } else { // reqEvent->getCmd() == GetX/GetSEx
                    if (reqEvent->getCmd() == GetSEx) inc_GetSExReqsReceived(true);
                    inc_GETXHit(reqEvent);
                    line->setOwner(reqEvent->getSrc());
                    if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                    sendResponseUp(reqEvent, M, line->getData(), true);
                    if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(line->getData(), false);
                    line->setState(M);
                }
            }
            return action;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Received AckInv in unhandled state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), ack->getBaseAddr(), CommandString[ack->getCmd()], ack->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());

    }
    return action;    // eliminate compiler warning
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface 
 *---------------------------------------------------------------------------------------------------------------------*/

/**
 *  Send an Inv to all sharers of the block. Used for evictions or Inv/FetchInv requests from lower level caches
 */
void MESIController::invalidateAllSharers(CacheLine * cacheLine, string rqstr, bool replay) {
    set<std::string> * sharers = cacheLine->getSharers();
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
    
        uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
    }
}


/**
 *  Send an Inv to all sharers unless the cache requesting exclusive permission is a sharer; then send Inv to all sharers except requestor. 
 *  Used for GetX/GetSEx requests.
 */
bool MESIController::invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay) {
    bool sentInv = false;
    set<std::string> * sharers = cacheLine->getSharers();
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        if (*it == rqstr) continue;

        MemEvent * inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(origRqstr);

        uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);
        sentInv = true;
        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
    }
    return sentInv;
}


/**
 *  Send FetchInv to owner of a block
 */
void MESIController::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
   
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
}


/** 
 *  Send FetchInv to owner of a block
 */
void MESIController::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    
    uint64_t deliveryTime = (replay) ? timestamp_ + mshrLatency_ : timestamp_ + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
}


/**
 *  Forward message to an upper level cache.
 *  Called both externally and internally
 */
void MESIController::forwardMessageUp(MemEvent* event) {
    MemEvent * forwardEvent = new MemEvent(*event);
    forwardEvent->setSrc(name_);
    forwardEvent->setDst(upperLevelCacheNames_[0]);
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response fwdReq = {forwardEvent, deliveryTime, false};
    addToOutgoingQueueUp(fwdReq);
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_, "Forwarding %s to %s at cycle = %" PRIu64 "\n", CommandString[forwardEvent->getCmd()], forwardEvent->getDst().c_str(), deliveryTime);
}

/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload  
 */
void MESIController::sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool dirty, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*cacheLine->getData());
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
    responseEvent->setSize(cacheLine->getSize());
    
    responseEvent->setDirty(dirty);

    uint64 deliveryTime = replay ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) { 
        d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}

/**
 *  Send response down (e.g., a FetchResp or AckInv) using an incoming event instead of the locally cached cacheline
 */
void MESIController::sendResponseDownFromMSHR(MemEvent * respEvent, MemEvent * reqEvent, bool dirty) {
    MemEvent * newResponseEvent = reqEvent->makeResponse();
    newResponseEvent->setPayload(respEvent->getPayload());
    newResponseEvent->setSize(respEvent->getSize());
    newResponseEvent->setDirty(dirty);

    uint64_t deliveryTime = timestamp_ + mshrLatency_;
    Response resp = {newResponseEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == respEvent->getBaseAddr()) {
        d_->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[newResponseEvent->getCmd()], newResponseEvent->getSrc().c_str());
    }
}


/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIController::sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string rqstr) {
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    if (cmd == PutM || writebackCleanBlocks_) {
        newCommandEvent->setSize(cacheLine->getSize());
        newCommandEvent->setPayload(*cacheLine->getData());
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) printData(cacheLine->getData(), false);
    }
    newCommandEvent->setRqstr(rqstr);
    newCommandEvent->setDirty(dirty);
    
    
    uint64 deliveryTime = timestamp_ + accessLatency_;
    Response resp = {newCommandEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. Cache index = %d\n", deliveryTime, CommandString[cmd], cacheLine->getIndex());
}

/**
 *  Send a writeback ack. Mostly used by non-inclusive caches.
 */
void MESIController::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, event->getBaseAddr(), event->getBaseAddr(), AckPut);
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_, "Sending AckPut at cycle = %" PRIu64 "\n", deliveryTime);
}


/**
 *  Send an AckInv as a response to an Inv
 */
void MESIController::sendAckInv(Addr baseAddr, string origRqstr) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, AckInv);
    ack->setDst(getDestination(baseAddr));
    ack->setRqstr(origRqstr);
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueue(resp);
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
}


/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/


/** Print value of data blocks for debugging */
void MESIController::printData(vector<uint8_t> * data, bool set) {
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
void MESIController::printStats(int _stats, vector<int> groupIds, map<int, CtrlStats> ctrlStats, uint64_t upgradeLatency, 
        uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM,
        uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
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

void MESIController::printStatsForMacSim(int statLocation, vector<int> groupIds, map<int, CtrlStats> ctrlStats, uint64_t upgradeLatency, 
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

