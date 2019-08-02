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
 *  Handle a request from upper level caches
 *  This function is only called if there are no blocking requests already in the MSHRs
 *  1. Locate line, if present, in the cache array & update its replacement manager state
 *  2. If cache miss, allocate a new cache line, possibly evicting another line
 *  3. If neccessary, get block/coherence permission from lower level caches/memory
 *  4. If neccessary, invalidate upper level caches and/or request updated data
 *  5. Send response to requestor
 */
void Cache::processCacheRequest(MemEvent* event, Command cmd, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);
    CacheAction action = coherenceMgr_->handleRequest(event, replay);
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);
    
    // Post-request processing and/or stall
    if (action == DONE) {
        Addr addr = event->getBaseAddr();
        delete event;
    
        /* For atomic requests handled by the cache itself, GetX unlocks the cache line.  Therefore,
           we possibly need to 'replay' events that blocked due to an locked cacheline */
        if (L1_) {
            CacheLine* cacheLine = cacheArray_->lookup(addr, false);
            if( cacheLine->getEventsWaitingForLock() && !cacheLine->isLocked()) reActivateEventWaitingForUserLock(cacheLine);
        }

        if (mshr_->isHit(addr)) activatePrevEvents(addr);   // Replay any waiting events that blocked for this one
    }
}


/* 
 *  Handles processing for all replacements - PutS, PutE, PutM, etc. 
 *  For non-inclusive/incoherent caches, may need to allocate a new line
 *  If Put conflicts with existing request, will still call handleReplacement to resolve any races
 */
void Cache::processCacheReplacement(MemEvent* event, Command cmd, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);

    // Attempt replacement, also handle any racing requests
    CacheAction action = coherenceMgr_->handleReplacement(event, replay);
    
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);
    
    if (action != STALL && action != BLOCK)
        postReplacementProcessing(event, action, replay);
}


void Cache::processCacheFlush(MemEvent* event, Addr baseAddr, bool replay) {
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);
    
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
    if (mshr_->exists(baseAddr)) origRequest = static_cast<MemEvent*>(mshr_->lookupFront(baseAddr));
    
    // Generally we should not nack this request without checking for races
    // But if no possible races and handling this will fill MSHR, nack it
    if (!origRequest && mshr_->isAlmostFull()) { 
        sendNACK(event);
        return;
    }

    CacheAction action = coherenceMgr_->handleFlush(event, line, origRequest, replay);
    
    /* Action returned is for the origRequest if it exists, otherwise for the flush */
    /* If origRequest, put flush in mshr and replay */
    /* Stall the request if we are waiting on a response to a forwarded Flush */
    
    if (is_debug_addr(baseAddr)) coherenceMgr_->printLine(baseAddr);
    
    if (origRequest != NULL) {
        processRequestInMSHR(baseAddr, event);
        if (action == DONE) {
            mshr_->removeFront(baseAddr);
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
    CacheAction action = coherenceMgr_->handleCacheResponse(responseEvent, true);

    if (action == DONE) {
        CacheLine* line = cacheArray_->lookup(baseAddr, false);
        if (L1_ && line && line->getEventsWaitingForLock() && !line->isLocked()) 
            reActivateEventWaitingForUserLock(line);

        if (mshr_->isHit(baseAddr)) 
            activatePrevEvents(baseAddr);   // Replay any waiting events that blocked for this one
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
    
    list<MSHREntry> entries = mshr_->removeAll(baseAddr);
    bool cont;
    int i = 0;
    
    if (is_debug_addr(baseAddr)) d_->debug(_L3_,"---------Replaying Events--------- Size: %zu\n", entries.size());
    
    bool writebackInProgress = false;   // Use this to allow replay of pointers but NOT events for this addr because an eviction is in progress
    
    // Self-pointer check -> remove it and record that we have it if needed
    if ((*entries.begin()).elem.isAddr() && ((*entries.begin()).elem).getAddr() == baseAddr) {
        entries.erase(entries.begin());
        writebackInProgress = true;
    }

    for (list<MSHREntry>::iterator it = entries.begin(); it != entries.end(); i++) {
        if ((*it).elem.isAddr()) {                          /* Pointer Type */
            Addr pointerAddr = ((*it).elem).getAddr();
            
            if (is_debug_addr(baseAddr)) d_->debug(_L6_,"Pointer Addr: %" PRIx64 "\n", pointerAddr);
            
            if (!mshr_->isHit(pointerAddr)) {                     /* Entry has been already been processed, delete mshr entry */
                it = entries.erase(it);
                continue;
            }
            
            // Reactivate entries for address that was waiting for baseAddr
            // This entry list shouldn't include pointers unless a writeback occured (and even then...)
            list<MSHREntry> pointerEntries = mshr_->removeAll(pointerAddr);    
            for (list<MSHREntry>::iterator it2 = pointerEntries.begin(); it2 != pointerEntries.end(); i++) {
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
                cont = activatePrevEvent(static_cast<MemEvent*>(((*it2).elem).getEvent()), pointerEntries, pointerAddr, it2, i);
                if (!cont) break;
            }
            it = entries.erase(it);  // Erase processed pointer

            // Check if we need to stop because a new event is in our mshr thanks to the processing of the pointer events?
            if (mshr_->isHit(baseAddr)) {
                bool stop = false;
                if (entries.size() != 0 && entries.begin()->elem.isEvent()) {
                    MemEvent * front = static_cast<MemEvent*>(((entries.begin())->elem).getEvent());
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
            cont = activatePrevEvent(static_cast<MemEvent*>(((*it).elem).getEvent()), entries, baseAddr, it, i);
            if (!cont) break;
        }
    }
    if (writebackInProgress) mshr_->insertWriteback(baseAddr);
    
    if (is_debug_addr(baseAddr)) d_->debug(_L3_,"---------end---------\n");
}



bool Cache::activatePrevEvent(MemEvent* event, list<MSHREntry>& entries, Addr addr, list<MSHREntry>::iterator& it, int index) {
    if (is_debug_addr(addr)) d_->debug(_L3_,"Replaying event #%i, cmd = %s, bsAddr: %" PRIx64 ", addr: %" PRIx64 ", dst: %s\n",
                  index, CommandString[(int)event->getCmd()], toBaseAddr(event->getAddr()), event->getAddr(), event->getDst().c_str());
    if (is_debug_addr(addr)) d_->debug(_L3_,"--------------------------------------\n");
    
    processEvent(event, true);
    
    if (is_debug_addr(addr)) d_->debug(_L3_,"--------------------------------------\n");
    
    it = entries.erase(it);
    /* If the event we just ran 'blocked', then there is not reason to activate other events. */
    /* However we do need to replay requests from lower levels! (Like Inv) Otherwise deadlock! */
    if (mshr_->isHit(addr)) {
        bool stop = false;
        if (!entries.empty() && entries.begin()->elem.isEvent()) {
            MemEvent * front = static_cast<MemEvent*>(((entries.begin())->elem).getEvent());
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

void Cache::postReplacementProcessing(MemEvent * event, CacheAction action, bool replay) {
    Addr baseAddr = event->getBaseAddr();

    /* Got a writeback, check if waiting events can proceed now */
    if (action == DONE) activatePrevEvents(baseAddr);
    
    /* Clean up */
    delete event;
}


void Cache::postRequestProcessing(MemEvent* event) {

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
MemEvent* Cache::getOrigReq(const list<MSHREntry> entries) {
    if (entries.front().elem.isAddr()) {
        out_->fatal(CALL_INFO, -1, "%s, Error: Request at front of the mshr is not of type MemEvent. Time = %" PRIu64 "\n",
                this->getName().c_str(), getCurrentSimTimeNano());
    }

    return static_cast<MemEvent*>((entries.front().elem).getEvent());
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
void Cache::sendNACK(MemEvent* event) {
    if (event->isCPUSideEvent()) {
        coherenceMgr_->sendNACK(event, true);
    } else {
        coherenceMgr_->sendNACK(event, false);
    }
}


bool operator== ( const MSHREntry& n1, const MSHREntry& n2) {
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

