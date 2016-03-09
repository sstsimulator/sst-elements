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
CacheAction MESIController::handleEviction(CacheLine* wbCacheLine, string rqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
    recordEvictionState(state);

    Addr wbBaseAddr = wbCacheLine->getBaseAddr();
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L6_, "Handling eviction at cache for addr 0x%" PRIx64 " with index %d\n", wbBaseAddr, wbCacheLine->getIndex());
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
#endif
                return STALL;
            }
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
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
#endif
                return STALL;
            }
            if (wbCacheLine->ownerExists()) {
                sendFetchInv(wbCacheLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                wbCacheLine->setState(EI);
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating owner\n");
#endif
                return STALL;
            }
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
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating sharers\n");
#endif
                return STALL;
            }
            if (wbCacheLine->ownerExists()) {
                sendFetchInv(wbCacheLine, name_, false);
                mshr_->incrementAcksNeeded(wbBaseAddr);
                wbCacheLine->setState(MI);
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == wbBaseAddr) d_->debug(_L7_, "Eviction requires invalidating owner\n");
#endif
                return STALL;
            }
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
    Command cmd = event->getCmd();

    switch(cmd) {
        case PutS:
            return handlePutSRequest(event, cacheLine, reqEvent);
        case PutM:
        case PutE:
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
            recordStateEventCount(respEvent->getCmd(), I);
            mshr_->removeWriteback(respEvent->getBaseAddr());
            return DONE;    // Retry any events that were stalled for ack
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


bool MESIController::isRetryNeeded(MemEvent* event, CacheLine* cacheLine) {
    Command cmd = event->getCmd();
    State state = cacheLine ? cacheLine->getState() : I;
    
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
            return true;
        case PutS:
        case PutE:
        case PutM:
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) 
                if (!mshr_->pendingWriteback(cacheLine->getBaseAddr())) return false;
            return true;
        case FetchInv:
        case FetchInvX:
            if (state == I) return false;   // Already resolved the request, don't resend
            if (cacheLine->getOwner() != event->getDst()) return false;    // Must have gotten a replacement from this owner
            return true;
        case Inv:
            if (state == I) return false;   // Already resolved the request, don't resend
            if (!cacheLine->isSharer(event->getDst())) return false;    // Must have gotten a replacement from this sharer
            return true;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: NACKed event is unrecognized: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return true;
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
            if (cmd == GetS) return 0;  // hit
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
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
    
    uint64_t sendTime = 0;
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == name_));
    recordStateEventCount(event->getCmd(), state);
    switch (state) {
        case I:
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), 0, NULL);
            cacheLine->setTimestamp(sendTime);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            cacheLine->addSharer(event->getSrc());
            sendTime = sendResponseUp(event, S, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            return DONE;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            
            // For Non-inclusive caches, we will deallocate this block so E/M permission needs to transfer!
            // Alternately could write back
            if (!inclusive_) {
                sendTime = sendResponseUp(event, state, data, replay, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                cacheLine->setOwner(event->getSrc());
                return DONE;
            }

            if (cacheLine->isShareless() && !cacheLine->ownerExists() && protocol_) {
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "New owner: %s\n", event->getSrc().c_str());
#endif
                cacheLine->setOwner(event->getSrc());
                sendTime = sendResponseUp(event, E, data, replay, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                return DONE;
            }
            if (cacheLine->ownerExists()) {
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"GetS request but exclusive owner exists \n");
#endif
                sendFetchInvX(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                if (state == E) cacheLine->setState(E_InvX);
                else cacheLine->setState(M_InvX);
                return STALL;
            }
            cacheLine->addSharer(event->getSrc());
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


/** Handle GetX or GetSEx (Read-lock) */
CacheAction MESIController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    if (state != SM) recordStateEventCount(event->getCmd(), state);
   
    uint64_t sendTime = 0;
    
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
            if (invalidateSharersExceptRequestor(cacheLine, event->getSrc(), event->getRqstr(), replay)) {
                cacheLine->setState(SM_Inv);
            } else {
                cacheLine->setState(SM);
                cacheLine->setTimestamp(sendTime);
            }
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);

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
            sendTime = sendResponseUp(event, M, cacheLine->getData(), replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(line->getData(), true);
#endif
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

    uint64_t sendTime = 0;
    
    switch (state) {
        case I:
        case S:
        case E:
        case M:
            if (!inclusive_) sendWritebackAck(event);
            return DONE;
        /* Races with evictions */
        case SI:
            sendWriteback(PutS, line, false, name_);
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case EI:
            sendWriteback(PutE, line, false, name_);
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case MI:
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
                line->setState(I);
            } else {
                sendResponseDown(reqEvent, line, false, true);
                line->setState(I);
            }
            return DONE;
        case E_Inv:
            sendResponseDown(reqEvent, line, false, true);
            line->setState(I);
            return DONE;
        case M_Inv:
            if (reqEvent->getCmd() == FetchInv) {
                sendResponseDown(reqEvent, line, true, true);
                line->setState(I);
            } else if (reqEvent->getCmd() == GetX || reqEvent->getCmd() == GetSEx) {
                line->setOwner(reqEvent->getSrc());
                if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, M, line->getData(), true, line->getTimestamp());
                line->setTimestamp(sendTime);
                line->setState(M);
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(line->getData(), false);
#endif
            }
            return DONE;
        case SM_Inv:
            if (reqEvent->getCmd() == Inv) {
                if (line->numSharers() > 0) {
                    invalidateAllSharers(line, reqEvent->getRqstr(), true);
                    return IGNORE;
                } else {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
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

    recordStateEventCount(event->getCmd(), state);
    
    if (!inclusive_) {
        if (cacheLine == NULL) {    // Didn't wait for allocation because a reqEvent is dependent on this replacement
            if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
            if (reqEvent->getCmd() == FetchInvX) {
                // Handle FetchInvX and keep stalling Put*
                sendResponseDownFromMSHR(event, reqEvent, event->getDirty());
                return STALL;
            } else if (reqEvent->getCmd() == FetchInv) {  
                // HandleFetchInv and retire Put*
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
    
    uint64_t sendTime = 0;
    
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
                sendWriteback(PutM, cacheLine, true, name_);
            } else {
                sendWriteback(PutE, cacheLine, false, name_);
            }
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            sendWriteback(PutM, cacheLine, true, name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        /* Races with FetchInv from outside our sub-hierarchy; races with GetX from within our sub-hierarchy */
        case E_Inv:
            sendResponseDown(reqEvent, cacheLine, event->getDirty(), true);
            cacheLine->setState(I);
            break;
        case M_Inv:
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == FetchInv) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else {
                cacheLine->setState(M);
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                cacheLine->setOwner(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, M, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
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
                sendResponseDown(reqEvent, cacheLine, (state == M_InvX || event->getDirty()), true);
                cacheLine->setState(S);
            } else if (!inclusive_) {
                sendTime = sendResponseUp(reqEvent, state, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                cacheLine->setOwner(reqEvent->getSrc());
            } else if (protocol_) {
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "New owner: %s\n", reqEvent->getSrc().c_str());
#endif
                cacheLine->setOwner(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, E, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
            } else {
                cacheLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, S, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
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
            cacheLine->setState(I);
            return DONE;
        case SM:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SM_Inv);
                return STALL;
            }
            sendAckInv(event->getBaseAddr(), event->getRqstr());
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
        uint64_t sendTime = sendResponseUp(origRequest, responseEvent->getGrantedState(), &responseEvent->getPayload(), true, 0);
        if (cacheLine != NULL) cacheLine->setTimestamp(sendTime);
        return DONE;
    }

    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == name_));
    
    uint64_t sendTime = 0;
    
    switch (state) {
        case IS:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);
#endif
            if (responseEvent->getGrantedState() == E) cacheLine->setState(E);
            else if (responseEvent->getGrantedState() == M) cacheLine->setState(M);
            else cacheLine->setState(S);
            notifyListenerOfAccess(origRequest, NotifyAccessType::READ, NotifyResultType::HIT);
            if (!shouldRespond) return DONE;
            if (cacheLine->getState() == E || cacheLine->getState() == M) {
                cacheLine->setOwner(origRequest->getSrc());
            } else {
                cacheLine->addSharer(origRequest->getSrc());
            }
            sendTime = sendResponseUp(origRequest, cacheLine->getState(), cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
            return DONE;
        case IM:
            cacheLine->setData(responseEvent->getPayload(), responseEvent);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);
#endif
        case SM:
            cacheLine->setState(M);
            cacheLine->setOwner(origRequest->getSrc());
            if (cacheLine->isSharer(origRequest->getSrc())) cacheLine->removeSharer(origRequest->getSrc());
            notifyListenerOfAccess(origRequest, NotifyAccessType::WRITE, NotifyResultType::HIT);
            sendTime = sendResponseUp(origRequest, M, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    if (state != I && (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr())) printData(cacheLine->getData(), true);
#endif

    recordStateEventCount(responseEvent->getCmd(), state);
    
    uint64_t sendTime = 0;

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
            sendWriteback(responseEvent->getDirty() ? PutM : PutE, cacheLine, responseEvent->getDirty(), name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            sendWriteback(PutM, cacheLine, true, name_);
	    cacheLine->setState(I);    // wait for ack
            if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case E_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == FetchInvX) {
                sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
                cacheLine->setState(S);
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                cacheLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, S, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                if (responseEvent->getDirty()) cacheLine->setState(M);
                else cacheLine->setState(E);
            }
            break;
        case E_Inv:
            if (cacheLine->isSharer(responseEvent->getSrc())) cacheLine->removeSharer(responseEvent->getSrc());
            if (cacheLine->getOwner() == responseEvent->getSrc()) cacheLine->clearOwner();
            sendResponseDown(reqEvent, cacheLine, responseEvent->getDirty(), true);
            cacheLine->setState(I);
            break;
        case M_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == FetchInvX) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(S);
            } else {    // reqEvent->getCmd() == GetS
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                cacheLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, S, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                cacheLine->setState(M);
            }
            break;
        case M_Inv:
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == FetchInv) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else {    // reqEvent->getCmd() == GetX
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                cacheLine->setOwner(reqEvent->getSrc());
                if (cacheLine->isSharer(reqEvent->getSrc())) cacheLine->removeSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, M, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
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
    
    recordStateEventCount(ack->getCmd(), state);

    if (line && line->isSharer(ack->getSrc())) {
        line->removeSharer(ack->getSrc());
    }
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == ack->getBaseAddr()) d_->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
#endif
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    
    uint64_t sendTime = 0;
    
    switch (state) {
        case I:
            if (action == DONE) {
                sendAckInv(ack->getBaseAddr(), ack->getRqstr());
            }
            return action;
        case S_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(I);
                } else {
                    sendAckInv(reqEvent->getBaseAddr(), reqEvent->getRqstr());
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
                        line->setState(IM);
                    }
                } else if (reqEvent->getCmd() == FetchInv) {
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
                sendWriteback(PutS, line, false, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case EI:
            if (action == DONE) {
                sendWriteback(PutE, line, false, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case MI:
            if (action == DONE) {
                sendWriteback(PutM, line, true, name_);
                if (!LL_ && (LLC_ || writebackCleanBlocks_)) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case E_Inv:
            if (action == DONE) {
                sendResponseDown(reqEvent, line, false, true);
                line->setState(I);
            }
            return action;
        case M_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == FetchInv) {
                    sendResponseDown(reqEvent, line, true, true);
                    line->setState(I);
                } else { // reqEvent->getCmd() == GetX/GetSEx
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    line->setOwner(reqEvent->getSrc());
                    if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, M, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime);
#ifdef __SST_DEBUG_OUTPUT__
                    if (DEBUG_ALL || DEBUG_ADDR == reqEvent->getBaseAddr()) printData(line->getData(), false);
#endif
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
    uint64_t deliveryTime = 0;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
        inv->setSize(cacheLine->getSize());
    
        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
#endif
    }
    if (deliveryTime != 0) cacheLine->setTimestamp(deliveryTime);
}


/**
 *  Send an Inv to all sharers unless the cache requesting exclusive permission is a sharer; then send Inv to all sharers except requestor. 
 *  Used for GetX/GetSEx requests.
 */
bool MESIController::invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay) {
    bool sentInv = false;
    set<std::string> * sharers = cacheLine->getSharers();
    uint64_t deliveryTime = 0;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        if (*it == rqstr) continue;

        MemEvent * inv = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Inv);
        inv->setDst(*it);
        inv->setRqstr(origRqstr);
        inv->setSize(cacheLine->getSize());

        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, false};
        addToOutgoingQueueUp(resp);
        sentInv = true;
        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
#endif
    }
    if (deliveryTime != 0) cacheLine->setTimestamp(deliveryTime);
    return sentInv;
}


/**
 *  Send FetchInv to owner of a block
 */
void MESIController::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInv);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
   
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
#endif
}


/** 
 *  Send FetchInv to owner of a block
 */
void MESIController::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent((Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());

    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, false};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
            cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_, "Forwarding %s to %s at cycle = %" PRIu64 "\n", CommandString[forwardEvent->getCmd()], forwardEvent->getDst().c_str(), deliveryTime);
#endif
}

/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload  
 */
void MESIController::sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool dirty, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*cacheLine->getData());
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
    responseEvent->setSize(cacheLine->getSize());
    
    responseEvent->setDirty(dirty);

    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
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
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == respEvent->getBaseAddr()) {
        d_->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[newResponseEvent->getCmd()], newResponseEvent->getSrc().c_str());
    }
#endif
}


/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIController::sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string rqstr) {
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    newCommandEvent->setSize(cacheLine->getSize());
    bool hasData = false;
    if (cmd == PutM || writebackCleanBlocks_) {
        newCommandEvent->setPayload(*cacheLine->getData());
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) printData(cacheLine->getData(), false);
#endif
        hasData = true;
    }
    newCommandEvent->setRqstr(rqstr);
    newCommandEvent->setDirty(dirty);
    
    
    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64 deliveryTime = hasData ? baseTime + accessLatency_ : baseTime + tagLatency_;
    Response resp = {newCommandEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. Cache index = %d\n", deliveryTime, CommandString[cmd], cacheLine->getIndex());
#endif
}

/**
 *  Send a writeback ack. Mostly used by non-inclusive caches.
 */
void MESIController::sendWritebackAck(MemEvent * event) {
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


/**
 *  Send an AckInv as a response to an Inv
 */
void MESIController::sendAckInv(Addr baseAddr, string origRqstr) {
    MemEvent * ack = new MemEvent((SST::Component*)owner_, baseAddr, baseAddr, AckInv);
    ack->setDst(getDestination(baseAddr));
    ack->setRqstr(origRqstr);
    ack->setSize(lineSize_); // Number of bytes invalidated

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, false};
    addToOutgoingQueue(resp);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
#endif
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

