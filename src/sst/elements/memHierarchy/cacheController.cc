// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
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
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <csignal>

#include "cacheController.h"
#include "memEvent.h"
#include "mshr.h"
#include "coherenceController.h"
#include "hash.h"


using namespace SST;
using namespace SST::MemHierarchy;


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
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, false);
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
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
    bool updateLine = !replay && MemEvent::isDataRequest(cmd);   /* TODO: move replacement manager update to time when cache actually sends a response */
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, updateLine);; 
    
    bool miss = (line == nullptr);
#ifdef __SST_DEBUG_OUTPUT__
    if (miss && (DEBUG_ALL || DEBUG_ADDR == baseAddr)) d_->debug(_L3_, "-- Miss --\n");
#endif
    if (!miss && line->inTransition()) {
        processRequestInMSHR(baseAddr, event);
        return;
    }
    if (cf_.type_ == "inclusive" && miss && !allocateLine(event, baseAddr)) {
        processRequestInMSHR(baseAddr, event);
        return;
    } else if (cf_.type_ == "noninclusive_with_directory" && miss && !allocateLine(event, baseAddr)) {
        processRequestInMSHR(baseAddr, event);
        return;
    } else if (cf_.type_ == "noninclusive" && (miss || !line->valid())) {
        processRequestInMSHR(baseAddr, event);
        if (event->inProgress()) {
#ifdef __SST_DEBUG_OUTPUT__
            d_->debug(_L8_, "Attempted retry too early, continue stalling\n");
#endif
            return;
        }

        // Forward instead of allocating for non-inclusive caches
        vector<uint8_t> * data = &event->getPayload();
        coherenceMgr_->forwardMessage(event, baseAddr, event->getSize(), 0, data); // Event to forward, address, requested size, data (if any)
        event->setInProgress(true);
        return;
    }
    
    line = cf_.cacheArray_->lookup(baseAddr, false);

    // Special case -> allocate line for prefetches to non-inclusive caches
    bool localPrefetch = event->isPrefetch() && event->getRqstr() == getName();
    if (cf_.type_ == "noninclusive_with_directory" && localPrefetch && line->getDataLine() == NULL && line->getState() == I) {
        if (!allocateDirCacheLine(event, baseAddr, line, false)) {
#ifdef __SST_DEBUG_OUTPUT__
                if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_, "-- Data Cache Miss --\n");
#endif
                processRequestInMSHR(baseAddr, event);
                return;
        }
    }

    // Handle hit
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
    CacheAction action = coherenceMgr_->handleRequest(event, line, replay);
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif

    CacheLine * line = nullptr;
    // May need to allocate for non-inclusive or incoherent caches
    if (cf_.type_ == "noninclusive" || cf_.protocol_ == CoherenceProtocol::NONE) {
        line = cf_.cacheArray_->lookup(baseAddr, true); // Update replacement metadata
        if (line == nullptr) { // miss
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_, "-- Cache Miss --\n");
#endif
            if (!allocateLine(event, baseAddr)) {
                if (mshr_->getAcksNeeded(baseAddr) == 0) {
                    processRequestInMSHR(baseAddr, event);
                    return;
#ifdef __SST_DEBUG_OUTPUT__
                } else {
                    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_, "Can't allocate immediately, but handling collision anyways\n");
#endif
                }
            }
        }       
    }
    
    line = cf_.cacheArray_->lookup(baseAddr, false); 
    if (cf_.type_ == "noninclusive_with_directory") {
        if (line->getDataLine() == NULL) {
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_, "-- Cache Miss --\n");
#endif
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
    
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
    if (!mshr_->pendingWriteback(baseAddr) && mshr_->isFull()) {
        processInvRequestInMSHR(baseAddr, event, false); // trigger a NACK
        return;
    }
    
    MemEvent * collisionEvent = NULL;
    if (mshr_->exists(baseAddr)) collisionEvent = mshr_->lookupFront(baseAddr);
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleInvalidationRequest(event, line, collisionEvent, replay);
        
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, false);
    bool miss = (line == nullptr);
    // Find line
    //      If hit and in transition: buffer in MSHR
    //      If hit and dirty: forward cacheFlush w/ data, wait for flushresp
    //      If hit and clean: forward cacheFlush w/o data, wait for flushresp
    //      If miss: forward cacheFlush w/o data, wait for flushresp
    
    if (event->inProgress()) {
        processRequestInMSHR(baseAddr, event);
#ifdef __SST_DEBUG_OUTPUT__
        d_->debug(_L8_, "Attempted retry too early, continue stalling\n");
#endif
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
    
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif

    MemEvent* origRequest = getOrigReq(mshr_->lookup(baseAddr));
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleResponse(responseEvent, line, origRequest);

    if (action == DONE) {
        if (responseEvent->getCmd() != AckPut) {
            mshr_->removeFront(baseAddr);
        }
#ifdef __SST_DEBUG_OUTPUT__
        printLine(baseAddr);
#endif
        postRequestProcessing(origRequest, line, true);
#ifdef __SST_DEBUG_OUTPUT__
    } else {
        printLine(baseAddr);
#endif
    }
    delete responseEvent;
}



void Cache::processFetchResp(MemEvent * event, Addr baseAddr) {
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif

    MemEvent * origRequest = NULL;
    if (mshr_->exists(baseAddr)) origRequest = mshr_->lookupFront(baseAddr); /* Note that 'exists' returns true if there is a waiting MemEvent for this addr, ignores waiting evictions */
    CacheLine * line = cf_.cacheArray_->lookup(baseAddr, false);
    CacheAction action = coherenceMgr_->handleResponse(event, line, origRequest);

    delete event;
    
#ifdef __SST_DEBUG_OUTPUT__
    printLine(baseAddr);
#endif

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
    CacheLine * replacementLine = cf_.cacheArray_->findReplacementCandidate(baseAddr, true);
#ifdef __SST_DEBUG_OUTPUT__
    if (replacementLine->valid() && (DEBUG_ALL || DEBUG_ADDR == baseAddr || DEBUG_ADDR == replacementLine->getBaseAddr())) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 "\n", replacementLine->getBaseAddr());
    }
#endif

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
    cf_.cacheArray_->replace(baseAddr, replacementLine);
    return true;
}


bool Cache::allocateCacheLine(MemEvent* event, Addr baseAddr) {
    CacheLine* replacementLine = cf_.cacheArray_->findReplacementCandidate(baseAddr, true);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (replacementLine->valid() && (DEBUG_ALL || DEBUG_ADDR == baseAddr || DEBUG_ADDR == replacementLine->getBaseAddr())) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 "\n", replacementLine->getBaseAddr());
    }
#endif

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
    cf_.cacheArray_->replace(baseAddr, replacementLine);
    return true;
}

/*
 *  Allocate a new line in the directory portion of the directory+cache
 *  Do not touch the cache part unless we need to evict a directory entry that is cached 
 */
bool Cache::allocateDirLine(MemEvent* event, Addr baseAddr) {
    CacheLine* replacementLine = cf_.directoryArray_->findReplacementCandidate(baseAddr, true);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (replacementLine->valid() && (DEBUG_ALL || DEBUG_ADDR == baseAddr || DEBUG_ADDR == replacementLine->getBaseAddr())) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 " from directory.\n", replacementLine->getBaseAddr());
    }
#endif

    // Valid line indicates an eviction is needed, have directory coherence manager handle this
    if (replacementLine->valid()) {
        if (replacementLine->inTransition()) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), baseAddr);
            return false;
        }

        CacheAction action = coherenceMgr_->handleEviction(replacementLine, this->getName(), false);
        if (action == STALL) {
            mshr_->insertPointer(replacementLine->getBaseAddr(), baseAddr);
            return false;
        }
    }

    /* OK to replace line  */
    cf_.directoryArray_->replace(baseAddr, replacementLine);
    return true;
}

// Allocate a new line in the cache part of this directory+cache. Link it to dirLine.
// If evicting a block, may need to evict it from the directory as well (if it had no sharers/owners/etc.)
bool Cache::allocateDirCacheLine(MemEvent * event, Addr baseAddr, CacheLine * dirLine, bool noStall) {
    CacheLine * replacementDirLine = cf_.cacheArray_->findReplacementCandidate(baseAddr, false);
    DataLine * replacementDataLine = replacementDirLine->getDataLine();
    if (dirLine == replacementDirLine) {
        cf_.cacheArray_->replace(baseAddr, dirLine, replacementDataLine);
        return true;
    }
#ifdef __SST_DEBUG_OUTPUT__
    if (replacementDirLine->valid() && (DEBUG_ALL || DEBUG_ADDR == baseAddr || DEBUG_ADDR == replacementDirLine->getBaseAddr())) {
        d_->debug(_L6_, "Evicting 0x%" PRIx64 " from cache\n", replacementDirLine->getBaseAddr());
    }
#endif
    if (replacementDirLine->valid()) {  
        if (replacementDirLine->inTransition()) {
            if (noStall) return false;
            mshr_->insertPointer(replacementDirLine->getBaseAddr(), baseAddr);
            return false;
        }
        coherenceMgr_->handleEviction(replacementDirLine, this->getName(), true);
    }

    cf_.cacheArray_->replace(baseAddr, dirLine, replacementDataLine);
    return true;
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
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"---------Replaying Events--------- Size: %lu\n", entries.size());
#endif
    bool writebackInProgress = false;   // Use this to allow replay of pointers but NOT events for this addr because an eviction is in progress
    
    // Self-pointer check -> remove it and record that we have it if needed
    if ((*entries.begin()).elem.type() == typeid(Addr) && boost::get<Addr>((*entries.begin()).elem) == baseAddr) {
        entries.erase(entries.begin());
        writebackInProgress = true;
    }

    for (vector<mshrType>::iterator it = entries.begin(); it != entries.end(); i++) {
        if ((*it).elem.type() == typeid(Addr)) {                          /* Pointer Type */
            Addr pointerAddr = boost::get<Addr>((*it).elem);
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L6_,"Pointer Addr: %" PRIx64 "\n", pointerAddr);
#endif
            if (!mshr_->isHit(pointerAddr)) {                     /* Entry has been already been processed, delete mshr entry */
                entries.erase(it);
                continue;
            }
            
            // Reactivate entries for address that was waiting for baseAddr
            // This entry list shouldn't include pointers unless a writeback occured (and even then...)
            vector<mshrType> pointerEntries = mshr_->removeAll(pointerAddr);    
            for (vector<mshrType>::iterator it2 = pointerEntries.begin(); it2 != pointerEntries.end(); i++) {
                if ((*it2).elem.type() != typeid(MemEvent*)) {
                    Addr elemAddr = boost::get<Addr>((*it2).elem);
                    if (elemAddr == pointerAddr) {
#ifdef __SST_DEBUG_OUTPUT__
                        d_->debug(_L5_, "Cache eviction raced with stalled request, wait for AckPut\n");
#endif
                        mshr_->insertAll(pointerAddr, pointerEntries);
                        break;
                    } else {
                        d_->fatal(CALL_INFO, -1, "%s, Error: Reactivating events for addr = 0x%" PRIx64 " and encountered unexpected mshr entry not of type MemEvent. Time = %" PRIu64 " ns\n",
                                this->getName().c_str(), pointerAddr, getCurrentSimTimeNano());
                    }
                }
                cont = activatePrevEvent(boost::get<MemEvent*>((*it2).elem), pointerEntries, pointerAddr, it2, i);
                if (!cont) break;
            }
            entries.erase(it);  // Erase processed pointer

            // Check if we need to stop because a new event is in our mshr thanks to the processing of the pointer events?
            if (mshr_->isHit(baseAddr)) {
                bool stop = false;
                if (entries.begin()->elem.type() == typeid(MemEvent*)) {
                    MemEvent * front = boost::get<MemEvent*>((entries.begin())->elem);
                    if (front->getCmd() != Inv && front->getCmd() != FetchInv && front->getCmd() != FetchInvX) {
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
            Command cmd = boost::get<MemEvent*>((entries.begin())->elem)->getCmd();
            if (writebackInProgress) {
                if (cmd != Inv && cmd != FetchInv && cmd != FetchInvX) {
                    mshr_->insertAll(baseAddr, entries);
                    break;
                } else {
                    writebackInProgress = false;
                }
            } 
            cont = activatePrevEvent(boost::get<MemEvent*>((*it).elem), entries, baseAddr, it, i);
            if (!cont) break;
        }
    }
    if (writebackInProgress) mshr_->insertWriteback(baseAddr);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L3_,"---------end---------\n");
#endif
}



bool Cache::activatePrevEvent(MemEvent* event, vector<mshrType>& _entries, Addr _addr, vector<mshrType>::iterator it, int index) {
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == _addr) d_->debug(_L3_,"Replaying event #%i, cmd = %s, bsAddr: %" PRIx64 ", addr: %" PRIx64 ", dst: %s\n",
                  index, CommandString[event->getCmd()], toBaseAddr(event->getAddr()), event->getAddr(), event->getDst().c_str());
    if (DEBUG_ALL || DEBUG_ADDR == _addr) d_->debug(_L3_,"--------------------------------------\n");
#endif
    
    this->processEvent(event, true);
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == _addr) d_->debug(_L3_,"--------------------------------------\n");
#endif
    
    _entries.erase(it);
    
    /* If the event we just ran 'blocked', then there is not reason to activate other events. */
    /* However we do need to replay requests from lower levels! (Like Inv) Otherwise deadlock! */
    if (mshr_->isHit(_addr)) {
        bool stop = false;
        if (_entries.begin()->elem.type() == typeid(MemEvent*)) {
            MemEvent * front = boost::get<MemEvent*>((_entries.begin())->elem);
            if (front->getCmd() != Inv && front->getCmd() != FetchInv && front->getCmd() != FetchInvX) stop = true;
        } else {
            stop = true;
        }
        if (stop) {
            mshr_->insertAll(_addr, _entries);
            return false;
        }
    }
    return true;
}

void Cache::recordLatency(MemEvent* event) {
    uint64 issueTime = (startTimeList.find(event))->second;
    if (missTypeList.find(event) != missTypeList.end()) {
        int missType = missTypeList.find(event)->second;
        switch (missType) {
            case 0:
                coherenceMgr_->recordLatency(GetS, IS, timestamp_ - issueTime);
                break;
            case 1:
                coherenceMgr_->recordLatency(GetS, M, timestamp_ - issueTime);
                break;
            case 2:
                coherenceMgr_->recordLatency(GetX, IM, timestamp_ - issueTime);
                break;
            case 3:
                coherenceMgr_->recordLatency(GetX, SM, timestamp_ - issueTime);
                break;
            case 4:
                coherenceMgr_->recordLatency(GetX, M, timestamp_ - issueTime);
                break;
            case 5:
                coherenceMgr_->recordLatency(GetSEx, IM, timestamp_ - issueTime);
                break;
            case 6:
                coherenceMgr_->recordLatency(GetSEx, SM, timestamp_ - issueTime);
                break;
            case 7:
                coherenceMgr_->recordLatency(GetSEx, M, timestamp_ - issueTime);
                break;
            default:
                break;
        }
        missTypeList.erase(event);
    }
    startTimeList.erase(event);
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

    /* Clean up */
    if (event->isPrefetch() && event->getRqstr() == this->getName() && !replay) {
        statPrefetchHit->addData(1);
    }
    recordLatency(event);
    delete event;
    
    /* For atomic requests handled by the cache itself, GetX unlocks the cache line.  Therefore,
       we possibly need to 'replay' events that blocked due to an locked cacheline */
    if (cmd == GetX && cf_.L1_ && cacheLine->getEventsWaitingForLock() && !cacheLine->isLocked()) reActivateEventWaitingForUserLock(cacheLine);

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
    if (entries.front().elem.type() != typeid(MemEvent*)) {
        d_->fatal(CALL_INFO, -1, "%s, Error: Request at front of the mshr is not of type MemEvent. Time = %" PRIu64 "\n",
                this->getName().c_str(), getCurrentSimTimeNano());
    }

    return boost::get<MemEvent*>(entries.front().elem);
}



void Cache::errorChecking() {    
    if (cf_.MSHRSize_ <= 0)             d_->fatal(CALL_INFO, -1, "MSHR size not specified correctly. \n");
    if (cf_.numLines_ <= 0)             d_->fatal(CALL_INFO, -1, "Number of lines not set correctly. \n");
    if (!isPowerOfTwo(cf_.lineSize_))   d_->fatal(CALL_INFO, -1, "Cache line size is not a power of two. \n");
}



void Cache::pMembers() {
    string protocolStr;
    if (cf_.protocol_ == CoherenceProtocol::MESI) protocolStr = "MESI";
    else if (cf_.protocol_ == CoherenceProtocol::NONE) protocolStr = "NONE";
    else protocolStr = "MSI";
    
    d_->debug(_INFO_,"Coherence Protocol: %s \n", protocolStr.c_str());
    d_->debug(_INFO_,"Cache lines: %d \n", cf_.numLines_);
    d_->debug(_INFO_,"Cache line size: %d \n", cf_.lineSize_);
    d_->debug(_INFO_,"MSHR entries:  %d \n\n", cf_.MSHRSize_);
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
    if (event->isHighNetEvent()) {
        coherenceMgr_->sendNACK(event, true, getCurrentSimTimeNano());
    } else if (event->isLowNetEvent()) {
        coherenceMgr_->sendNACK(event, false, getCurrentSimTimeNano());
    }
    else
        d_->fatal(CALL_INFO, -1, "Command type not recognized, Cmd = %s\n", CommandString[event->getCmd()]);
}


/*
 *  Response latency: MSHR latency because MSHR lookup to find event that was nacked. No cache access.
 */
void Cache::processIncomingNACK(MemEvent* origReqEvent) {
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == origReqEvent->getBaseAddr()) d_->debug(_L3_,"NACK received.\n");
#endif
    
    /* Determine whether NACKed event needs to be retried */
    CacheLine * cacheLine = cf_.cacheArray_->lookup(origReqEvent->getBaseAddr(), false);
    if (!coherenceMgr_->isRetryNeeded(origReqEvent, cacheLine)) {
#ifdef __SST_DEBUG_OUTPUT__
        d_->debug(_L4_, "Dropping NACKed request\n");
#endif
        delete origReqEvent;    // TODO: should do this here?
        return;
    }

    /* Determine what CC will retry sending the event */
    if (origReqEvent->fromHighNetNACK()) {
        coherenceMgr_->resendEvent(origReqEvent, true);
    } else if (origReqEvent->fromLowNetNACK()) {
        coherenceMgr_->resendEvent(origReqEvent, false);
    } else
        d_->fatal(CALL_INFO, -1, "Command type not recognized, Cmd = %s\n", CommandString[origReqEvent->getCmd()]);
}

void Cache::printLine(Addr addr) {
    if (!DEBUG_ALL && DEBUG_ADDR != addr) return;
    if (cf_.type_ == "noninclusive_with_directory") {
        CacheLine * line = cf_.cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        bool isCached = (line == nullptr) ? false : (line->getDataLine() != NULL);
        unsigned int sharers = (line == nullptr) ? 0 : line->numSharers();
        string owner = (line == nullptr) ? "" : line->getOwner();
        d_->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\" %d\n", 
                addr, StateString[state], sharers, owner.c_str(), isCached); 
    } else if (cf_.L1_) {
        CacheLine * line = cf_.cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        d_->debug(_L8_, "0x%" PRIx64 ": %s\n", addr, StateString[state]);
    } else {
        CacheLine * line = cf_.cacheArray_->lookup(addr, false);
        State state = (line == nullptr) ? NP : line->getState();
        unsigned int sharers = (line == NULL) ? 0 : line->numSharers();
        string owner = (line == NULL) ? "" : line->getOwner();
        d_->debug(_L8_, "0x%" PRIx64 ": %s, %u, \"%s\"\n", addr, StateString[state], sharers, owner.c_str());
    }
}


bool operator== ( const mshrType& n1, const mshrType& n2) {
    if (n1.elem.type() == typeid(Addr)) return false;
    return(boost::get<MemEvent*>(n1.elem) == boost::get<MemEvent*>(n2.elem));
}

