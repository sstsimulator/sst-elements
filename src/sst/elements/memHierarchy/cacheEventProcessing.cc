// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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
#include "cacheController.h"
#include "coherenceController.h"
#include "hash.h"

#include "memEvent.h"
#include "memEventBase.h"
#include <sst/core/interfaces/stringEvent.h>


using namespace SST;
using namespace SST::MemHierarchy;


void Cache::profileEvent(MemEvent* event, Command cmd, bool replay, bool canStall) {
    if (!replay) {
        switch (cmd) {
            case Command::GetS:
                statGetS_recv->addData(1);
                break;
            case Command::GetX:
                statGetX_recv->addData(1);
                break;
            case Command::GetSX:
                statGetSX_recv->addData(1);
                break;
            case Command::GetSResp:
                statGetSResp_recv->addData(1);
                break;
            case Command::GetXResp:
                statGetXResp_recv->addData(1);
                break;
            case Command::PutS:
                statPutS_recv->addData(1);
                return;
            case Command::PutE:
                statPutE_recv->addData(1);
                return;
            case Command::PutM:
                statPutM_recv->addData(1);
                return;
            case Command::FetchInv:
                statFetchInv_recv->addData(1);
                return;
            case Command::FetchInvX:
                statFetchInvX_recv->addData(1);
                return;
            case Command::Inv:
                statInv_recv->addData(1);
                return;
            case Command::NACK:
                statNACK_recv->addData(1);
                return;
            default:
                return;
        }
    }
    if (cmd != Command::GetS && cmd != Command::GetX && cmd != Command::GetSX) return;

    // Data request profiling only
    if (mshr_->isHit(event->getBaseAddr()) && canStall) return;   // will block this event, profile it later
    int cacheHit = isCacheHit(event, cmd, event->getBaseAddr());
    bool wasBlocked = event->blocked();                             // Event was blocked, now we're starting to handle it
    if (wasBlocked) event->setBlocked(false);
    if (cmd == Command::GetS || cmd == Command::GetX || cmd == Command::GetSX) {
        if (mshr_->isFull() || (!cf_.L1_ && !replay && mshr_->isAlmostFull()  && !(cacheHit == 0))) { 
                return; // profile later, this event is getting NACKed 
        }
    }

    switch(cmd) {
        case Command::GetS:
            if (!replay) {                // New event
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetSHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSMissOnArrival->addData(1);
                    if (cacheHit == 1 || cacheHit == 2) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 0));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 1));
                    }
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetSHitAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSMissAfterBlocked->addData(1);
                    if (cacheHit == 1 || cacheHit == 2) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 0));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 1));
                    }
                }
            }
            break;
        case Command::GetX:
            if (!replay) {                // New event
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetXHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetXMissOnArrival->addData(1);
                    if (cacheHit == 1) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 2));
                    } else if (cacheHit == 2) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 3));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 4));
                    }
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetXHitAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetXMissAfterBlocked->addData(1);
                    if (cacheHit == 1) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 2));
                    } else if (cacheHit == 2) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 3));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 4));
                    }
                }
            }
            break;
        case Command::GetSX:
            if (!replay) {                // New event
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetSXHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSXMissOnArrival->addData(1);
                    if (cacheHit == 1) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 5));
                    } else if (cacheHit == 2) { 
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 6));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 7));
                    }
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetSXMissAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSXMissAfterBlocked->addData(1);
                    if (cacheHit == 1) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 5));
                    } else if (cacheHit == 2) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 6));
                    } else if (cacheHit == 3) {
                        missTypeList.insert(std::pair<MemEvent*,int>(event, 7));
                    }
                }
            }
            break;
        default:
            break;
    }
}


bool Cache::processEvent(MemEventBase* ev, bool replay) {
    
    
    // Debug
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || ev->doDebug(DEBUG_ADDR)) {
        if (replay) {
            d_->debug(_L3_, "Replay. Name: %s. Cycles: %" PRIu64 ". Event: (%s)\n", this->getName().c_str(), timestamp_, ev->getVerboseString().c_str());
        } else {
            d2_->debug(_L3_,"\n\n-------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); 
            d_->debug(_L3_,"New Event. Name: %s. Cycles: %" PRIu64 ", Event: (%s)\n", this->getName().c_str(), timestamp_, ev->getVerboseString().c_str());
        }
        cout << flush;
    }
#endif

    // Set noncacheable requests if needed
    if (cf_.allNoncacheableRequests_) {
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
    Addr baseAddr   = event->getBaseAddr();
    
    // TODO this is a temporary check while we ensure that the source sets baseAddr correctly
    if (baseAddr % cf_.cacheArray_->getLineSize() != 0) {
        d_->fatal(CALL_INFO, -1, "Base address is not a multiple of line size!\n");
    }
    
    MemEvent* origEvent;
    
    // Update statistics
    if (!replay) statTotalEventsReceived->addData(1);
    else statTotalEventsReplayed->addData(1);
    
    // Cannot stall if this is a GetX to L1 and the line is locked because GetX is the unlock!
    bool canStall = !cf_.L1_ || event->getCmd() != Command::GetX;
    if (!canStall) {
        CacheLine * cacheLine = cf_.cacheArray_->lookup(baseAddr, false);
        canStall = cacheLine == nullptr || !(cacheLine->isLocked());
    }

    profileEvent(event, cmd, replay, canStall);

    switch(cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            // Determine if request should be NACKed: Request cannot be handled immediately and there are no free MSHRs to buffer the request
            if (!replay && mshr_->isAlmostFull()) { 
                // Requests can cause deadlock because requests and fwd requests (inv, fetch, etc) share mshrs -> always leave one mshr free for fwd requests
                if (!cf_.L1_) {
                    sendNACK(event);
                    break;
                } else if (canStall) {
                    d_->debug(_L6_,"Stalling request...MSHR almost full\n");
                    return false;
                }
            }
            
            if (mshr_->isHit(baseAddr) && canStall) {
                // Drop local prefetches if there are outstanding requests for the same address NOTE this includes replacements/inv/etc.
                if (event->isPrefetch() && event->getRqstr() == this->getName()) {
                    statPrefetchDrop->addData(1);
                    delete event;
                    break;
                }
                if (processRequestInMSHR(baseAddr, event)) {
#ifdef __SST_DEBUG_OUTPUT__
                    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
#endif
                    event->setBlocked(true);
                }
                // track times in our separate queue
                if (startTimeList.find(event) == startTimeList.end()) {
                    startTimeList.insert(std::pair<MemEvent*,uint64>(event, timestamp_));
                }

                break;
            }
            
            // track times in our separate queue
            if (startTimeList.find(event) == startTimeList.end()) {
                startTimeList.insert(std::pair<MemEvent*,uint64>(event, timestamp_));
            }
            
            processCacheRequest(event, cmd, baseAddr, replay);
            break;
        case Command::GetSResp:
        case Command::GetXResp:
        case Command::FlushLineResp:
            processCacheResponse(event, baseAddr);
            break;
        case Command::PutS:
        case Command::PutM:
        case Command::PutE:
            processCacheReplacement(event, cmd, baseAddr, replay);
            break;
        case Command::NACK:
            origEvent = event->getNACKedEvent();
            processIncomingNACK(origEvent);
            delete event;
            break;
        case Command::FetchInv:
        case Command::Fetch:
        case Command::FetchInvX:
        case Command::Inv:
            processCacheInvalidate(event, baseAddr, replay);
            break;
        case Command::AckPut:
        case Command::AckInv:
        case Command::FetchResp:
        case Command::FetchXResp:
            processFetchResp(event, baseAddr);
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            processCacheFlush(event, baseAddr, replay);
            break;
        default:
            d_->fatal(CALL_INFO, -1, "Command not supported, cmd = %s", CommandString[(int)cmd]);
    }
    return true;
}

/* For handling non-cache commands (including NONCACHEABLE data requests) */
void Cache::processNoncacheable(MemEventBase* event) {
    if (CommandCPUSide[(int)event->getCmd()]) {
        responseDst_.insert(std::make_pair(event->getID(), event->getSrc()));
        coherenceMgr_->forwardTowardsMem(event);
    } else {
        std::map<SST::Event::id_type,std::string>::iterator it = responseDst_.find(event->getResponseToID());
        if (it == responseDst_.end()) {
            d_->fatal(CALL_INFO, 01, "%s, Error: noncacheable response received does not match a request. Event: (). Time: %" PRIu64 "\n",
                    getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
        }
        coherenceMgr_->forwardTowardsCPU(event, it->second);
        responseDst_.erase(it);
    }
}


void Cache::handlePrefetchEvent(SST::Event* ev) {
    prefetchLink_->send(1, ev);
}

/* Handler for self events, namely prefetches */
void Cache::processPrefetchEvent(SST::Event* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    event->setRqstr(this->getName());

    if (!clockIsOn_) {
        Cycle_t time = reregisterClock(defaultTimeBase_, clockHandler_); 
        timestamp_ = time - 1;
        coherenceMgr_->updateTimestamp(timestamp_);
        int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
        for (int64_t i = 0; i < cyclesOff; i++) {           // TODO more efficient way to do this? Don't want to add in one-shot or we get weird averages/sum sq.
            statMSHROccupancy->addData(mshr_->getSize());
        }
        //d_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), time, timestamp_, getCurrentSimTimeNano());
        clockIsOn_ = true;
    }

    // Record received prefetch
    statPrefetchRequest->addData(1);

    // Drop prefetch if we can't handle it immediately or handling it would violate maxOustandingPrefetch or dropPrefetchLevel
    if (requestsThisCycle_ != maxRequestsPerCycle_) {
        if (event->getCmd() != Command::NULLCMD && mshr_->getSize() < dropPrefetchLevel_ && mshr_->getPrefetchCount() < maxOutstandingPrefetch_) { 
            requestsThisCycle_++;
            processEvent(event, false);
        } else {
            statPrefetchDrop->addData(1);
            delete event;
        }
    } else {
        statPrefetchDrop->addData(1);
        delete event;
    }
}



void Cache::init(unsigned int phase) {
    if (topNetworkLink_) { // I'm connected to the network ONLY via a single NIC
        
        bottomNetworkLink_->init(phase);
        
        if (!phase)
            bottomNetworkLink_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Cache, cf_.type_ == "inclusive", cf_.cacheArray_->getLineSize()));

        /*  */
        while(MemEventInit *event = bottomNetworkLink_->recvInitData()) {
            if (event->getCmd() == Command::NULLCMD) {
                d_->debug(_L10_, "%s received init event with cmd: %s, src: %s, type: %d, inclusive: %d, line size: %" PRIu64 "\n", 
                        this->getName().c_str(), CommandString[(int)event->getCmd()], event->getSrc().c_str(), (int)event->getType(), event->getInclusive(), event->getLineSize());
            }
            delete event;
        }
        return;
    }
    
    SST::Event *ev;
    if (bottomNetworkLink_) {
        bottomNetworkLink_->init(phase);
    }
    
    if (!phase) {
        highNetPort_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Cache, cf_.type_ == "inclusive", cf_.cacheArray_->getLineSize()));
        
        if (!bottomNetworkLink_) { // If we do have a bottom network link, the nic takes care of this
            lowNetPort_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Cache, cf_.type_ == "inclusive", cf_.cacheArray_->getLineSize()));
        } else {
            bottomNetworkLink_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Cache, cf_.type_ == "inclusive", cf_.cacheArray_->getLineSize()));
        }
    }

    while ((ev = highNetPort_->recvInitData())) {
        MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);
        if (!memEvent) { /* Do nothing */ }
        else if (memEvent->getCmd() == Command::NULLCMD) {
            d_->debug(_L10_, "%s received init event with cmd: %s, src: %s, type: %d, inclusive: %d\n", 
                    this->getName().c_str(), CommandString[(int)memEvent->getCmd()], memEvent->getSrc().c_str(), (int)memEvent->getType(), memEvent->getInclusive());
            coherenceMgr_->addUpperLevelCacheName(memEvent->getSrc());
            upperLevelCacheNames_.push_back(memEvent->getSrc());
        } else {
            d_->debug(_L10_, "%s received init event with cmd: %s, src: %s, addr: %" PRIu64 ", payload size: %zu\n", 
                    this->getName().c_str(), CommandString[(int)memEvent->getCmd()], memEvent->getSrc().c_str(), memEvent->getAddr(), memEvent->getPayload().size());
        
            if (bottomNetworkLink_) {
                bottomNetworkLink_->sendInitData(new MemEventInit(*memEvent));
            } else {
                lowNetPort_->sendInitData(new MemEventInit(*memEvent));
            }
        }
        delete memEvent;
     }
    
    if (!bottomNetworkLink_) {  // Save names of caches below us
        while ((ev = lowNetPort_->recvInitData())) {
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);
            if (memEvent && memEvent->getCmd() == Command::NULLCMD) {
                d_->debug(_L10_, "%s received init event with cmd: %s, src: %s, type: %d, inclusive: %d\n", 
                        this->getName().c_str(), CommandString[(int)memEvent->getCmd()], memEvent->getSrc().c_str(), (int)memEvent->getType(), memEvent->getInclusive());
                if (memEvent->getType() == Endpoint::Cache) {
                    isLL = false;
                    if (!memEvent->getInclusive()) {
                        lowerIsNoninclusive = true; // TODO better checking if we have multiple caches below us
                    }
                }
                coherenceMgr_->addLowerLevelCacheName(memEvent->getSrc());
                lowerLevelCacheNames_.push_back(memEvent->getSrc());
            }
            delete memEvent;
        }
    } else {
        /*
         *  Network attached caches
         *  Note that this finds ALL endpoints on the network, not just those the cache might directly communicate with
         *  Current MemNIC does all the tracking of which endpoints we actually need to know about so this is just for debug
         */
        while (MemEventInit *memEvent = bottomNetworkLink_->recvInitData()) {
            if (memEvent->getCmd() == Command::NULLCMD) {
                d_->debug(_L10_, "%s received init event with cmd: %s, src: %s, type: %d, inclusive: %d\n", 
                        this->getName().c_str(), CommandString[(int)memEvent->getCmd()], memEvent->getSrc().c_str(), (int)memEvent->getType(), memEvent->getInclusive());
            }
            delete memEvent;
        }
    }
}


void Cache::setup() {
    bool isDirBelow = false; // is a directory below?
    if (bottomNetworkLink_) { 
        isLL = false;   // Either a directory or a cache below us
        lowerIsNoninclusive = false; // Assume the cache below us is inclusive or it's a directory
        isDirBelow = true; // Assume a directory is below
        const std::vector<MemNIC::PeerInfo_t> &ci = bottomNetworkLink_->getPeerInfo();
        const MemNIC::ComponentInfo &myCI = bottomNetworkLink_->getComponentInfo();
        // Search peer info to determine if we have inclusive or noninclusive caches below us
        if (MemNIC::TypeCacheToCache == myCI.type) { // I'm a cache with a cache below
            isDirBelow = false; // Cache not directory below us
            for (std::vector<MemNIC::PeerInfo_t>::const_iterator i = ci.begin() ; i != ci.end() ; ++i) {
                if (MemNIC::TypeNetworkCache == i->first.type) { // This would be any cache that is 'below' us
                    if (i->second.cacheType != "inclusive") {
                        lowerIsNoninclusive = true;
                    }
                }
            }
        }
    }
    coherenceMgr_->setupLowerStatus(isLL && !bottomNetworkLink_, lowerIsNoninclusive, isDirBelow);
}


void Cache::finish() {
    listener_->printStats(*d_);
    delete cf_.cacheArray_;
    delete d_;
    linkIdMap_.clear();
    nameMap_.clear();
}

/* Main handler for links to upper and lower caches/cores/buses/etc */
void Cache::processIncomingEvent(SST::Event* ev) {
    MemEventBase* event = static_cast<MemEventBase*>(ev);
    if (!clockIsOn_) {
        Cycle_t time = reregisterClock(defaultTimeBase_, clockHandler_); 
        timestamp_ = time - 1;
        coherenceMgr_->updateTimestamp(timestamp_);
        int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
        for (int64_t i = 0; i < cyclesOff; i++) {           // TODO more efficient way to do this? Don't want to add in one-shot or we get weird averages/sum sq.
            statMSHROccupancy->addData(mshr_->getSize());
        }
        //d_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), time, timestamp_, getCurrentSimTimeNano());
        clockIsOn_ = true;
    }
    if (requestsThisCycle_ == maxRequestsPerCycle_) {
        requestBuffer_.push(event);
    } else {
        if ((event->getCmd() == Command::GetS || event->getCmd() == Command::GetX) && !requestBuffer_.empty()) {
            requestBuffer_.push(event); // Force ordering on requests -> really only need @ L1s
        } else {
            requestsThisCycle_++;
            if (!processEvent(event, false)) {
                requestBuffer_.push(event);   
            }
        }
    }
}
