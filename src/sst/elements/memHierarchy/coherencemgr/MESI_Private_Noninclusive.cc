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
#include "coherencemgr/MESI_Private_Noninclusive.h"

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
 * MESI Coherence Controller Implementation
 * Provides MESI & MSI coherence for all non-L1 caches
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
CacheAction MESIPrivNoninclusive::handleEviction(CacheLine* wbCacheLine, string rqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
    recordEvictionState(state);

    Addr wbBaseAddr = wbCacheLine->getBaseAddr();
    
    if (is_debug_addr(wbBaseAddr)) debug->debug(_L6_, "Handling eviction at cache for addr 0x%" PRIx64 " with index %d\n", wbBaseAddr, wbCacheLine->getIndex());
    
    switch(state) {
        case SI:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
        case E_InvX:
        case M_InvX:
        case S_B:
        case I_B:
            return STALL;
        case I:
            return DONE;
        case S:
            if (wbCacheLine->getPrefetch()) {
                wbCacheLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (wbCacheLine->numSharers() > 0) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (!silentEvictClean_) sendWriteback(Command::PutS, wbCacheLine, false, rqstr);
            if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
            wbCacheLine->setState(I);    // wait for ack
	    return DONE;
        case E:
            if (wbCacheLine->getPrefetch()) {
                wbCacheLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (wbCacheLine->ownerExists()) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (wbCacheLine->numSharers() > 0) {
                invalidateAllSharers(wbCacheLine, ownerName_, false); 
                wbCacheLine->setState(EI);
                
                if (is_debug_addr(wbBaseAddr)) debug->debug(_L7_, "Eviction requires invalidating sharers\n");
                
                return STALL;
            }
            
            if (!silentEvictClean_) sendWriteback(Command::PutE, wbCacheLine, false, rqstr);
            if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
	    wbCacheLine->setState(I);    // wait for ack
	    return DONE;
        case M:
            if (wbCacheLine->getPrefetch()) {
                wbCacheLine->setPrefetch(false);
                statPrefetchEvict->addData(1);
            }
            if (wbCacheLine->ownerExists()) {
                wbCacheLine->setState(I);
                return DONE;
            }
            if (wbCacheLine->numSharers() > 0) {
                invalidateAllSharers(wbCacheLine, ownerName_, false); 
                wbCacheLine->setState(MI);
                
                if (is_debug_addr(wbBaseAddr)) debug->debug(_L7_, "Eviction requires invalidating sharers\n");
                
                return STALL;
            }
	    sendWriteback(Command::PutM, wbCacheLine, true, rqstr);
            wbCacheLine->setState(I);    // wait for ack
            if (expectWritebackAck_) mshr_->insertWriteback(wbBaseAddr);
	    return DONE;
        case IS:
        case IM:
        case SM:
            return STALL;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction MESIPrivNoninclusive::handleRequest(MemEvent* event, bool replay) {
    Addr addr = event->getBaseAddr();
    CacheLine * cacheLine = cacheArray_->lookup(addr, !replay);
    

    if (cacheLine && cacheLine->inTransition()) {
        allocateMSHR(addr, event);
        return STALL;
    }

    if (cacheLine == nullptr || !cacheLine->valid()) {
        if (is_debug_addr(addr)) debug->debug(_L3_, "-- Miss --\n");
        allocateMSHR(addr, event);
        if (event->inProgress()) {
            if (is_debug_addr(addr)) debug->debug(_L8_, "Attempted retry too early, continue stalling\n");
            return STALL;
        }

        vector<uint8_t> * data = &event->getPayload();
        forwardMessage(event, addr, event->getSize(), 0, data);
        event->setInProgress(true);
        recordMiss(event->getID());
        return STALL;
    }

    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::GetS:
            return handleGetSRequest(event, cacheLine, replay);
        case Command::GetX:
        case Command::GetSX:
            return handleGetXRequest(event, cacheLine, replay);
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request. Event = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}


/**
 *  Handle replacement
 */
CacheAction MESIPrivNoninclusive::handleReplacement(MemEvent* event, bool replay) {
    Addr addr = event->getBaseAddr();
    CacheLine* cacheLine = cacheArray_->lookup(addr, true);
    if (cacheLine == nullptr) {
        if (is_debug_addr(addr)) debug->debug(_L3_, "-- Cache Miss --\n");

        if (!allocateLine(addr, event)) {
            if (mshr_->getAcksNeeded(addr) == 0) {
                allocateMSHR(addr, event);
                return STALL;
            } else {
                if (is_debug_addr(addr)) debug->debug(_L3_, "Can't allocate immediately, but handling collision anyways\n");
            }
        }
        cacheLine = cacheArray_->lookup(addr, false);
    }

    MemEvent * reqEvent = (mshr_->exists(addr)) ? static_cast<MemEvent*>(mshr_->lookupFront(addr)) : nullptr;

    CacheAction action = DONE;
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::PutS:
            action = handlePutSRequest(event, cacheLine, reqEvent);
            break;
        case Command::PutM:
        case Command::PutE:
            action = handlePutMRequest(event, cacheLine, reqEvent);
            break;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request. Event = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if ((action == DONE || action == STALL) && reqEvent != nullptr) {
        mshr_->removeFront(addr);
        delete reqEvent;
    }
    if (action == STALL || action == BLOCK)
        allocateMSHR(addr, event);
    
    return action;
}

CacheAction MESIPrivNoninclusive::handleFlush(MemEvent* event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay) {
    CacheAction action = DONE;
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::FlushLineInv:
            action = handleFlushLineInvRequest(event, cacheLine, reqEvent, replay);
            break;
        case Command::FlushLine:
            action = handleFlushLineRequest(event, cacheLine, reqEvent, replay);
            break;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request. Event = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    return action;
}


/**
 *  Handle invalidations.
 *  Special cases exist when the invalidation races with a writeback that is queued in the MSHR.
 *  Non-inclusive caches which miss locally simply forward the request to the next higher cache
 */
CacheAction MESIPrivNoninclusive::handleInvalidationRequest(MemEvent * event, bool replay) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* cacheLine = cacheArray_->lookup(bAddr, false);

    if (is_debug_addr(bAddr))
        printLine(bAddr, cacheLine);

    if (!mshr_->pendingWriteback(bAddr) && mshr_->isFull()) {
        processInvRequestInMSHR(bAddr, event, false);
        return STALL;
    }

    /*
     *  Possible races: 
     *  Received Inv/FetchInv/FetchInvX and am waiting on AckPut or have stalled Put* for another eviction (only for noninclusive), or am waiting on a FlushResp (to FlushLineInv or FlushLine)
     */
    MemEvent* collisionEvent = nullptr;
    if (mshr_->exists(bAddr))
        collisionEvent = static_cast<MemEvent*>(mshr_->lookupFront(bAddr));

    if (!cacheLine || cacheLine->getState() == I) { // Either raced with another request or the block is present in an upper level cache (non-inclusive only)
        recordStateEventCount(event->getCmd(), I);

        // Was waiting for AckPut, treat Inv/FetchInv/FetchInvX as AckPut and do not respond
        if (mshr_->pendingWriteback(bAddr)) {
            mshr_->removeWriteback(bAddr);
            delete event;
            return DONE;
        }
        
        if (collisionEvent && collisionEvent->isWriteback()) {
            processInvRequestInMSHR(bAddr, event, true);
            return BLOCK; // We are waiting for an open cache line for a Put
        } else if (collisionEvent && (collisionEvent->getCmd() == Command::FlushLineInv || (collisionEvent->getCmd() == Command::FlushLine && event->getCmd() == Command::FetchInvX))) {
            delete event;
            return IGNORE; // Our FlushLine will serve as the Inv response
        } else { // Line is just cached elsewhere
            forwardMessageUp(event);
            mshr_->setAcksNeeded(bAddr, 1);
            event->setInProgress(true);
            processInvRequestInMSHR(bAddr, event, false);
            return STALL;
        }
    }

    Command cmd = event->getCmd();
    CacheAction action = STALL;
    switch (cmd) {
        case Command::Inv: 
            action = handleInv(event, cacheLine, replay);
            break;
        case Command::Fetch:
            action = handleFetch(event, cacheLine, replay);
            break;
        case Command::FetchInv:
            action = handleFetchInv(event, cacheLine, collisionEvent, replay);
            break;
        case Command::FetchInvX:
            action = handleFetchInvX(event, cacheLine, collisionEvent, replay);
            break;
        case Command::ForceInv:
            action = handleForceInv(event, cacheLine, replay);
            break;
        default:
	    debug->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized invalidation. Event = %s. Time = %" PRIu64 "ns\n", 
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(bAddr))
        printLine(bAddr, cacheLine);
    
    if (action == STALL)
        processInvRequestInMSHR(bAddr, event, false);
    else if (action == BLOCK)
        processInvRequestInMSHR(bAddr, event, true);
    else
        delete event;

    return action;
}


/**
 *  Handle responses including data (GetSResp, GetXResp), Inv/Fetch (FetchResp, FetchXResp, AckInv) and writeback acks (AckPut)
 */
CacheAction MESIPrivNoninclusive::handleCacheResponse(MemEvent * event, bool inMSHR) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* line = cacheArray_->lookup(bAddr, false);

    if (is_debug_addr(bAddr))
        printLine(bAddr, line);

    MemEvent* reqEvent = mshr_->lookupFront(bAddr);

    Command cmd = event->getCmd();
    CacheAction action = DONE;
    switch (cmd) {
        case Command::GetSResp:
        case Command::GetXResp:
            action = handleDataResponse(event, line, reqEvent);
            break;
        case Command::FlushLineResp:
            recordStateEventCount(event->getCmd(), line ? line->getState() : I);
            if (line && line->getState() == S_B) line->setState(S);
            else if (line && line->getState() == I_B) line->setState(I);
            sendFlushResponse(reqEvent, event->success());
            action = DONE;
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(bAddr)) printLine(bAddr, line);
    
    if (action == DONE) {
        mshr_->removeFront(bAddr);
        delete reqEvent;
    }
    
    delete event;

    return action;
}

CacheAction MESIPrivNoninclusive::handleFetchResponse(MemEvent * event, bool inMSHR) {
    Addr bAddr = event->getBaseAddr();
    CacheLine* line = cacheArray_->lookup(bAddr, false);
    
    if (is_debug_addr(bAddr))
        printLine(bAddr, line);

    MemEvent* reqEvent = mshr_->exists(bAddr) ? mshr_->lookupFront(bAddr) : nullptr;
    
    CacheAction action = DONE;
    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::FetchResp:
        case Command::FetchXResp:
            action = handleFetchResp(event, line, reqEvent);
            break;
        case Command::AckInv:
            action = handleAckInv(event, line, reqEvent);
            break;
        case Command::AckPut:
            recordStateEventCount(event->getCmd(), I);
            mshr_->removeWriteback(bAddr);
            action = DONE;    // Retry any events that were stalled for ack
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (is_debug_addr(bAddr))
        printLine(bAddr, line);

    delete event;

    if (action == DONE && reqEvent) {
        mshr_->removeFront(bAddr);
        delete reqEvent;
    }

    return action;
}


/**
 *  Return type of miss. Used for profiling incoming requests at the cacheController
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request)
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition
 */
bool MESIPrivNoninclusive::isCacheHit(MemEvent* event) {
    CacheLine* line = cacheArray_->lookup(event->getBaseAddr(), false);
    Command cmd = event->getCmd();
    State state = line ? line->getState() : I;
    if (cmd == Command::GetSX) cmd = Command::GetX;  // for our purposes these are equal

    if (state == I) return false;
    if (event->isPrefetch() && event->getRqstr() == ownerName_) return true;
    if (state == S && lastLevel_) state = M;
    switch (state) {
        case S:
            if (cmd == Command::GetS) return true;
            return false;
        case E:
        case M:
            if (line->ownerExists()) return false;
            if (cmd == Command::GetS) return true;  // hit
            if (line->isShareless() || (line->isSharer(event->getSrc()) && line->numSharers() == 1)) return true; // Hit
            return false;
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
            return false;
        default:
            return true;   // this is profiling so don't die on unhandled state
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Internal event handlers
 *---------------------------------------------------------------------------------------------------------------------*/


/** Handle GetS request */
CacheAction MESIPrivNoninclusive::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Addr addr = event->getBaseAddr();
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    
    if (is_debug_event(event)) printData(cacheLine->getData(), false);
    
    uint64_t sendTime = 0;
    bool localPrefetch = event->isPrefetch() && (event->getRqstr() == ownerName_);
    recordStateEventCount(event->getCmd(), state);
    switch (state) {
        case I:
            sendTime = forwardMessage(event, addr, cacheLine->getSize(), 0, NULL);
            cacheLine->setTimestamp(sendTime);
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
            cacheLine->setState(IS);
            recordLatencyType(event->getID(), LatType::MISS);
            allocateMSHR(addr, event); 
            return STALL;
        case S:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            cacheLine->addSharer(event->getSrc());
            sendTime = sendResponseUp(event, data, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
        case E:
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            if (localPrefetch) {
                statPrefetchRedundant->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }
            
            // For Non-inclusive caches, we will deallocate this block so E/M permission needs to transfer!
            // Alternately could write back
            sendTime = sendResponseUp(event, Command::GetXResp, data, state == M, replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            cacheLine->setOwner(event->getSrc());
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;

        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());

    }
    return STALL;    // eliminate compiler warning
}


/** Handle GetX or GetSX (Read-lock) */
CacheAction MESIPrivNoninclusive::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Addr addr = event->getBaseAddr();
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    if (state != SM) recordStateEventCount(event->getCmd(), state);
   
    uint64_t sendTime = 0;
    
    /* Special case - if this is the last coherence level (e.g., just mem below), 
     * can upgrade without forwarding request */
    if (state == S && lastLevel_) {
        state = M;
        cacheLine->setState(M);
    }

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
            
            if (cacheLine->getPrefetch()) {
                statPrefetchUpgradeMiss->addData(1);
                cacheLine->setPrefetch(false);
            }
            
            sendTime = forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), cacheLine->getTimestamp(), NULL);
            if (invalidateSharersExceptRequestor(cacheLine, event->getSrc(), event->getRqstr(), replay)) {
                cacheLine->setState(SM_Inv);
            } else {
                cacheLine->setState(SM);
                cacheLine->setTimestamp(sendTime);
            }
            
            recordLatencyType(event->getID(), LatType::UPGRADE);
            allocateMSHR(addr, event); 
            return STALL;
        case E:
            cacheLine->setState(M);
        case M:
            notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            if (cacheLine->getPrefetch()) {
                statPrefetchHit->addData(1);
                cacheLine->setPrefetch(false);
            }

            if (!cacheLine->isShareless()) {
                if (invalidateSharersExceptRequestor(cacheLine, event->getSrc(), event->getRqstr(), replay)) {
                    cacheLine->setState(M_Inv);
                    recordLatencyType(event->getID(), LatType::INV);
                    allocateMSHR(addr, event); 
                    return STALL;
                }
            }
            if (cacheLine->ownerExists()) {
                sendFetchInv(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(M_Inv);
                recordLatencyType(event->getID(), LatType::INV);
                return STALL;
            }
            cacheLine->setOwner(event->getSrc());
            if (cacheLine->isSharer(event->getSrc())) cacheLine->removeSharer(event->getSrc());
            sendTime = sendResponseUp(event, cacheLine->getData(), replay, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            
            if (is_debug_event(event)) printData(cacheLine->getData(), false);
            
            recordLatencyType(event->getID(), LatType::HIT);
            return DONE;
        case SM:
            allocateMSHR(addr, event); 
            return STALL;   // retried this request too soon because we were checking for waiting invalidations
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 * Handle a FlushLine request by writing back line if dirty & forwarding reqest
 */
CacheAction MESIPrivNoninclusive::handleFlushLineRequest(MemEvent * event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    State state = I;
    if (cacheLine != NULL) state = cacheLine->getState();
    if (!replay) recordStateEventCount(event->getCmd(), state);

    CacheAction reqEventAction;
    uint64_t sendTime = 0;
        
    recordLatencyType(event->getID(), LatType::HIT);

    // Handle flush at local level
    switch (state) {
        case I:
        case I_B:
        case S:
        case S_B:
            if (reqEvent != NULL) return STALL;
            break;
        case E:
        case M:
            if (cacheLine->getOwner() == event->getSrc()) {
                cacheLine->clearOwner();
                cacheLine->addSharer(event->getSrc());
                if (event->getDirty()) {
                    cacheLine->setData(event->getPayload(), 0);
                    cacheLine->setState(M);
                }
            }
            if (cacheLine->ownerExists()) {
                sendFetchInvX(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                state == M ? cacheLine->setState(M_InvX) : cacheLine->setState(E_InvX);
                return STALL;
            }
            break;
        case IM:
        case IS:
        case SM:
        case S_Inv:
        case SM_Inv:
        case SI:
            return STALL; // Wait for the Get* request to finish
        case EI:
        case MI:
            if (cacheLine->getOwner() == event->getSrc()) {
                cacheLine->clearOwner();
                cacheLine->addSharer(event->getSrc());
                if (event->getDirty()) {
                    cacheLine->setData(event->getPayload(), 0);
                    cacheLine->setState(MI);
                }
            }
            return STALL;
        case M_Inv:
        case E_Inv:
            if (cacheLine->getOwner() == event->getSrc()) {
                cacheLine->clearOwner();
                cacheLine->addSharer(event->getSrc());
                if (event->getDirty()) {
                    cacheLine->setData(event->getPayload(), 0);
                    cacheLine->setState(M_Inv);
                }
            }
            return STALL;
        case M_InvX:
        case E_InvX:
            if (cacheLine->getOwner() == event->getSrc()) {
                cacheLine->clearOwner();
                cacheLine->addSharer(event->getSrc());
                mshr_->decrementAcksNeeded(event->getBaseAddr());
                if (event->getDirty()) {
                    cacheLine->setData(event->getPayload(), 0);
                    cacheLine->setState(M_InvX);
                }
            }
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInvX) {
                    sendResponseDown(reqEvent, cacheLine, (state == E_InvX || event->getDirty()), true); // calc latency based on where data comes from
                    cacheLine->setState(S);
                } else if (reqEvent->getCmd() == Command::FlushLine) {
                    cacheLine->getState() == E_InvX ? cacheLine->setState(E) : cacheLine->setState(M);
                    return handleFlushLineRequest(reqEvent, cacheLine, NULL, true);
                } else if (reqEvent->getCmd() == Command::FetchInv) { // Occurs if FetchInv raced with our FetchInvX for a flushline
                    if (cacheLine->getState() == M_InvX) cacheLine->setState(M);
                    else cacheLine->setState(E);
                    return handleFetchInv(reqEvent, cacheLine, NULL, true);
                } else { // Need to forward dirty/M so we don't lose that info
                    debug->fatal(CALL_INFO, -1, "%s, Error: Handling not implemented because state not expected: noninclusive cache, state = %s, request = %s. Time = %" PRIu64 " ns\n",
                            ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
                return DONE;
            } else return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    forwardFlushLine(event->getBaseAddr(), event->getRqstr(), cacheLine, Command::FlushLine);
    if (cacheLine && state != I) cacheLine->setState(S_B);
    else if (cacheLine) cacheLine->setState(I_B);
    event->setInProgress(true);
    return STALL;   // wait for response
}


/**
 *  Handle a FlushLineInv request by writing back/invalidating line and forwarding request
 *  May require resolving a race with reqEvent (inv/fetch/etc)
 */
CacheAction MESIPrivNoninclusive::handleFlushLineInvRequest(MemEvent * event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    State state = I;
    if (cacheLine != NULL) state = cacheLine->getState();
    if (!replay) recordStateEventCount(event->getCmd(), state);
    
    recordLatencyType(event->getID(), LatType::HIT);
    // Apply incoming flush -> remove if sharer/owner & update data if dirty
    
    if (cacheLine) {
        if (cacheLine->isSharer(event->getSrc())) {
            cacheLine->removeSharer(event->getSrc());
            if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
        }
        if (cacheLine->getOwner() == event->getSrc()) {
            cacheLine->clearOwner();
            if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
        }
        if (event->getDirty()) {
            if (state == E) {
                cacheLine->setState(M);
                state = M;
            }
            cacheLine->setData(event->getPayload(), 0);
        }
    
        if (cacheLine->getPrefetch()) {
            statPrefetchEvict->addData(1);
            cacheLine->setPrefetch(false);
        }
    }
            
    CacheAction reqEventAction;
    uint64_t sendTime = 0;
    // Handle flush at local level
    switch (state) {
        case I:
        case I_B:
            if (reqEvent != NULL) return STALL;
            break;
        case S:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(S_Inv);
                return STALL;
            }
            break;
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
        case IM:
        case IS:
        case SM:
        case S_B:
            return STALL; // Wait for reqEvent to finish
        case S_Inv:
            // reqEvent is Inv, FlushLineInv, FetchInv
            reqEventAction = (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) ? DONE : STALL;
            if (reqEventAction == DONE) {
                if (reqEvent->getCmd() == Command::Inv) sendAckInv(reqEvent);
                else if (reqEvent->getCmd() == Command::FetchInv) sendResponseDown(reqEvent, cacheLine, false, true);
                else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(reqEvent->getBaseAddr(), reqEvent->getRqstr(), cacheLine, Command::FlushLineInv);
                    cacheLine->setState(I_B);
                    return STALL;
                }
                cacheLine->setState(I);
            }
            return reqEventAction;
        case SM_Inv:
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::Inv) {
                    if (cacheLine->numSharers() > 0) {  // May not have invalidated GetX requestor -> cannot also be the FlushLine requestor since that one is in I and blocked on flush
                        invalidateAllSharers(cacheLine, reqEvent->getRqstr(), true);
                        return STALL;
                    } else {
                        sendAckInv(reqEvent);
                        cacheLine->setState(IM);
                        return DONE;
                    }
                } else if (reqEvent->getCmd() == Command::GetX) {
                    cacheLine->setState(SM);
                    return STALL; // Waiting for GetXResp
                }
                debug->fatal(CALL_INFO, -1, "%s, Error: Received event in state SM_Inv but case does not match an implemented handler. Event = %s, OrigEvent = %s. Time = %" PRIu64 "ns\n",
                        ownerName_.c_str(), event->getVerboseString().c_str(), reqEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
            }
            return STALL;
        case EI:
        case MI:
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (state == MI || event->getDirty()) sendWriteback(Command::PutM, cacheLine, true, ownerName_);
    /* Event/State combinations - Count how many times an event was seen in particular state */
                else sendWriteback(Command::PutE, cacheLine, false, ownerName_);
                if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
                cacheLine->setState(I);
                return DONE;
            } else return STALL;
        case SI:
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                sendWriteback(Command::PutS, cacheLine, false, ownerName_);
                if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
                cacheLine->setState(I);
                return DONE;
            } else return STALL;
        case M_Inv:
            // reqEvent is GetX, FlushLineInv, or FetchInv
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, cacheLine, true, true);
                    cacheLine->setState(I);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::GetX || reqEvent->getCmd() == Command::GetSX) {
                    cacheLine->setOwner(reqEvent->getSrc());
                    if (cacheLine->isSharer(reqEvent->getSrc())) cacheLine->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, cacheLine->getData(), true, cacheLine->getTimestamp());
                    cacheLine->setTimestamp(sendTime);
                    cacheLine->setState(M);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(event->getBaseAddr(), event->getRqstr(), cacheLine, Command::FlushLineInv);
                    cacheLine->setState(I_B);
                    return STALL;
                }
            } else return STALL;
        case E_Inv:
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, cacheLine, event->getDirty(), true);
                    cacheLine->setState(I);
                    return DONE;
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    forwardFlushLine(reqEvent->getBaseAddr(), reqEvent->getRqstr(), cacheLine, Command::FlushLineInv);
                    cacheLine->setState(I_B);
                    return STALL;
                }
            } else return STALL;
        case M_InvX:
        case E_InvX:
            /* reqEvent is FetchInvX, FlushLine, or GetS */
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                if (reqEvent->getCmd() == Command::FetchInvX) {
                    sendResponseDown(reqEvent, cacheLine, (state == E_InvX || event->getDirty()), true); // calc latency based on where data comes from
                    cacheLine->setState(S);
                } else if (reqEvent->getCmd() == Command::FlushLine) {
                    if (event->getDirty() || state == M_InvX) cacheLine->setState(M);
                    else cacheLine->setState(E);
                    return handleFlushLineRequest(reqEvent, cacheLine, NULL, true);
                } else { // cmd = GetS; need to forward dirty/M so we don't lose that info
                    cacheLine->setOwner(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, Command::GetXResp, cacheLine->getData(), (state == M_InvX || event->getDirty()), true, cacheLine->getTimestamp());
                    cacheLine->setTimestamp(sendTime);
                    (state == M_InvX || event->getDirty()) ? cacheLine->setState(M) : cacheLine->setState(E);
                } 
                return DONE;
            } else return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    forwardFlushLine(event->getBaseAddr(), event->getRqstr(), cacheLine, Command::FlushLineInv);
    if (cacheLine) cacheLine->setState(I_B);
    return STALL;   // wait for response
}


/**
 * Handle PutS requests. 
 * Special cases for non-inclusive caches with racing events where we don't wait for a new line to be allocated if deadlock might occur
 */
CacheAction MESIPrivNoninclusive::handlePutSRequest(MemEvent* event, CacheLine* line, MemEvent * reqEvent) {
    State state = (line != NULL) ? line->getState() : I;
    recordStateEventCount(event->getCmd(), state);
    
    // Handle special cases for non-inclusive caches
    if (line == NULL) { // Means we couldn't allocate a cache line immediately but we have a reqEvent that we need to resolve
        if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
        if (reqEvent->getCmd() == Command::Fetch) {
            sendResponseDownFromMSHR(event, reqEvent, false);
            return STALL; // Replay PutS since we haven't officially dropped it
        } else if (reqEvent->getCmd() == Command::Inv) {
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                sendAckInv(reqEvent); 
                return DONE;    // Drop both requests
            } else return IGNORE; // Drop PutS only
        } else if (reqEvent->getCmd() == Command::FetchInv) {
            if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
                sendResponseDownFromMSHR(event, reqEvent, false);
                return DONE; // Drop both requests
            }
        } else {
            debug->fatal(CALL_INFO, -1, "%s, Error: Received PutS for an unallocated line but reqEvent cmd is unhandled. Event = %s. ReqEvent = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), event->getVerboseString().c_str(), reqEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
    }
    line->setData(event->getPayload(), 0);
        
    if (is_debug_event(event)) printData(line->getData(), true);
        
    if (state == I) {
        if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
            line->setState(S); // newly allocated line
        } else {
            if (reqEvent->getCmd() == Command::Fetch) {
                line->setState(S_D);
            } else line->setState(S_Inv);   // cmd is FetchInv or Inv
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
        case S_B:
            sendWritebackAck(event);
            return DONE;
        /* Races with evictions */
        case SI:
            sendWriteback(Command::PutS, line, false, ownerName_);
            if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case EI:
            sendWriteback(Command::PutE, line, false, ownerName_);
            if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        case MI:
            sendWriteback(Command::PutM, line, true, ownerName_);
            if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
            line->setState(I);
            return DONE;
        /* Race with a fetch */
        case S_D:
            sendResponseDown(reqEvent, line, false, true);
            line->setState(S);
            return DONE;
        /* Races with Invs and FetchInvs from outside our sub-hierarchy; races with GetX from within our sub-hierarchy*/
        case S_Inv:
            if (reqEvent->getCmd() == Command::Inv) {
                sendAckInv(reqEvent);
                line->setState(I);
            } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                line->setState(S);
                if (handleFlushLineInvRequest(reqEvent, line, NULL, true) == DONE) return DONE;
                else return IGNORE;
            } else if (reqEvent->getCmd() == Command::FlushLine) {
                line->setState(S);
                if (handleFlushLineRequest(reqEvent, line, NULL, true) == DONE) return DONE;
                else return IGNORE;
            } else {
                sendResponseDown(reqEvent, line, false, true);
                line->setState(I);
            }
            return DONE;
        case SB_Inv:
            sendAckInv(reqEvent);
            line->setState(I_B);
            return DONE;
        case E_Inv:
            if (reqEvent->getCmd() == Command::FlushLineInv) {
                line->setState(E);
                if (handleFlushLineInvRequest(reqEvent, line, NULL, true) == DONE) return DONE;
                else return IGNORE;
            } else {
                sendResponseDown(reqEvent, line, false, true);
                line->setState(I);
                return DONE;
            }
        case M_Inv:
            if (reqEvent->getCmd() == Command::FetchInv) {
                sendResponseDown(reqEvent, line, true, true);
                line->setState(I);
            } else if (reqEvent->getCmd() == Command::GetX || reqEvent->getCmd() == Command::GetSX) {
                line->setOwner(reqEvent->getSrc());
                if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, line->getData(), true, line->getTimestamp());
                line->setTimestamp(sendTime);
                line->setState(M);
                
                if (is_debug_event(reqEvent)) printData(line->getData(), false);
            } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                line->setState(M);
                // Returns: Flush   PutS    Val
                //          DONE    IGNORE  DONE
                //          STALL   IGNORE  IGNORE
                if (handleFlushLineInvRequest(reqEvent, line, NULL, true) == DONE) return DONE;
                else return IGNORE;
            }
            return DONE;
        case SM_Inv:
            if (reqEvent->getCmd() == Command::Inv) {
                if (line->numSharers() > 0) { // Possible that GetX requestor was never invalidated
                    invalidateAllSharers(line, reqEvent->getRqstr(), true);
                    return IGNORE;
                } else {
                    sendAckInv(reqEvent);
                    line->setState(IM);
                    return DONE;
                }
            } else {
                line->setState(SM);
                return IGNORE; // Waiting for GetXResp
            }
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return IGNORE;   // eliminate compiler warning
}


/**
 *  Handle PutM and PutE requests (i.e., dirty and clean non-inclusive replacements respectively)
 */
CacheAction MESIPrivNoninclusive::handlePutMRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent) {
    State state = (cacheLine == NULL) ? I : cacheLine->getState();

    recordStateEventCount(event->getCmd(), state);
    
    if (cacheLine == NULL) {    // Didn't wait for allocation because a reqEvent is dependent on this replacement
        if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
        if (reqEvent->getCmd() == Command::FetchInvX) {
            // Handle FetchInvX and keep stalling Put*
            sendResponseDownFromMSHR(event, reqEvent, event->getDirty());
            return STALL;
        } else if (reqEvent->getCmd() == Command::FetchInv) {  
            // HandleFetchInv and retire Put*
            sendResponseDownFromMSHR(event, reqEvent, event->getDirty());
            return IGNORE;
        } else {
            debug->fatal(CALL_INFO, -1, "%s, Error: Received event for an unallocated line but conflicting event's command is unhandled. Event = %s. Conflicting event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), event->getVerboseString().c_str(), reqEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
    } else if (cacheLine->getState() == I) {
        if (mshr_->getAcksNeeded(event->getBaseAddr()) == 0) {
            (event->getCmd() == Command::PutM) ? cacheLine->setState(M) : cacheLine->setState(E);
        } else {
            if (reqEvent->getCmd() == Command::FetchInvX) {
                (event->getCmd() == Command::PutM) ? cacheLine->setState(M_InvX) : cacheLine->setState(E_InvX);
            } else {
                (event->getCmd() == Command::PutM) ? cacheLine->setState(M_Inv) : cacheLine->setState(E_Inv);
            }
        }
        state = cacheLine->getState();
    } 
        
    cacheLine->setData(event->getPayload(), 0);
    cacheLine->clearOwner();
            
    if (mshr_->getAcksNeeded(event->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(event->getBaseAddr());
    
    uint64_t sendTime = 0;
    
    switch (state) {
        case E:
            if (event->getDirty()) cacheLine->setState(M);
        case M:
            sendWritebackAck(event);
            break;
        /* Races with evictions */
        case EI:
            if (event->getCmd() == Command::PutM) {
                sendWriteback(Command::PutM, cacheLine, true, ownerName_);
            } else {
                sendWriteback(Command::PutE, cacheLine, false, ownerName_);
            }
	    cacheLine->setState(I);    // wait for ack
            if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            sendWriteback(Command::PutM, cacheLine, true, ownerName_);
	    cacheLine->setState(I);    // wait for ack
            if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        /* Races with FetchInv from outside our sub-hierarchy; races with GetX from within our sub-hierarchy */
        case E_Inv:
            if (reqEvent->getCmd() == Command::FlushLineInv) {
                event->getDirty() ? cacheLine->setState(M) : cacheLine->setState(E);
                if (handleFlushLineInvRequest(reqEvent, cacheLine, NULL, true) == DONE) return DONE;
                else return IGNORE;
            } else {
                sendResponseDown(reqEvent, cacheLine, event->getDirty(), true);
                cacheLine->setState(I);
            }
            break;
        case M_Inv:
            if (reqEvent->getCmd() == Command::FetchInv || reqEvent->getCmd() == Command::ForceInv) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                cacheLine->setState(M);
                if (handleFlushLineInvRequest(reqEvent, cacheLine, NULL, true) == DONE) return DONE;
                else return IGNORE;
            } else {
                cacheLine->setState(M);
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                cacheLine->setOwner(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
            
                if (is_debug_event(reqEvent)) printData(cacheLine->getData(), false);
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
            if (reqEvent->getCmd() == Command::FetchInvX) {
                sendResponseDown(reqEvent, cacheLine, (state == M_InvX || event->getDirty()), true);
                cacheLine->setState(S);
            } else { // Race with GetS
                sendTime = sendResponseUp(reqEvent, Command::GetXResp, cacheLine->getData(), (cacheLine->getState() == M), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                cacheLine->setOwner(reqEvent->getSrc());
            }
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
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
CacheAction MESIPrivNoninclusive::handleInv(MemEvent* event, CacheLine* cacheLine, bool replay) {
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
            return IGNORE;    // Eviction raced with Inv, IS/IM only happen if we don't use AckPuts
        case S_B:
        case S:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                if (state == S) cacheLine->setState(S_Inv);
                else cacheLine->setState(SB_Inv);
                return STALL;
            }
            sendAckInv(event);
            if (state == S) cacheLine->setState(I);
            else cacheLine->setState(I_B);
            return DONE;
        case SM:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SM_Inv);
                return STALL;
            }
            sendAckInv(event);
            cacheLine->setState(IM);
            return DONE;
        case S_Inv: // PutS or Flush in progress, stall this Inv for that
            return BLOCK;
        case SI: // PutS in progress, turn it into an Inv in progress
            cacheLine->setState(S_Inv);
        case SM_Inv:    // Waiting on GetSResp & AckInvs, stall this Inv until invacks come back
            return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL;
}


/** 
 * Handle ForceInv requests
 * Invalidate block regardless of whether it is dirty or not and send an ack
 * Do not forward data with ack
 */
CacheAction MESIPrivNoninclusive::handleForceInv(MemEvent * event, CacheLine * cacheLine, bool replay) {
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
            return IGNORE; // Raced with Put so let Put be our AckInv
        case S_B:
        case S:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                if (state == S) cacheLine->setState(S_Inv);
                else cacheLine->setState(SB_Inv);
                return STALL;
            }
            sendAckInv(event);
            if (state == S) cacheLine->setState(I);
            else cacheLine->setState(I_B);
            return DONE;
        case SM:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SM_Inv);
                return STALL;
            }
            sendAckInv(event);
            cacheLine->setState(IM);
            return DONE;
        case S_Inv:
        case SB_Inv:
            return BLOCK;
        case SI:
            cacheLine->setState(S_Inv);
        case SM_Inv:
            return STALL;
        case E:
        case M:
            if (cacheLine->ownerExists()) {
                sendForceInv(cacheLine, event->getRqstr(), replay);
                mshr_->incrementAcksNeeded(event->getBaseAddr());
                cacheLine->setState(M_Inv);
                return STALL;
            }
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(M_Inv);
                return STALL;
            }
            sendAckInv(event);
            cacheLine->setState(I);
            return DONE;
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
           // if (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv) return STALL;    // Handle the incoming Inv first
            return BLOCK;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return STALL;
}


/**
 *  Handle FetchInv requests
 *  FetchInvs are a request to get data (Fetch) and invalidate the local copy.
 *  Generally sent to caches with exclusive blocks but a non-inclusive cache may send to a sharer instead of an Inv 
 *  if the non-inclusive cache does not have a cached copy of the block
 */
CacheAction MESIPrivNoninclusive::handleFetchInv(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay) {
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
        case S: // Happens when there is a non-inclusive cache below us
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(S_Inv);
                return STALL;
            }
            break;
        case S_B:
            if (cacheLine->numSharers() > 0) {
                invalidateAllSharers(cacheLine, event->getRqstr(), replay);
                cacheLine->setState(SB_Inv);
                return STALL;
            }
            sendAckInv(event);
            cacheLine->setState(I_B);
            return DONE;
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
        case SI:
            cacheLine->setState(S_Inv);
            return STALL;
        case EI:
            cacheLine->setState(E_Inv);
            return STALL;
        case MI:
            cacheLine->setState(M_Inv);
            return STALL;
        case S_Inv:
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            if (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv) return STALL;    // Handle the incoming Inv first
            return BLOCK;
        case SB_Inv:
            return BLOCK;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    sendResponseDown(event, cacheLine, (state == M), replay);
    cacheLine->setState(I);
    return DONE;
}


/**
 *  Handle FetchInvX.
 *  A FetchInvX is a request to downgrade the block to Shared state (from E or M) and to include a copy of the block
 */
CacheAction MESIPrivNoninclusive::handleFetchInvX(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay) {
    State state = cacheLine->getState();
    recordStateEventCount(event->getCmd(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
        case I_B:
        case S_B:
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
            if (collisionEvent && (collisionEvent->getCmd() == Command::FlushLine || collisionEvent->getCmd() == Command::FlushLineInv)) return STALL;    // Handle the incoming Inv first
            return BLOCK;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
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
CacheAction MESIPrivNoninclusive::handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay) {
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
            sendResponseDown(event, cacheLine, false, replay);
            return DONE;
        case S_Inv:
        case SI:
            return BLOCK;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/**
 *  Handle data responses (GetSResp and GetXResp)
 *  Send message up if request was from an upper cache
 *  Simply cache data locally if request was a local prefetch
 */
CacheAction MESIPrivNoninclusive::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    // Update memFlags so it's copied through on the way up
    origRequest->setMemFlags(responseEvent->getMemFlags());

    if (cacheLine == NULL || cacheLine->getState() == I) {
        uint64_t sendTime = sendResponseUp(origRequest, responseEvent->getCmd(), &responseEvent->getPayload(), responseEvent->getDirty(), true, 0);
        if (cacheLine != NULL) cacheLine->setTimestamp(sendTime);
        return DONE;
    }

    State state = cacheLine->getState();
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool localPrefetch = origRequest->isPrefetch() && (origRequest->getRqstr() == ownerName_);
    
    uint64_t sendTime = 0;
    
    switch (state) {
        case IS:
            cacheLine->setData(responseEvent->getPayload(), 0);
            
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), true);
            
            if (responseEvent->getCmd() == Command::GetXResp && responseEvent->getDirty()) cacheLine->setState(M);
            else if (protocol_ && responseEvent->getCmd() == Command::GetXResp) cacheLine->setState(E);
            else cacheLine->setState(S);
            if (localPrefetch) {
                cacheLine->setPrefetch(true);
                recordPrefetchLatency(origRequest->getID(), LatType::MISS);
                return DONE;     
            }
            
            if (cacheLine->getState() != S) { // Transfer E/M permission
                cacheLine->setOwner(origRequest->getSrc());
                sendTime = sendResponseUp(origRequest, Command::GetXResp, &responseEvent->getPayload(), state == M, true, cacheLine->getTimestamp());
            } else if (protocol_ && cacheLine->getState() != S && mshr_->lookup(responseEvent->getBaseAddr()).size() == 1) { // Send exclusive response unless another request is waiting
                cacheLine->setOwner(origRequest->getSrc());
                sendTime = sendResponseUp(origRequest, Command::GetXResp, &responseEvent->getPayload(), true, cacheLine->getTimestamp());
            } else { // Default shared response
                cacheLine->addSharer(origRequest->getSrc());
                sendTime = sendResponseUp(origRequest, &responseEvent->getPayload(), true, cacheLine->getTimestamp());
            }

            cacheLine->setTimestamp(sendTime);
            
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), false);
            
            return DONE;
        case IM:
            cacheLine->setData(responseEvent->getPayload(), 0);
            
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), true);
        case SM:
            cacheLine->setState(M);
            cacheLine->setOwner(origRequest->getSrc());
            if (cacheLine->isSharer(origRequest->getSrc())) cacheLine->removeSharer(origRequest->getSrc());
            sendTime = sendResponseUp(origRequest, cacheLine->getData(), true, cacheLine->getTimestamp());
            cacheLine->setTimestamp(sendTime);
            
            if (is_debug_event(responseEvent)) printData(cacheLine->getData(), false);
            
            return DONE;
        case SM_Inv:
            cacheLine->setState(M_Inv);
            return STALL;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], responseEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/**
 *  Handle Fetch responses (responses to Fetch, FetchInv, and FetchInvX)
 */
CacheAction MESIPrivNoninclusive::handleFetchResp(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent) {
    State state = (cacheLine == NULL) ? I : cacheLine->getState();
    
    // Check acks needed
    if (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(responseEvent->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(responseEvent->getBaseAddr()) == 0) ? DONE : IGNORE;

    // Update data
    if (state != I) cacheLine->setData(responseEvent->getPayload(), 0);
    
    if (state != I && (is_debug_event(responseEvent))) printData(cacheLine->getData(), true);

    recordStateEventCount(responseEvent->getCmd(), state);
    
    uint64_t sendTime = 0;

    switch (state) {
        case I:
            if (action != DONE) {
                debug->fatal(CALL_INFO, -1, "%s, Error: Non-inclusive cache received a FetchResp for a non-cached address and is waiting for more acks. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), responseEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
            }
            sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
            break;
        case EI:
            sendWriteback(responseEvent->getDirty() ? Command::PutM : Command::PutE, cacheLine, responseEvent->getDirty(), ownerName_);
	    cacheLine->setState(I);    // wait for ack
            if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case MI:
            sendWriteback(Command::PutM, cacheLine, true, ownerName_);
	    cacheLine->setState(I);    // wait for ack
            if (expectWritebackAck_) mshr_->insertWriteback(cacheLine->getBaseAddr());
            break;
        case E_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == Command::FetchInvX) {
                sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
                cacheLine->setState(S);
            } else if (reqEvent->getCmd() == Command::FlushLine) {
                if (responseEvent->getDirty()) {
                    cacheLine->setState(M);
                    cacheLine->setData(responseEvent->getPayload(), 0);
                } else {
                    cacheLine->setState(E);
                }
                action = handleFlushLineRequest(reqEvent, cacheLine, NULL, true);
                break;
            } else if (reqEvent->getCmd() == Command::FetchInv) {    // Raced with FlushLine
                if (responseEvent->getDirty()) {
                    cacheLine->setData(responseEvent->getPayload(), 0);
                }
                if (cacheLine->numSharers() > 0) {
                    invalidateAllSharers(cacheLine, reqEvent->getRqstr(), true);
                    responseEvent->getDirty() ? cacheLine->setState(M_Inv) : cacheLine->setState(E_Inv);
                    return STALL;
                }
                responseEvent->getDirty() ? cacheLine->setState(M) : cacheLine->setState(E);
                sendResponseDownFromMSHR(responseEvent, reqEvent, responseEvent->getDirty());
                break;
            } else {
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                cacheLine->addSharer(reqEvent->getSrc());
                if (responseEvent->getDirty()) {
                    cacheLine->setState(M);
                    cacheLine->setData(responseEvent->getPayload(), 0);
                } else cacheLine->setState(E);
                sendTime = sendResponseUp(reqEvent, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
            }
            break;
        case E_Inv:
            if (reqEvent->getCmd() == Command::FlushLineInv) {
                
                if (cacheLine->isSharer(responseEvent->getSrc())) cacheLine->removeSharer(responseEvent->getSrc());
                if (cacheLine->getOwner() == responseEvent->getSrc()) cacheLine->clearOwner();
                if (responseEvent->getDirty()) {
                    cacheLine->setState(M);
                    cacheLine->setData(responseEvent->getPayload(), 0);
                } else cacheLine->setState(E);
                if (action != DONE) { // Sanity check...
                    debug->fatal(CALL_INFO, -1, "%s, Error: Received a FetchResp to a FlushLineInv but still waiting on more acks. Event = %s. Time = %" PRIu64 "ns\n",
                        ownerName_.c_str(), responseEvent->getVerboseString().c_str(), getCurrentSimTimeNano()); 
                }
                action = handleFlushLineInvRequest(reqEvent, cacheLine, NULL, true);
                break;
            }
            if (cacheLine->isSharer(responseEvent->getSrc())) cacheLine->removeSharer(responseEvent->getSrc());
            if (cacheLine->getOwner() == responseEvent->getSrc()) cacheLine->clearOwner();
            sendResponseDown(reqEvent, cacheLine, responseEvent->getDirty(), true);
            cacheLine->setState(I);
            break;
        case M_InvX:
            cacheLine->clearOwner();
            cacheLine->addSharer(responseEvent->getSrc());
            if (reqEvent->getCmd() == Command::FetchInvX) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(S);
            } else if (reqEvent->getCmd() == Command::FlushLine) {
                if (responseEvent->getDirty()) {
                    cacheLine->setState(M);
                    cacheLine->setData(responseEvent->getPayload(), 0);
                } else {
                    cacheLine->setState(E);
                }
                action = handleFlushLineRequest(reqEvent, cacheLine, NULL, true);
                break;
            } else if (reqEvent->getCmd() == Command::FetchInv) {
                if (responseEvent->getDirty()) {
                    cacheLine->setData(responseEvent->getPayload(), 0);
                }
                if (cacheLine->numSharers() > 0) {
                    invalidateAllSharers(cacheLine, reqEvent->getRqstr(), true);
                    cacheLine->setState(M_Inv);
                    return STALL;
                }
                cacheLine->setState(M);
                sendResponseDownFromMSHR(responseEvent, reqEvent, true);
                break;
            } else {    // reqEvent->getCmd() == GetS
                notifyListenerOfAccess(reqEvent, NotifyAccessType::READ, NotifyResultType::HIT);
                cacheLine->addSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                cacheLine->setState(M);
            }
            break;
        case M_Inv:
            cacheLine->clearOwner();
            if (reqEvent->getCmd() == Command::FlushLineInv) {
                if (cacheLine->getOwner() == responseEvent->getSrc()) cacheLine->clearOwner();
                if (responseEvent->getDirty()) cacheLine->setData(responseEvent->getPayload(), 0);
                cacheLine->setState(M);
                if (action != DONE) { // Sanity check...
                    debug->fatal(CALL_INFO, -1, "%s, Error: Received a FetchResp to a FlushLineInv but still waiting on more acks. Event = %s. Time = %" PRIu64 "ns\n",
                        ownerName_.c_str(), responseEvent->getVerboseString().c_str(), getCurrentSimTimeNano()); 
                }
                action = handleFlushLineInvRequest(reqEvent, cacheLine, NULL, true);
                break;
            } else if (reqEvent->getCmd() == Command::FetchInv) {
                sendResponseDown(reqEvent, cacheLine, true, true);
                cacheLine->setState(I);
            } else {    // reqEvent->getCmd() == GetX
                notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                cacheLine->setOwner(reqEvent->getSrc());
                if (cacheLine->isSharer(reqEvent->getSrc())) cacheLine->removeSharer(reqEvent->getSrc());
                sendTime = sendResponseUp(reqEvent, cacheLine->getData(), true, cacheLine->getTimestamp());
                cacheLine->setTimestamp(sendTime);
                
                if (is_debug_event(reqEvent)) printData(cacheLine->getData(), false);
                
                cacheLine->setState(M);
            }
            break;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], responseEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return action;
}


/**
 *  Handle AckInvs (response to Inv requests)
 */
CacheAction MESIPrivNoninclusive::handleAckInv(MemEvent * ack, CacheLine * line, MemEvent * reqEvent) {
    State state = (line == NULL) ? I : line->getState();
    
    recordStateEventCount(ack->getCmd(), state);

    if (line && line->isSharer(ack->getSrc())) {
        line->removeSharer(ack->getSrc());
    }
    if (line && line->getOwner() == ack->getSrc()) {
        line->clearOwner();
    }
    
    if (is_debug_event(ack)) debug->debug(_L6_, "Received AckInv for 0x%" PRIx64 ", acks needed: %d\n", ack->getBaseAddr(), mshr_->getAcksNeeded(ack->getBaseAddr()));
    
    if (mshr_->getAcksNeeded(ack->getBaseAddr()) > 0) mshr_->decrementAcksNeeded(ack->getBaseAddr());
    CacheAction action = (mshr_->getAcksNeeded(ack->getBaseAddr()) == 0) ? DONE : IGNORE;
    
    uint64_t sendTime = 0;
    
    switch (state) {
        case I:
            if (action == DONE) {
                sendAckInv(reqEvent);
            }
            return action;
        case S_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(I);
                } else if (reqEvent->getCmd() == Command::Inv || reqEvent->getCmd() == Command::ForceInv) {
                    sendAckInv(reqEvent);
                    line->setState(I);
                } else if (reqEvent->getCmd() == Command::FlushLineInv) {
                    line->setState(S);
                    action = handleFlushLineInvRequest(reqEvent, line, NULL, true);
                } else if (reqEvent->getCmd() == Command::FlushLine) {
                    line->setState(S);
                    action = handleFlushLineRequest(reqEvent, line, NULL, true);
                }
            }
            return action;
        case SM_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::Inv || reqEvent->getCmd() == Command::ForceInv) {
                    if (line->numSharers() > 0) { // May not have invalidated GetX requestor
                        invalidateAllSharers(line, reqEvent->getRqstr(), true);
                        return IGNORE;
                    } else {
                        sendAckInv(reqEvent);
                        line->setState(IM);
                    }
                } else if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(IM);
                } else {
                    line->setState(SM);
                    return IGNORE;
                }
            }
            return action;
        case SB_Inv:
            if (action == DONE) {
                if (line->numSharers() > 0) {
                    invalidateAllSharers(line, reqEvent->getRqstr(), true);
                    return IGNORE;
                }
                sendAckInv(ack);
                line->setState(I_B);
            }
            return action;
        case SI:
            if (action == DONE) {
                sendWriteback(Command::PutS, line, false, ownerName_);
                if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case EI:
            if (action == DONE) {
                sendWriteback(Command::PutE, line, false, ownerName_);
                if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case MI:
            if (action == DONE) {
                sendWriteback(Command::PutM, line, true, ownerName_);
                if (expectWritebackAck_) mshr_->insertWriteback(line->getBaseAddr());
                line->setState(I);
            }
            return action;
        case E_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FlushLineInv) {
                    line->setState(E);
                    action = handleFlushLineInvRequest(reqEvent, line, NULL, true);
                } else if (reqEvent->getCmd() == Command::ForceInv) {
                    sendAckInv(reqEvent);
                    line->setState(I);  
                } else {
                    sendResponseDown(reqEvent, line, false, true);
                    line->setState(I);
                }
            }
            return action;
        case M_Inv:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FlushLineInv) {
                    line->setState(M);
                    action = handleFlushLineInvRequest(reqEvent, line, NULL, true);
                } else if (reqEvent->getCmd() == Command::FetchInv) {
                    sendResponseDown(reqEvent, line, true, true);
                    line->setState(I);
                } else if (reqEvent->getCmd() == Command::ForceInv) {
                    sendAckInv(reqEvent);
                    line->setState(I);  
                } else { // reqEvent->getCmd() == GetX/GetSX
                    notifyListenerOfAccess(reqEvent, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    line->setOwner(reqEvent->getSrc());
                    if (line->isSharer(reqEvent->getSrc())) line->removeSharer(reqEvent->getSrc());
                    sendTime = sendResponseUp(reqEvent, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime);
                    
                    if (is_debug_event(reqEvent)) printData(line->getData(), false);
                    
                    line->setState(M);
                }
            }
            return action;
        case E_InvX:
        case M_InvX:
            if (action == DONE) {
                if (reqEvent->getCmd() == Command::FlushLine) {
                    state == E_InvX ? line->setState(E) : line->setState(M);
                    action = handleFlushLineRequest(reqEvent, line, NULL, true);
                } else {
                   debug->fatal(CALL_INFO, -1, "%s, Error: Received AckInv in M_InvX, but reqEvent is unhandled. Event = %s. reqEvent = %s. Time = %" PRIu64 "ns\n",
                           ownerName_.c_str(), ack->getVerboseString().c_str(), reqEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
            }
            return action;
        default:
            debug->fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), StateString[state], ack->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return action;    // eliminate compiler warning
}


bool MESIPrivNoninclusive::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent* nackedEvent = event->getNACKedEvent();

    CacheLine* line = cacheArray_->lookup(nackedEvent->getBaseAddr(), false);
    Command cmd = nackedEvent->getCmd();
    State state = line ? line->getState() : I;

    bool resend = false;
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
            if (expectWritebackAck_ && !mshr_->pendingWriteback(line->getBaseAddr())) 
                resend = false;
            else
                resend = true;
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
            if (state == I) 
                resend = false;   // Already resolved the request, don't resend
            else if (line->getOwner() != nackedEvent->getDst()) {
                if (line->isSharer(nackedEvent->getDst()) && cmd == Command::FetchInv) { // Got a downgrade from the owner but still need to invalidate
                    uint64_t deliveryTime = 0;
                    MemEvent * inv = new MemEvent(ownerName_, line->getBaseAddr(), line->getBaseAddr(), Command::Inv);
                    inv->setDst(nackedEvent->getDst());
                    inv->setRqstr(nackedEvent->getRqstr());
                    inv->setSize(line->getSize());
                    deliveryTime = timestamp_  + mshrLatency_;
                    Response resp = {inv, deliveryTime, packetHeaderBytes};
                    addToOutgoingQueueUp(resp);
        
                    if (is_debug_addr(line->getBaseAddr())) {
                        debug->debug(_L7_,"Re-sending FetchInv as Inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                                line->getBaseAddr(), nackedEvent->getDst().c_str(), deliveryTime);
                    }
                }
                resend = false;    // Must have gotten a replacement/downgrade from this owner
            } else
                resend = true;
            break;
        case Command::Inv:
            if (state == I || !line->isSharer(nackedEvent->getDst())) 
                resend = false;
            else
                resend = true;
            break;
        default:
            debug->fatal(CALL_INFO,-1,"%s, Error: NACKed event is unrecognized. Event = %s. Time = %" PRIu64 "ns\n",
                    ownerName_.c_str(), nackedEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (resend)
        resendEvent(nackedEvent, nackedEvent->fromHighNetNACK());
    else
        delete nackedEvent;

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for managing data structures
 *---------------------------------------------------------------------------------------------------------------------*/
bool MESIPrivNoninclusive::allocateLine(Addr addr, MemEvent* event) {
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
/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface 
 *---------------------------------------------------------------------------------------------------------------------*/

/**
 *  Send an Inv to all sharers of the block. Used for evictions or Inv/FetchInv requests from lower level caches
 */
void MESIPrivNoninclusive::invalidateAllSharers(CacheLine * cacheLine, string rqstr, bool replay) {
    set<std::string> * sharers = cacheLine->getSharers();
    uint64_t deliveryTime = 0;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        MemEvent * inv = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::Inv);
        inv->setDst(*it);
        inv->setRqstr(rqstr);
        inv->setSize(cacheLine->getSize());
    
        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);

        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());

        if (is_debug_addr(cacheLine->getBaseAddr())) {
            debug->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                    cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
        }
    }
    if (deliveryTime != 0) cacheLine->setTimestamp(deliveryTime);
}


/**
 *  Send an Inv to all sharers unless the cache requesting exclusive permission is a sharer; then send Inv to all sharers except requestor. 
 *  Used for GetX/GetSX requests.
 */
bool MESIPrivNoninclusive::invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay) {
    bool sentInv = false;
    set<std::string> * sharers = cacheLine->getSharers();
    uint64_t deliveryTime = 0;
    for (set<std::string>::iterator it = sharers->begin(); it != sharers->end(); it++) {
        if (*it == rqstr) continue;

        MemEvent * inv = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::Inv);
        inv->setDst(*it);
        inv->setRqstr(origRqstr);
        inv->setSize(cacheLine->getSize());

        uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
        deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
        Response resp = {inv, deliveryTime, packetHeaderBytes};
        addToOutgoingQueueUp(resp);
        sentInv = true;
        
        mshr_->incrementAcksNeeded(cacheLine->getBaseAddr());
        
        if (is_debug_addr(cacheLine->getBaseAddr())) {
            debug->debug(_L7_,"Sending inv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                    cacheLine->getBaseAddr(), (*it).c_str(), deliveryTime);
        }
    }
    if (deliveryTime != 0) cacheLine->setTimestamp(deliveryTime);
    return sentInv;
}


/**
 *  Send FetchInv to owner of a block
 */
void MESIPrivNoninclusive::sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInv);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());
    
    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
   
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending FetchInv: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


/** 
 *  Send FetchInv to owner of a block
 */
void MESIPrivNoninclusive::sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * fetch = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::FetchInvX);
    fetch->setDst(cacheLine->getOwner());
    fetch->setRqstr(rqstr);
    fetch->setSize(cacheLine->getSize());

    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = (replay) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    Response resp = {fetch, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) {
        debug->debug(_L7_, "Sending FetchInvX: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n", 
                cacheLine->getBaseAddr(), cacheLine->getOwner().c_str(), deliveryTime);
    }
}


/**
 *  Send ForceInv to block owner
 */
void MESIPrivNoninclusive::sendForceInv(CacheLine * cacheLine, string rqstr, bool replay) {
    MemEvent * inv = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), Command::ForceInv);
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


/**
 *  Forward message to an upper level cache
 *  when we don't know exact destination
 */
void MESIPrivNoninclusive::forwardMessageUp(MemEvent* event) {
    MemEvent * forwardEvent = new MemEvent(*event);
    forwardEvent->setSrc(ownerName_);
    forwardEvent->setDst(getSrc());
    
    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response fwdReq = {forwardEvent, deliveryTime, packetHeaderBytes + forwardEvent->getPayloadSize()};
    addToOutgoingQueueUp(fwdReq);
    
    if (is_debug_event(event)) {
        debug->debug(_L3_, "Forwarding %s to %s at cycle = %" PRIu64 "\n", CommandString[(int)forwardEvent->getCmd()], forwardEvent->getDst().c_str(), deliveryTime);
    }
}

/*
 *  Handles: responses to fetch invalidates
 *  Latency: cache access to read data for payload  
 */
void MESIPrivNoninclusive::sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool dirty, bool replay){
    MemEvent *responseEvent = event->makeResponse();
    responseEvent->setPayload(*cacheLine->getData());
    
    if (is_debug_event(event)) printData(cacheLine->getData(), false);
    
    responseEvent->setSize(cacheLine->getSize());
    
    responseEvent->setDirty(dirty);

    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = replay ? baseTime + mshrLatency_ : baseTime + accessLatency_;
    Response resp  = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize()};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_event(event)) { 
        debug->debug(_L3_,"Sending Response at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)responseEvent->getCmd()], responseEvent->getSrc().c_str());
    }
}

/**
 *  Send response down (e.g., a FetchResp or AckInv) using an incoming event instead of the locally cached cacheline
 */
void MESIPrivNoninclusive::sendResponseDownFromMSHR(MemEvent * respEvent, MemEvent * reqEvent, bool dirty) {
    MemEvent * newResponseEvent = reqEvent->makeResponse();
    newResponseEvent->setPayload(respEvent->getPayload());
    newResponseEvent->setSize(respEvent->getSize());
    newResponseEvent->setDirty(dirty);

    uint64_t deliveryTime = timestamp_ + mshrLatency_;
    Response resp = {newResponseEvent, deliveryTime, packetHeaderBytes + respEvent->getPayloadSize()};
    addToOutgoingQueue(resp);

    if (is_debug_event(respEvent)) {
        debug->debug(_L3_,"Sending Response from MSHR at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", deliveryTime, CommandString[(int)newResponseEvent->getCmd()], newResponseEvent->getSrc().c_str());
    }
}


/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIPrivNoninclusive::sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string rqstr) {
    MemEvent* newCommandEvent = new MemEvent(ownerName_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    newCommandEvent->setSize(cacheLine->getSize());
    bool hasData = false;
    if (dirty || writebackCleanBlocks_) {
        newCommandEvent->setPayload(*cacheLine->getData());
        
        if (is_debug_addr(cacheLine->getBaseAddr())) printData(cacheLine->getData(), false);
        
        hasData = true;
    }
    newCommandEvent->setRqstr(rqstr);
    newCommandEvent->setDirty(dirty);
    
    
    uint64_t baseTime = timestamp_ > cacheLine->getTimestamp() ? timestamp_ : cacheLine->getTimestamp();
    uint64_t deliveryTime = hasData ? baseTime + accessLatency_ : baseTime + tagLatency_;
    Response resp = {newCommandEvent, deliveryTime, packetHeaderBytes + newCommandEvent->getPayloadSize()};
    addToOutgoingQueue(resp);
    cacheLine->setTimestamp(deliveryTime);
    
    if (is_debug_addr(cacheLine->getBaseAddr())) 
        debug->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. Cache index = %d\n", deliveryTime, CommandString[(int)cmd], cacheLine->getIndex());
}

/**
 *  Send a writeback ack. Mostly used by non-inclusive caches.
 */
void MESIPrivNoninclusive::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = new MemEvent(ownerName_, event->getBaseAddr(), event->getBaseAddr(), Command::AckPut);
    ack->setDst(event->getSrc());
    ack->setRqstr(event->getSrc());
    ack->setSize(event->getSize());

    uint64_t deliveryTime = timestamp_ + tagLatency_;

    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueueUp(resp);
    
    if (is_debug_event(event)) debug->debug(_L3_, "Sending AckPut at cycle = %" PRIu64 "\n", deliveryTime);
}


/**
 *  Send an AckInv as a response to an Inv
 */
void MESIPrivNoninclusive::sendAckInv(MemEvent * inv) {
    MemEvent * ack = inv->makeResponse(); // Use make response to get IDs set right
    ack->setCmd(Command::AckInv); // Just in case it wasn't an Inv we're responding to
    ack->setDst(getDestination(inv->getBaseAddr()));
    ack->setSize(lineSize_); // Number of bytes invalidated

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {ack, deliveryTime, packetHeaderBytes};
    addToOutgoingQueue(resp);
    
    if (is_debug_event(inv)) debug->debug(_L3_,"Sending AckInv at cycle = %" PRIu64 "\n", deliveryTime);
}


/**
 *  Forward a flush line request, with or without data
 */
void MESIPrivNoninclusive::forwardFlushLine(Addr baseAddr, string origRqstr, CacheLine * cacheLine, Command cmd) {
    MemEvent * flush = new MemEvent(ownerName_, baseAddr, baseAddr, cmd);
    flush->setDst(getDestination(baseAddr));
    flush->setRqstr(origRqstr);
    flush->setSize(lineSize_);
    uint64_t latency = tagLatency_;
    // Always include block if we have it, simplifies race handling TODO relax if we can
    if (cacheLine) {
        if (cacheLine->getState() == M) flush->setDirty(true);
        flush->setPayload(*cacheLine->getData());
        latency = accessLatency_;
    }
    uint64_t baseTime = timestamp_;
    if (cacheLine && cacheLine->getTimestamp() > baseTime) baseTime = cacheLine->getTimestamp();
    uint64_t deliveryTime = baseTime + latency;
    Response resp = {flush, deliveryTime, packetHeaderBytes + flush->getPayloadSize()};
    addToOutgoingQueue(resp);
    if (cacheLine) cacheLine->setTimestamp(deliveryTime-1);
    
    if (is_debug_addr(baseAddr)) {
        debug->debug(_L3_,"Forwarding %s at cycle = %" PRIu64 ", Cmd = %s, Src = %s\n", CommandString[(int)cmd], deliveryTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str());
    }
}


/**
 *  Send a flush response up
 */
void MESIPrivNoninclusive::sendFlushResponse(MemEvent * requestEvent, bool success) {
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


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIPrivNoninclusive::addToOutgoingQueue(Response& resp) {
    CoherenceController::addToOutgoingQueue(resp);
    recordEventSentDown(resp.event->getCmd());
}

void MESIPrivNoninclusive::addToOutgoingQueueUp(Response& resp) {
    CoherenceController::addToOutgoingQueueUp(resp);
    recordEventSentUp(resp.event->getCmd());
}

/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/


/** Print value of data blocks for debugging */
void MESIPrivNoninclusive::printData(vector<uint8_t> * data, bool set) {
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
void MESIPrivNoninclusive::recordEvictionState(State state) {
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

void MESIPrivNoninclusive::recordStateEventCount(Command cmd, State state) {
    stat_eventState[(int)cmd][state]->addData(1);
}

void MESIPrivNoninclusive::recordEventSentDown(Command cmd) {
    stat_eventSent[(int)cmd]->addData(1);
}


void MESIPrivNoninclusive::recordEventSentUp(Command cmd) {
    stat_eventSent[(int)cmd]->addData(1);
}

void MESIPrivNoninclusive::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

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
            stat_latencyFlushLine->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latencyFlushLineInv->addData(latency);
            break;
        default:
            break;
    }
}

void MESIPrivNoninclusiveController::printLine(Addr addr) {
    if (!is_debug_addr(addr)) return;
    CacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : NP;
    unsigned int sharers = line ? line->numSharers() : 0;
    string owner = line ? line->getOwner() : "";
    debug->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\"\n", addr, StateSTring[state], sharers, owner.c_str());
}
