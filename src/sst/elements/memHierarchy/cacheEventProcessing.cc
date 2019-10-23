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
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <sst/core/interfaces/stringEvent.h>

#include "cacheController.h"
#include "coherencemgr/coherenceController.h"
#include "hash.h"

#include "memEvent.h"
#include "memEventBase.h"


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


void Cache::profileEvent(MemEvent* event, Command cmd, bool replay, bool canStall) {
    if (cmd != Command::GetS && cmd != Command::GetX && cmd != Command::GetSX) return;

    // Data request profiling only
    if (mshr_->isHit(event->getBaseAddr()) && canStall) return;   // will block this event, profile it later
    bool cacheHit = coherenceMgr_->isCacheHit(event);
    bool wasBlocked = event->blocked();                             // Event was blocked, now we're starting to handle it
    if (wasBlocked) event->setBlocked(false);
    if (cmd == Command::GetS || cmd == Command::GetX || cmd == Command::GetSX) {
        if (mshr_->isFull() || (!L1_ && !replay && mshr_->isAlmostFull()  && !cacheHit)) { 
            return; // profile later, this event is getting NACKed 
        }
    }

    switch(cmd) {
        case Command::GetS:
            if (!replay) {                // New event
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetSHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSMissOnArrival->addData(1);
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetSHitAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSMissAfterBlocked->addData(1);
                }
            }
            break;
        case Command::GetX:
            if (!replay) {                // New event
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetXHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetXMissOnArrival->addData(1);
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetXHitAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetXMissAfterBlocked->addData(1);
                }
            }
            break;
        case Command::GetSX:
            if (!replay) {                // New event
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetSXHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSXMissOnArrival->addData(1);
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit) {
                    statCacheHits->addData(1);
                    statGetSXMissAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSXMissAfterBlocked->addData(1);
                }
            }
            break;
        default:
            break;
    }
}


bool Cache::processEvent(MemEventBase* ev, bool replay) {
    
    // Debug
    if (is_debug_event(ev)) {
        if (replay) {
            d_->debug(_L3_, "Replay. Name: %s. Cycles: %" PRIu64 ". Event: (%s)\n", 
                    getName().c_str(), 
                    timestamp_, 
                    ev->getVerboseString().c_str());
        } else {
            d2_->debug(_L3_,"\n\n-------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); 
            d_->debug(_L3_,"New Event. Name: %s. Cycles: %" PRIu64 ", Event: (%s)\n", this->getName().c_str(), timestamp_, ev->getVerboseString().c_str());
        }
        cout << flush;
    }

    // Set noncacheable requests if needed
    if (allNoncacheableRequests_) {
        ev->setFlag(MemEvent::F_NONCACHEABLE);
    }
    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);


    if (MemEventTypeArr[(int)ev->getCmd()] != MemEventType::Cache || noncacheable) {
        statNoncacheableEventsReceived->addData(1);
        processNoncacheable(ev);
        return true;
    }

    /* Start handling cache events */
    MemEvent * event = static_cast<MemEvent*>(ev);
    Command cmd     = event->getCmd();
    std::string gpu ("gpu0");
    if((event->getSrc().compare(gpu))==0){
        if(event->getAddr() % cacheArray_->getLineSize()==0){
            event->setBaseAddr(event->getAddr());
        } else {
            event->setBaseAddr(event->getAddr()-(event->getAddr() % cacheArray_->getLineSize()));
        }
    }
    Addr baseAddr   = event->getBaseAddr();
    // TODO this is a temporary check while we ensure that the source sets baseAddr correctly
    if (baseAddr % cacheArray_->getLineSize() != 0) {
        out_->fatal(CALL_INFO, -1, "%s, Base address is not a multiple of line size! Line size: %" PRIu64 ". Event: %s\n", getName().c_str(), cacheArray_->getLineSize(), ev->getVerboseString().c_str());
    }
    
    // Check bank free before we do anything
    if (bankStatus_.size() > 0) {
        Addr bank = cacheArray_->getBank(event->getBaseAddr());
        if (bankStatus_[bank]) { // bank conflict
            if (is_debug_event(event))
            d_->debug(_L3_, "Bank conflict on bank %" PRIu64 "\n", bank);
            statBankConflicts->addData(1);
            bankConflictBuffer_[bank].push(event);
            return true; // Accepted but we're stopping now
        } else {
            d_->debug(_L3_, "No bank conflict, setting bank %" PRIu64 " to busy\n", bank);
            bankStatus_[bank] = true;
        }
    }
    
    MemEvent* origEvent;
    
    // Update statistics
    if (!replay) statTotalEventsReceived->addData(1);
    else statTotalEventsReplayed->addData(1);
    
    // Cannot stall if this is a GetX to L1 and the line is locked because GetX is the unlock!
    bool canStall = !L1_ || event->getCmd() != Command::GetX;
    if (!canStall) {
        CacheLine * cacheLine = cacheArray_->lookup(baseAddr, false);
        canStall = cacheLine == nullptr || !(cacheLine->isLocked());
    }

    switch(cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            // Determine if request should be NACKed: Request cannot be handled immediately and there are no free MSHRs to buffer the request
            if (!replay && mshr_->isAlmostFull()) { 
                // Requests can cause deadlock because requests and fwd requests (inv, fetch, etc) share mshrs -> always leave one mshr free for fwd requests
                if (!L1_) {
                    stat_eventRecv[(int)cmd]->addData(1);
                    profileEvent(event, cmd, replay, canStall);
                    sendNACK(event);
                    break;
                } else if (canStall) {
                    d_->debug(_L6_,"Stalling request...MSHR almost full\n");
                    return false;
                }
            }
            
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            profileEvent(event, cmd, replay, canStall);
            
            if (mshr_->isHit(baseAddr) && canStall) {
                // Drop local prefetches if there are outstanding requests for the same address NOTE this includes replacements/inv/etc.
                if (event->isPrefetch() && event->getRqstr() == this->getName()) {
                    if (is_debug_addr(baseAddr))
                        d_->debug(_L6_, "Drop prefetch: cache hit\n");
                    statPrefetchDrop->addData(1);
                    coherenceMgr_->removeRequestRecord(event->getID());
                    delete event;
                    break;
                }
                if (processRequestInMSHR(baseAddr, event)) {
                    if (is_debug_addr(baseAddr)) 
                        d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                    
                    event->setBlocked(true);
                }
                break;
            }
            
            processCacheRequest(event, cmd, baseAddr, replay);
            break;
        case Command::GetSResp:
        case Command::GetXResp:
        case Command::FlushLineResp:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            processCacheResponse(event, baseAddr);
            break;
        case Command::PutS:
        case Command::PutM:
        case Command::PutE:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            processCacheReplacement(event, cmd, baseAddr, replay);
            break;
        case Command::NACK:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            coherenceMgr_->handleNACK(event, replay);
            delete event;
            break;
        case Command::FetchInv:
        case Command::Fetch:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            if (coherenceMgr_->handleInvalidationRequest(event, replay) == DONE)
                activatePrevEvents(baseAddr);
            break;
        case Command::AckPut:
        case Command::AckInv:
        case Command::FetchResp:
        case Command::FetchXResp:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            if (coherenceMgr_->handleFetchResponse(event, true) == DONE) {
                activatePrevEvents(baseAddr);
            }
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            if (!replay) {
                stat_eventRecv[(int)cmd]->addData(1);
            }
            processCacheFlush(event, baseAddr, replay);
            break;
        default:
            out_->fatal(CALL_INFO, -1, "%s, Command not supported. Time = %" PRIu64 "ns, Event = %s", getName().c_str(), getCurrentSimTimeNano(), event->getVerboseString().c_str());
    }
    return true;
}

/* For handling non-cache commands (including NONCACHEABLE data requests) */
void Cache::processNoncacheable(MemEventBase* event) {
    if (CommandCPUSide[(int)event->getCmd()]) {
        if (!(event->queryFlag(MemEvent::F_NORESPONSE))) {
            responseDst_.insert(std::make_pair(event->getID(), event->getSrc()));
        }
        coherenceMgr_->forwardTowardsMem(event);
    } else {
        std::map<SST::Event::id_type,std::string>::iterator it = responseDst_.find(event->getResponseToID());
        if (it == responseDst_.end()) {
            out_->fatal(CALL_INFO, 01, "%s, Error: noncacheable response received does not match a request. Event: (%s). Time: %" PRIu64 "\n",
                    getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
        coherenceMgr_->forwardTowardsCPU(event, it->second);
        responseDst_.erase(it);
    }
}


