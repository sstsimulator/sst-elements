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
#include <csignal>

#include "memEvent.h"
#include <sst/core/interfaces/stringEvent.h>


using namespace SST;
using namespace SST::MemHierarchy;


void Cache::profileEvent(MemEvent* event, Command cmd, bool replay, bool canStall) {
    if (!replay) {
        switch (cmd) {
            case GetS:
                statGetS_recv->addData(1);
                break;
            case GetX:
                statGetX_recv->addData(1);
                break;
            case GetSEx:
                statGetSEx_recv->addData(1);
                break;
            case GetSResp:
                statGetSResp_recv->addData(1);
                break;
            case GetXResp:
                statGetXResp_recv->addData(1);
                break;
            case PutS:
                statPutS_recv->addData(1);
                return;
            case PutE:
                statPutE_recv->addData(1);
                return;
            case PutM:
                statPutM_recv->addData(1);
                return;
            case FetchInv:
                statFetchInv_recv->addData(1);
                return;
            case FetchInvX:
                statFetchInvX_recv->addData(1);
                return;
            case Inv:
                statInv_recv->addData(1);
                return;
            case NACK:
                statNACK_recv->addData(1);
                return;
            default:
                return;
        }
    }
    if (cmd != GetS && cmd != GetX && cmd != GetSEx) return;

    // Data request profiling only
    if (mshr_->isHit(event->getBaseAddr()) && canStall) return;   // will block this event, profile it later
    int cacheHit = isCacheHit(event, cmd, event->getBaseAddr());
    bool wasBlocked = event->blocked();                             // Event was blocked, now we're starting to handle it
    if (wasBlocked) event->setBlocked(false);
    if (cmd == GetS || cmd == GetX || cmd == GetSEx) {
        if (mshr_->isFull() || (!cf_.L1_ && !replay && mshr_->isAlmostFull()  && !(cacheHit == 0))) { 
                return; // profile later, this event is getting NACKed 
        }
    }

    switch(cmd) {
        case GetS:
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
        case GetX:
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
        case GetSEx:
            if (!replay) {                // New event
                if (cacheHit == 0) {
                    statCacheHits->addData(1);
                    statGetSExHitOnArrival->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSExMissOnArrival->addData(1);
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
                    statGetSExMissAfterBlocked->addData(1);
                } else {
                    statCacheMisses->addData(1);
                    statGetSExMissAfterBlocked->addData(1);
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


bool Cache::processEvent(MemEvent* event, bool replay) {
    Command cmd     = event->getCmd();
    if (cf_.L1_) event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
    bool noncacheable   = event->queryFlag(MemEvent::F_NONCACHEABLE) || cf_.allNoncacheableRequests_;
    MemEvent* origEvent;
    
    /* Set requestor field if this is the first cache that's seen this event */
    if (event->getRqstr() == "None") { event->setRqstr(this->getName()); }

    
    if (!replay) { 
        statTotalEventsReceived->addData(1);
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d2_->debug(_L3_,"\n\n-------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); 
        cout << flush;
#endif
    }
    else statTotalEventsReplayed->addData(1);

#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) {
        if (replay) {
            d_->debug(_L3_,"Replay. Name: %s, Cmd: %s, BsAddr: %" PRIx64 ", Addr: %" PRIx64 ", VAddr: %" PRIx64 ", iPtr: %" PRIx64 ", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Bytes requested = %u, cycles: %" PRIu64 ", %s\n",
                       this->getName().c_str(), CommandString[event->getCmd()], baseAddr, event->getAddr(), event->getVirtualAddress(), event->getInstructionPointer(), event->getRqstr().c_str(), 
                       event->getSrc().c_str(), event->getDst().c_str(), event->isPrefetch() ? "true" : "false", event->getSize(), timestamp_, noncacheable ? "noncacheable" : "cacheable");
        } else {
            d_->debug(_L3_,"New Event. Name: %s, Cmd: %s, BsAddr: %" PRIx64 ", Addr: %" PRIx64 ", VAddr: %" PRIx64 ", iPtr: %" PRIx64 ", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Bytes requested = %u, cycles: %" PRIu64 ", %s\n",
                       this->getName().c_str(), CommandString[event->getCmd()], baseAddr, event->getAddr(), event->getVirtualAddress(), event->getInstructionPointer(), event->getRqstr().c_str(), 
                       event->getSrc().c_str(), event->getDst().c_str(), event->isPrefetch() ? "true" : "false", event->getSize(), timestamp_, noncacheable ? "noncacheable" : "cacheable");
        }
    }
    cout << flush; 
#endif

    if (noncacheable || cf_.allNoncacheableRequests_) {
        processNoncacheable(event, cmd, baseAddr);
        return true;
    }

    // Cannot stall if this is a GetX to L1 and the line is locked because GetX is the unlock!
    bool canStall = !cf_.L1_ || event->getCmd() != GetX;
    if (!canStall) {
        CacheLine * cacheLine = cf_.cacheArray_->lookup(baseAddr, false);
        canStall = cacheLine == nullptr || !(cacheLine->isLocked());
    }

    profileEvent(event, cmd, replay, canStall);

    switch(cmd) {
        case GetS:
        case GetX:
        case GetSEx:
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
        case GetSResp:
        case GetXResp:
        case FlushLineResp:
            processCacheResponse(event, baseAddr);
            break;
        case PutS:
        case PutM:
        case PutE:
            processCacheReplacement(event, cmd, baseAddr, replay);
            break;
        case NACK:
            origEvent = event->getNACKedEvent();
            processIncomingNACK(origEvent);
            delete event;
            break;
        case FetchInv:
        case Fetch:
        case FetchInvX:
        case Inv:
            processCacheInvalidate(event, baseAddr, replay);
            break;
        case AckPut:
        case AckInv:
        case FetchResp:
        case FetchXResp:
            processFetchResp(event, baseAddr);
            break;
        case FlushLine:
        case FlushLineInv:
            processCacheFlush(event, baseAddr, replay);
            break;
        default:
            d_->fatal(CALL_INFO, -1, "Command not supported, cmd = %s", CommandString[cmd]);
    }
    return true;
}

void Cache::processNoncacheable(MemEvent* event, Command cmd, Addr baseAddr) {
    MemEvent* origRequest;
    bool inserted;
    event->setFlag(MemEvent::F_NONCACHEABLE);
    
    switch(cmd) {
        case GetS:
        case GetX:
        case GetSEx:
        case FlushLine:
        case FlushLineInv:  // Note that noncacheable flushes currently ignore the cache - they just flush any buffers at memory
#ifdef __SST_DEBUG_OUTPUT__
	    if (cmd == GetSEx) d_->debug(_WARNING_, "WARNING: Noncachable atomics have undefined behavior; atomicity not preserved\n"); 
#endif
            inserted = mshrNoncacheable_->insert(baseAddr, event);
            if (!inserted) {
                d_->fatal(CALL_INFO, -1, "%s, Error inserting noncacheable request in mshr. Cmd = %s, Addr = 0x%" PRIx64 ", Time = %" PRIu64 "\n",getName().c_str(), CommandString[cmd], baseAddr, getCurrentSimTimeNano());
            }
            if (cmd == GetS) coherenceMgr_->forwardMessage(event, baseAddr, event->getSize(), 0, NULL);
            else             coherenceMgr_->forwardMessage(event, baseAddr, event->getSize(), 0, &event->getPayload());
            break;
        case GetSResp:
        case GetXResp:
            origRequest = mshrNoncacheable_->removeFront(baseAddr);
            if (origRequest->getID().first != event->getResponseToID().first || origRequest->getID().second != event->getResponseToID().second) {
                d_->fatal(CALL_INFO, -1, "%s, Error: noncacheable response received does not match request at front of mshr. Resp cmd = %s, Resp addr = 0x%" PRIx64 ", Req cmd = %s, Req addr = 0x%" PRIx64 ", Time = %" PRIu64 "\n",
                        getName().c_str(),CommandString[cmd],baseAddr, CommandString[origRequest->getCmd()], origRequest->getBaseAddr(),getCurrentSimTimeNano());
            }
            coherenceMgr_->sendResponseUp(origRequest, NULLST, &event->getPayload(), true, 0);
            delete origRequest;
            delete event;
            break;
        case FlushLineResp: {
            // Flushes can be returned out of order since they don't neccessarily require a memory access so we need to actually search the MSHRs
            vector<mshrType> * entries = mshrNoncacheable_->getAll(baseAddr);
            for (vector<mshrType>::iterator it = entries->begin(); it != entries->end(); it++) {
                MemEvent * candidate = (it->elem).getEvent();
                if (candidate->getCmd() == FlushLine || candidate->getCmd() == FlushLineInv) { // All entries are events so no checking for pointer vs event needed
                    if (candidate->getID().first == event->getResponseToID().first && candidate->getID().second == event->getResponseToID().second) {
                        origRequest = candidate;
                        break;
                    }
                }
            }
            if (origRequest == nullptr) {
                d_->fatal(CALL_INFO, -1, "%s, Error: noncacheable response received does not match any request in the mshr. Resp cmd = %s, Resp addr = 0x%" PRIx64 ", Req cmd = %s, Req addr = 0x%" PRIx64 ", Time = %" PRIu64 "\n",
                        getName().c_str(),CommandString[cmd],baseAddr, CommandString[origRequest->getCmd()], origRequest->getBaseAddr(),getCurrentSimTimeNano());
            }
            coherenceMgr_->sendResponseUp(origRequest, NULLST, &event->getPayload(), true, 0);
            mshrNoncacheable_->removeElement(baseAddr, origRequest);
            delete origRequest;
            delete event;
            break;
            }
        default:
            d_->fatal(CALL_INFO, -1, "Command does not exist. Command: %s, Src: %s\n", CommandString[cmd], event->getSrc().c_str());
    }
}


void Cache::handlePrefetchEvent(SST::Event* ev) {
    prefetchLink_->send(1, ev);
}

/* Handler for self events, namely prefetches */
void Cache::processPrefetchEvent(SST::Event* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    

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
        if (event->getCmd() != NULLCMD && mshr_->getSize() < dropPrefetchLevel_ && mshr_->getPrefetchCount() < maxOutstandingPrefetch_) { 
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
    // See if we can determine whether the lower entity is non-inclusive
    // For direct-connect cache, 
    if (topNetworkLink_) { // I'm connected to the network ONLY via a single NIC
        bottomNetworkLink_->init(phase);
            
        /*  */
        while(MemEvent *event = bottomNetworkLink_->recvInitData()) {
            delete event;
        }
        return;
    }
    
    SST::Event *ev;
    if (bottomNetworkLink_) {
        bottomNetworkLink_->init(phase);
    }
    
    if (!phase) {
        if (cf_.L1_) {
            highNetPort_->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
        } else {
            if (cf_.type_ == "inclusive") {
                highNetPort_->sendInitData(new MemEvent(this, 0, 0, NULLCMD));
            } else {
                highNetPort_->sendInitData(new MemEvent(this, 1, 0, NULLCMD));
            }
        }
        if (!bottomNetworkLink_) {
            lowNetPort_->sendInitData(new MemEvent(this, 10, 10, NULLCMD));
        }
        
    }

    while ((ev = highNetPort_->recvInitData())) {
        MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
        if (!memEvent) { /* Do nothing */ }
        else if (memEvent->getCmd() == NULLCMD) {
            if (memEvent->getCmd() == NULLCMD) {    // Save upper level cache names
                coherenceMgr_->addUpperLevelCacheName(memEvent->getSrc());
                upperLevelCacheNames_.push_back(memEvent->getSrc());
            }
        } else {
            if (bottomNetworkLink_) {
                bottomNetworkLink_->sendInitData(new MemEvent(*memEvent));
            } else {
                lowNetPort_->sendInitData(new MemEvent(*memEvent));
            }
        }
        delete memEvent;
     }
    
    if (!bottomNetworkLink_) {  // Save names of caches below us
        while ((ev = lowNetPort_->recvInitData())) {
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if (memEvent && memEvent->getCmd() == NULLCMD) {
                if (memEvent->getBaseAddr() == 0) {
                    isLL = false;
                    if (memEvent->getAddr() == 1) {
                        lowerIsNoninclusive = true; // TODO better checking if we have multiple caches below us
                    }
                }
                coherenceMgr_->addLowerLevelCacheName(memEvent->getSrc());
                lowerLevelCacheNames_.push_back(memEvent->getSrc());
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
    MemEvent* event = static_cast<MemEvent*>(ev);
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
        if ((event->getCmd() == GetS || event->getCmd() == GetX) && !requestBuffer_.empty()) {
            requestBuffer_.push(event); // Force ordering on requests -> really only need @ L1s
        } else {
            requestsThisCycle_++;
            if (!processEvent(event, false)) {
                requestBuffer_.push(event);   
            }
        }
    }
}
