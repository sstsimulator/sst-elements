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

/*
 * File:   cache.cc
 * Author: Caesar De la Paz III
 */


#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <csignal>

#include "cacheController.h"
#include "memEvent.h"
#include "mshr.h"
#include "coherencemgr/coherenceController.h"
#include "hash.h"


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


/***
 *
 *  Cache Controller
 *      Handles all incoming messages and routes cacheable requests to coherence handlers, handles non-cacheable
 *      Manages MSHRS -> buffering requests, NACKing requests when full, retrying requests (coherence handlers inform when to retry)
 *      Manages WB buffer -> buffering and removing writebacks
 *      Determines eviction victims when needed and coordinates eviction
 *  Coherence controller
 *      Handles requests passed in by cache controller  -> return whether to stall or retire a request and whether to replay waiting requests
 *      Handles all outgoing messages
 *
 * ****/

/**
 *  Determine whether an access will be a cache hit or not
 *  Cache hit if:
 *      Line is present in the cache and
 *      Line is in the correct coherence state for the request (BottomCC state is correct) and
 *      Line is not currently being invalidated (TopCC state is correct)
 *  @return int indicating cache hit (0) or miss (1=cold miss, 2=block has incorrect permissions, 3=sharers/owner needs to be invalidated)
 */
int Cache::isCacheHit(MemEvent* event, Command cmd, Addr baseAddr) {
    CacheLine * line = cacheArray_->lookup(baseAddr, false);
    if (line == nullptr) return 1; // Cache miss, line not found
    return coherenceMgr_->isCoherenceMiss(event, line);
}

/**
 *  Handle a request from upper level caches
 *  This function is only called if there are no blocking requests already in the MSHRs
 *  1. Locate line, if present, in the cache array & update its replacement manager state
 *  2. If cache miss, allocate a new cache line, possibly evicting another line
 *  3. If neccessary, get block/coherence permission from lower level caches/memory
 *  4. If neccessary, invalidate upper level caches and/or request updated data
 *  5. Send response to requestor
 */
void Cache::processCacheRequest(MemEvent* event, Command cmd, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    bool updateLine = !replay && event->isDataRequest();   /* TODO: move replacement manager update to time when cache actually sends a response */
    CacheLine * line = cacheArray_->lookup(baseAddr, updateLine);; 
    
    bool miss = (line == nullptr);
    
    if (miss && (is_debug_addr(baseAddr))) d_->debug(_L3_, "-- Miss --\n");
    
    if (!miss && line->inTransition()) {
        processRequestInMSHR(baseAddr, event);
        return;
    }
    if (type_ == "inclusive" && miss && !allocateLine(event, baseAddr)) {
        processRequestInMSHR(baseAddr, event);
        return;
    } else if (type_ == "noninclusive_with_directory" && miss && !allocateLine(event, baseAddr)) {
        processRequestInMSHR(baseAddr, event);
        return;
    } else if (type_ == "noninclusive" && (miss || !line->valid())) {
        processRequestInMSHR(baseAddr, event);
        if (event->inProgress()) {
            if (is_debug_addr(baseAddr)) d_->debug(_L8_, "Attempted retry too early, continue stalling\n");
            return;
        }

        // Forward instead of allocating for non-inclusive caches
        vector<uint8_t> * data = &event->getPayload();
        coherenceMgr_->forwardMessage(event, baseAddr, event->getSize(), 0, data); // Event to forward, address, requested size, data (if any)
        event->setInProgress(true);
        return;
    }
    
    line = cacheArray_->lookup(baseAddr, false);

    // Special case -> allocate line for prefetches to non-inclusive caches
    bool localPrefetch = event->isPrefetch() && event->getRqstr() == getName();
    if (type_ == "noninclusive_with_directory" && localPrefetch && line->getDataLine() == NULL && line->getState() == I) {
        if (!allocateDirCacheLine(event, baseAddr, line, false)) {
            if (is_debug_addr(baseAddr)) d_->debug(_L3_, "-- Data Cache Miss --\n");
            processRequestInMSHR(baseAddr, event);
            return;
        }
    }

    // Handle hit
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    CacheAction action = coherenceMgr_->handleRequest(event, line, replay);
    
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    // Post-request processing and/or stall
    if (action != DONE) processRequestInMSHR(baseAddr, event);
    else postRequestProcessing(event, line, replay); // cacheline only required for L1s, cache+dir cannot be an L1
}


/* 
 *  Handles processing for all replacements - PutS, PutE, PutM, etc. 
 *  For non-inclusive/incoherent caches, may need to allocate a new line
 *  If Put conflicts with existing request, will still call handleReplacement to resolve any races
 */
void Cache::processCacheReplacement(MemEvent* event, Command cmd, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);

    CacheLine * line = nullptr;
    // May need to allocate for non-inclusive or incoherent caches
    if (type_ == "noninclusive" || protocol_ == CoherenceProtocol::NONE) {
        line = cacheArray_->lookup(baseAddr, true); // Update replacement metadata
        if (line == nullptr) { // miss
            if (is_debug_addr(baseAddr)) d_->debug(_L3_, "-- Cache Miss --\n");
            
            if (!allocateLine(event, baseAddr)) {
                if (mshr_->getAcksNeeded(baseAddr) == 0) {
                    processRequestInMSHR(baseAddr, event);
                    return;
                } else {
                    if (is_debug_addr(baseAddr)) d_->debug(_L3_, "Can't allocate immediately, but handling collision anyways\n");
                }
            }
        }       
    }
    
    line = cacheArray_->lookup(baseAddr, false); 
    if (type_ == "noninclusive_with_directory") {
        if (line->getDataLine() == NULL) {
            if (is_debug_addr(baseAddr)) d_->debug(_L3_, "-- Cache Miss --\n");
            
            // Avoid some deadlocks by not stalling Put* requests to lines that are in transition, attempt replacement but don't force
            if (!allocateDirCacheLine(event, baseAddr, line, line->inTransition()) && !line->inTransition()) {
                processRequestInMSHR(baseAddr, event);
                return;
            }
        }
    }

    // Attempt replacement, also handle any racing requests
    MemEvent * origRequest = NULL;
    if (mshr_->exists(baseAddr)) origRequest = mshr_->lookupFront(baseAddr);
    CacheAction action = coherenceMgr_->handleReplacement(event, line, origRequest, replay);
    
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    if (( action == DONE || action == STALL) && origRequest != NULL) {
        mshr_->removeFront(baseAddr);
        recordLatency(origRequest);
        delete origRequest;
    }
    if (action == STALL || action == BLOCK) {
        processRequestInMSHR(baseAddr, event);
        return;
    }
    postReplacementProcessing(event, action, replay);
}


void Cache::processCacheInvalidate(MemEvent* event, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    if (!mshr_->pendingWriteback(baseAddr) && mshr_->isFull()) {
        processInvRequestInMSHR(baseAddr, event, false); // trigger a NACK
        return;
    }
    
    MemEvent * collisionEvent = NULL;
    if (mshr_->exists(baseAddr)) collisionEvent = mshr_->lookupFront(baseAddr);
    CacheLine * line = cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleInvalidationRequest(event, line, collisionEvent, replay);
        
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    if (action == STALL) {
        processInvRequestInMSHR(baseAddr, event, false);  // This inv is currently being handled, insert in front of mshr
    } else if (action == BLOCK) {
        processInvRequestInMSHR(baseAddr, event, true);   // This inv is blocked by a different access, insert just behind that access in mshr
    } else if (action == DONE) {
        delete event;
        activatePrevEvents(baseAddr);  // Inv acted as ackPut, retry any stalled requests
    } else {    // IGNORE
        delete event;
    }
}


void Cache::processCacheFlush(MemEvent* event, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    CacheLine * line = cacheArray_->lookup(baseAddr, false);
    bool miss = (line == nullptr);
    // Find line
    //      If hit and in transition: buffer in MSHR
    //      If hit and dirty: forward cacheFlush w/ data, wait for flushresp
    //      If hit and clean: forward cacheFlush w/o data, wait for flushresp
    //      If miss: forward cacheFlush w/o data, wait for flushresp
    
    if (event->inProgress()) {
        processRequestInMSHR(baseAddr, event);
       
        if (is_debug_addr(baseAddr)) d_->debug(_L8_, "Attempted retry too early, continue stalling\n");
        
        return;
    }

    

    MemEvent * origRequest = NULL;
    if (mshr_->exists(baseAddr)) origRequest = mshr_->lookupFront(baseAddr);
    
    // Generally we should not nack this request without checking for races
    // But if no possible races and handling this will fill MSHR, nack it
    if (!origRequest && mshr_->isAlmostFull()) { 
        sendNACK(event);
        return;
    }

    CacheAction action = coherenceMgr_->handleReplacement(event, line, origRequest, replay);
    
    /* Action returned is for the origRequest if it exists, otherwise for the flush */
    /* If origRequest, put flush in mshr and replay */
    /* Stall the request if we are waiting on a response to a forwarded Flush */
    
    if (is_debug_addr(baseAddr)) printLine(baseAddr);
    
    if (origRequest != NULL) {
        processRequestInMSHR(baseAddr, event);
        if (action == DONE) {
            mshr_->removeFront(baseAddr);
            recordLatency(origRequest);
            delete origRequest;
            activatePrevEvents(baseAddr);
        }
    } else {
        if (action == STALL) {
            processRequestInMSHR(baseAddr, event);
        } else {
            delete event;
            activatePrevEvents(baseAddr);
        }
    }
}


/*************************
 * Cache responses 
 * ***********************/


/* Handles processing for data responses - GetSResp and GetXResp */
void Cache::processCacheResponse(MemEvent* responseEvent, Addr baseAddr) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);

    MemEvent* origRequest = getOrigReq(mshr_->lookup(baseAddr));
    CacheLine * line = cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleResponse(responseEvent, line, origRequest);

    if (action == DONE) {
        if (responseEvent->getCmd() != Command::AckPut) {
            mshr_->removeFront(baseAddr);
        }

        if (is_debug_addr(baseAddr)) printLine(baseAddr);
        
        postRequestProcessing(origRequest, line, true);
    } else {
        if (is_debug_addr(baseAddr)) printLine(baseAddr);
    }
    delete responseEvent;
}



void Cache::processFetchResp(MemEvent * event, Addr baseAddr) {
    if (is_debug_addr(baseAddr)) printLine(baseAddr);

    MemEvent * origRequest = NULL;
    if (mshr_->exists(baseAddr)) origRequest = mshr_->lookupFront(baseAddr); /* Note that 'exists' returns true if there is a waiting MemEvent for this addr, ignores waiting evictions */
    CacheLine * line = cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleResponse(event, line, origRequest);

    delete event;
    
    if (is_debug_addr(baseAddr)) printLine(baseAddr);

    if (action == DONE) {
        if (origRequest != NULL) {
            recordLatency(origRequest);
            mshr_->removeFront(baseAddr);
            delete origRequest;
        }
        
        activatePrevEvents(baseAddr);
    }
}


/* ---------------------------------
   Writeback Related Functions
   --------------------------------- */
bool Cache::allocateLine(MemEvent * event, Addr baseAddr) {
    CacheLine * replacementLine = cacheArray_->findReplacementCandidate(baseAddr, true);
    
    if (replacementLine->valid() && (is_debug_addr(baseAddr))) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 "\n", replacementLine->getBaseAddr());
    }

    /* Valid line indicates an eviction is needed, have cache coherence manager handle this */
    if (replacementLine->valid()) {
        if (replacementLine->inTransition()) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }
        
        CacheAction action = coherenceMgr_->handleEviction(replacementLine, this->getName(), false);
        if (action == STALL) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }
    }

    /* OK to replace line */
    notifyListenerOfEvict(event, replacementLine);
    cacheArray_->replace(baseAddr, replacementLine);
    return true;
}


bool Cache::allocateCacheLine(MemEvent* event, Addr baseAddr) {
    CacheLine* replacementLine = cacheArray_->findReplacementCandidate(baseAddr, true);
    
    if (replacementLine->valid() && (is_debug_addr(baseAddr) || is_debug_addr(replacementLine->getBaseAddr()))) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 "\n", replacementLine->getBaseAddr());
    }

    /* Valid line indicates an eviction is needed, have cache coherence manager handle this */
    if (replacementLine->valid()) {
        if (replacementLine->inTransition()) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }
        
        CacheAction action = coherenceMgr_->handleEviction(replacementLine, this->getName(), false);
        if (action == STALL) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), event->getBaseAddr());
            return false;
        }
    }
    
    /* OK to replace line  */
    notifyListenerOfEvict(event, replacementLine);
    cacheArray_->replace(baseAddr, replacementLine);
    return true;
}

// Allocate a new line in the cache part of this directory+cache. Link it to dirLine.
// If evicting a block, may need to evict it from the directory as well (if it had no sharers/owners/etc.)
bool Cache::allocateDirCacheLine(MemEvent * event, Addr baseAddr, CacheLine * dirLine, bool noStall) {
    CacheLine * replacementDirLine = cacheArray_->findReplacementCandidate(baseAddr, false);
    DataLine * replacementDataLine = replacementDirLine->getDataLine();
    if (dirLine == replacementDirLine) {
        cacheArray_->replace(baseAddr, dirLine, replacementDataLine);
        return true;
    }
    
    if (replacementDirLine->valid() && (is_debug_addr(baseAddr) || is_debug_addr(replacementDirLine->getBaseAddr()))) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 " from cache\n", replacementDirLine->getBaseAddr());
    }
    
    if (replacementDirLine->valid()) {  
        if (replacementDirLine->inTransition()) {
            if (noStall) return false;
            mshr_->insertPointer(replacementDirLine->getBaseAddr(), baseAddr);
            return false;
        }
        // should listener be informed?
        coherenceMgr_->handleEviction(replacementDirLine, this->getName(), true);
    }

    cacheArray_->replace(baseAddr, dirLine, replacementDataLine);
    return true;
}


void Cache::notifyListenerOfEvict(const MemEvent *event, 
                                  const CacheLine *replaceLine) {
    if (listener_) {
        CacheListenerNotification notify(replaceLine->getBaseAddr(), 
                                         replaceLine->getBaseAddr(), 
                                         0,
                                         event->getInstructionPointer(),
                                         replaceLine->getSize(), EVICT, NA);
        listener_->notifyAccess(notify);
    }
}


/* -------------------------------------------------------------------------------------
            Helper Functions
 ------------------------------------------------------------------------------------- */
/*
 *  Replay stalled/blocked events
 *  Three types:
 *      Event - an event that arrived and had to be buffered while or until it could be handled
 *      Self pointer - pointer to one's own address indicating a writeback ack is needed before any new requests can be issued
 *      Pointer - pointer to a different address indicating that the address is waiting for this block to evict
 *
 *  Special cases occur when a self-pointer is at the head of the queue
 *      1. Should replay any waiting pointers as the block has been written back
 *      2. Should replay any waiting external events (i.e., Inv, FetchInv, FetchInvX, etc.)
 *      3. Should not replay any waiting internal events (i.e., GetS, GetX, etc.)
 *
 */
void Cache::activatePrevEvents(Addr baseAddr) {
    //d_->debug(_L3_, "Called activatePrevEvents with addr = 0x%" PRIx64 ". Printing MSHR...\n", baseAddr);
    //mshr_->printTable();
    if (!mshr_->isHit(baseAddr)) return;
    
    vector<mshrType> entries = mshr_->removeAll(baseAddr);
    bool cont;
    int i = 0;
    
    if (is_debug_addr(baseAddr)) d_->debug(_L3_,"---------Replaying Events--------- Size: %zu\n", entries.size());
    
    bool writebackInProgress = false;   // Use this to allow replay of pointers but NOT events for this addr because an eviction is in progress
    
    // Self-pointer check -> remove it and record that we have it if needed
    if ((*entries.begin()).elem.isAddr() && ((*entries.begin()).elem).getAddr() == baseAddr) {
        entries.erase(entries.begin());
        writebackInProgress = true;
    }

    for (vector<mshrType>::iterator it = entries.begin(); it != entries.end(); i++) {
        if ((*it).elem.isAddr()) {                          /* Pointer Type */
            Addr pointerAddr = ((*it).elem).getAddr();
            
            if (is_debug_addr(baseAddr)) d_->debug(_L6_,"Pointer Addr: %" PRIx64 "\n", pointerAddr);
            
            if (!mshr_->isHit(pointerAddr)) {                     /* Entry has been already been processed, delete mshr entry */
                entries.erase(it);
                continue;
            }
            
            // Reactivate entries for address that was waiting for baseAddr
            // This entry list shouldn't include pointers unless a writeback occured (and even then...)
            vector<mshrType> pointerEntries = mshr_->removeAll(pointerAddr);    
            for (vector<mshrType>::iterator it2 = pointerEntries.begin(); it2 != pointerEntries.end(); i++) {
                if ((*it2).elem.isAddr()) {
                    Addr elemAddr = ((*it2).elem).getAddr();
                    if (elemAddr == pointerAddr) {
                        
                        if (is_debug_addr(baseAddr) || is_debug_addr(elemAddr)) d_->debug(_L5_, "Cache eviction raced with stalled request, wait for AckPut\n");
                        
                        mshr_->insertAll(pointerAddr, pointerEntries);
                        break;
                    } else {
                        out_->fatal(CALL_INFO, -1, "%s, Error: Reactivating events for addr = 0x%" PRIx64 " and encountered unexpected mshr entry not of type MemEvent. Time = %" PRIu64 " ns\n",
                                this->getName().c_str(), pointerAddr, getCurrentSimTimeNano());
                    }
                }
                cont = activatePrevEvent(((*it2).elem).getEvent(), pointerEntries, pointerAddr, it2, i);
                if (!cont) break;
            }
            entries.erase(it);  // Erase processed pointer

            // Check if we need to stop because a new event is in our mshr thanks to the processing of the pointer events?
            if (mshr_->isHit(baseAddr)) {
                bool stop = false;
                if (entries.begin()->elem.isEvent()) {
                    MemEvent * front = ((entries.begin())->elem).getEvent();
                    if (front->getCmd() != Command::Inv && front->getCmd() != Command::FetchInv && front->getCmd() != Command::FetchInvX && front->getCmd() != Command::ForceInv) {
                        stop = true;
                    }
                } else {
                    stop = true;
                }
                if (stop) {
                    mshr_->insertAll(baseAddr, entries);
                    break;
                }
            }
        } else {    /* MemEvent Type */
            Command cmd = ((entries.begin())->elem).getEvent()->getCmd();
            if (writebackInProgress) {
                if (cmd != Command::Inv && cmd != Command::FetchInv && cmd != Command::FetchInvX && cmd != Command::ForceInv) {
                    mshr_->insertAll(baseAddr, entries);
                    break;
                } else {
                    writebackInProgress = false;
                }
            } 
            cont = activatePrevEvent(((*it).elem).getEvent(), entries, baseAddr, it, i);
            if (!cont) break;
        }
    }
    if (writebackInProgress) mshr_->insertWriteback(baseAddr);
    
    if (is_debug_addr(baseAddr)) d_->debug(_L3_,"---------end---------\n");
}



bool Cache::activatePrevEvent(MemEvent* event, vector<mshrType>& entries, Addr addr, vector<mshrType>::iterator it, int index) {
    if (is_debug_addr(addr)) d_->debug(_L3_,"Replaying event #%i, cmd = %s, bsAddr: %" PRIx64 ", addr: %" PRIx64 ", dst: %s\n",
                  index, CommandString[(int)event->getCmd()], toBaseAddr(event->getAddr()), event->getAddr(), event->getDst().c_str());
    if (is_debug_addr(addr)) d_->debug(_L3_,"--------------------------------------\n");
    
    this->processEvent(event, true);
    
    if (is_debug_addr(addr)) d_->debug(_L3_,"--------------------------------------\n");
    
    entries.erase(it);
    
    /* If the event we just ran 'blocked', then there is not reason to activate other events. */
    /* However we do need to replay requests from lower levels! (Like Inv) Otherwise deadlock! */
    if (mshr_->isHit(addr)) {
        bool stop = false;
        if (entries.begin()->elem.isEvent()) {
            MemEvent * front = ((entries.begin())->elem).getEvent();
            if (front->getCmd() != Command::Inv && front->getCmd() != Command::FetchInv && front->getCmd() != Command::FetchInvX && front->getCmd() != Command::ForceInv) 
                stop = true;
        } else {
            stop = true;
        }
        if (stop) {
            mshr_->insertAll(addr, entries);
            return false;
        }
    }
    return true;
}

void Cache::recordLatency(MemEvent* event) {
    uint64 issueTime = (startTimeList_.find(event))->second;
    if (missTypeList_.find(event) != missTypeList_.end()) {
        int missType = missTypeList_.find(event)->second;
        switch (missType) {
            case 0:
                coherenceMgr_->recordLatency(Command::GetS, IS, timestamp_ - issueTime);
                break;
            case 1:
                coherenceMgr_->recordLatency(Command::GetS, M, timestamp_ - issueTime);
                break;
            case 2:
                coherenceMgr_->recordLatency(Command::GetX, IM, timestamp_ - issueTime);
                break;
            case 3:
                coherenceMgr_->recordLatency(Command::GetX, SM, timestamp_ - issueTime);
                break;
            case 4:
                coherenceMgr_->recordLatency(Command::GetX, M, timestamp_ - issueTime);
                break;
            case 5:
                coherenceMgr_->recordLatency(Command::GetSX, IM, timestamp_ - issueTime);
                break;
            case 6:
                coherenceMgr_->recordLatency(Command::GetSX, SM, timestamp_ - issueTime);
                break;
            case 7:
                coherenceMgr_->recordLatency(Command::GetSX, M, timestamp_ - issueTime);
                break;
            default:
                break;
        }
        missTypeList_.erase(event);
    }
    startTimeList_.erase(event);
}


void Cache::postReplacementProcessing(MemEvent * event, CacheAction action, bool replay) {
    Addr baseAddr = event->getBaseAddr();

    /* Got a writeback, check if waiting events can proceed now */
    if (action == DONE) activatePrevEvents(baseAddr);
    
    /* Clean up */
    delete event;
}


void Cache::postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool replay) {
    Command cmd = event->getCmd();
    Addr addr   = event->getBaseAddr();

    recordLatency(event);
    delete event;
    
    /* For atomic requests handled by the cache itself, GetX unlocks the cache line.  Therefore,
       we possibly need to 'replay' events that blocked due to an locked cacheline */
    if (cmd == Command::GetX && L1_ && cacheLine->getEventsWaitingForLock() && !cacheLine->isLocked()) reActivateEventWaitingForUserLock(cacheLine);

    if (mshr_->isHit(addr)) activatePrevEvents(addr);   // Replay any waiting events that blocked for this one

}


void Cache::reActivateEventWaitingForUserLock(CacheLine* cacheLine) {
    Addr baseAddr = cacheLine->getBaseAddr();
    if (cacheLine->getEventsWaitingForLock() && !cacheLine->isLocked()) {
        cacheLine->setEventsWaitingForLock(false);
        if (mshr_->isHit(baseAddr)) activatePrevEvents(baseAddr);
    }
}


/* ---------------------------------------
   Extras
   --------------------------------------- */
MemEvent* Cache::getOrigReq(const vector<mshrType> entries) {
    if (entries.front().elem.isAddr()) {
        out_->fatal(CALL_INFO, -1, "%s, Error: Request at front of the mshr is not of type MemEvent. Time = %" PRIu64 "\n",
                this->getName().c_str(), getCurrentSimTimeNano());
    }

    return (entries.front().elem).getEvent();
}

bool Cache::processRequestInMSHR(Addr baseAddr, MemEvent* event) {
    if (mshr_->insert(baseAddr, event)) {
        return true;
    } else {
        sendNACK(event);
        return false;
    }
}


/* Invalidations/fetches will wait for the current outstanding transaction, but no waiting ones! */
bool Cache::processInvRequestInMSHR(Addr baseAddr, MemEvent* event, bool inProgress) {
    if (mshr_->insertInv(baseAddr, event, inProgress)) {
        return true;
    } else {
        sendNACK(event);
        return false;
    }
}


void Cache::sendNACK(MemEvent* event) {
    if (event->isCPUSideEvent()) {
        coherenceMgr_->sendNACK(event, true, getCurrentSimTimeNano());
    } else {
        coherenceMgr_->sendNACK(event, false, getCurrentSimTimeNano());
    }
}


/*
 *  Response latency: MSHR latency because MSHR lookup to find event that was nacked. No cache access.
 */
void Cache::processIncomingNACK(MemEvent* origReqEvent) {
    if (is_debug_event(origReqEvent)) d_->debug(_L3_,"NACK received.\n");
    
    /* Determine whether NACKed event needs to be retried */
    CacheLine * cacheLine = cacheArray_->lookup(origReqEvent->getBaseAddr(), false);
    if (!coherenceMgr_->isRetryNeeded(origReqEvent, cacheLine)) {
        if (is_debug_event(origReqEvent)) d_->debug(_L4_, "Dropping NACKed request\n");
        
        delete origReqEvent;    // TODO: should do this here?
        return;
    }

    /* Determine what CC will retry sending the event */
    if (origReqEvent->fromHighNetNACK()) {
        coherenceMgr_->resendEvent(origReqEvent, true);
    } else if (origReqEvent->fromLowNetNACK()) {
        coherenceMgr_->resendEvent(origReqEvent, false);
    } else
        out_->fatal(CALL_INFO, -1, "%s, ProcessIncomingNACK, command not recognized. Event: %s\n", getName().c_str(), origReqEvent->getVerboseString().c_str());
}

void Cache::printLine(Addr addr) {
    if (!is_debug_addr(addr)) return;
    if (type_ == "noninclusive_with_directory") {
        CacheLine * line = cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        bool isCached = (line == nullptr) ? false : (line->getDataLine() != NULL);
        unsigned int sharers = (line == nullptr) ? 0 : line->numSharers();
        string owner = (line == nullptr) ? "" : line->getOwner();
        d_->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\" %d\n", 
                addr, StateString[state], sharers, owner.c_str(), isCached); 
    } else if (L1_) {
        CacheLine * line = cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        d_->debug(_L8_, "0x%" PRIx64 ": %s\n", addr, StateString[state]);
    } else {
        CacheLine * line = cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        unsigned int sharers = (line == NULL) ? 0 : line->numSharers();
        string owner = (line == NULL) ? "" : line->getOwner();
        d_->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\"\n", addr, StateString[state], sharers, owner.c_str());
    }
}


bool operator== ( const mshrType& n1, const mshrType& n2) {
    if (n1.elem.isAddr()) return false;
    return((n1.elem).getEvent() == (n2.elem).getEvent());
}

void Cache::printStatus(Output &out) {
    out.output("MemHierarchy::Cache %s\n", getName().c_str());
    out.output("  Clock is %s. Last active cycle: %" PRIu64 "\n", clockIsOn_ ? "on" : "off", timestamp_);
    out.output("  Requests waiting to be handled by cache: %zu\n", requestBuffer_.size());
    out.output("  Requests waiting for bank access: %zu\n", bankConflictBuffer_.size());
    if (mshr_) {
        out.output("  MSHR Status:\n");
        mshr_->printStatus(out);
    }
    if (linkUp_ && linkUp_ != linkDown_) {
        out.output("  Up link status: ");
        linkUp_->printStatus(out);
        out.output("  Down link status: ");
    } else {
        out.output("  Link status: ");
    }
    if (linkDown_) linkDown_->printStatus(out);

    out.output("  Cache contents:\n");
    cacheArray_->printCacheArray(out);
    out.output("End MemHierarchy::Cache\n\n");
}

void Cache::emergencyShutdown() {
    if (out_->getVerboseLevel() > 1) {
        if (out_->getOutputLocation() == Output::STDOUT)
            out_->setOutputLocation(Output::STDERR);
        printStatus(*out_);
        if (linkUp_ && linkUp_ != linkDown_) {
            out_->output("  Checking for unreceived events on up link: \n");
            linkUp_->emergencyShutdownDebug(*out_);
            out_->output("  Checking for unreceived events on down link: \n");
        } else {
            out_->output("  Checking for unreceived events on link: \n");
        }
        linkDown_->emergencyShutdownDebug(*out_);
    }
}

