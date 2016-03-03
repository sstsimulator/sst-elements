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

/*
 * File:   cache.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "cacheController.h"
#include "coherenceControllers.h"
#include "hash.h"
#include <boost/lexical_cast.hpp>
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


void Cache::processEvent(MemEvent* event, bool replay) {
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
        return;
    }

    // Cannot stall if this is a GetX to L1 and the line is locked because GetX is the unlock!
    bool canStall = !cf_.L1_ || event->getCmd() != GetX;
    if (!canStall) {
        CacheLine * cacheLine = NULL;
        int lineIndex = cf_.cacheArray_->find(baseAddr, false);
        if (lineIndex != -1) cacheLine = cf_.cacheArray_->lines_[lineIndex];
        canStall = cacheLine == NULL || !(cacheLine->isLocked());
    }

    profileEvent(event, cmd, replay, canStall);

    switch(cmd) {
        case GetS:
        case GetX:
        case GetSEx:
            // Determine if request should be NACKed: Request cannot be handled immediately and there are no free MSHRs to buffer the request
            if (!replay && mshr_->isAlmostFull()) { 
                // Requests can cause deadlock because requests and fwd requests (inv, fetch, etc) share mshrs -> always leave one mshr free for fwd requests
                sendNACK(event);
                break;
            }
            
            // track times in our separate queue
            if (startTimeList.find(event) == startTimeList.end()) startTimeList.insert(std::pair<MemEvent*,uint64>(event, timestamp_));

            if (mshr_->isHit(baseAddr) && canStall) {
                if (processRequestInMSHR(baseAddr, event)) {
#ifdef __SST_DEBUG_OUTPUT__
                    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
#endif
                    event->setBlocked(true);
                }
                break;
            }
            
            processCacheRequest(event, cmd, baseAddr, replay);
            break;
        case GetSResp:
        case GetXResp:
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
        default:
            d_->fatal(CALL_INFO, -1, "Command not supported, cmd = %s", CommandString[cmd]);
    }
}

void Cache::processNoncacheable(MemEvent* event, Command cmd, Addr baseAddr) {
    MemEvent* origRequest;
    bool inserted;
    event->setFlag(MemEvent::F_NONCACHEABLE);
    
    switch(cmd) {
        case GetS:
        case GetX:
        case GetSEx:
#ifdef __SST_DEBUG_OUTPUT__
	    if (cmd == GetSEx) d_->debug(_WARNING_, "WARNING: Noncachable atomics have undefined behavior; atomicity not preserved\n"); 
#endif
            inserted = mshrNoncacheable_->insert(baseAddr, event);
            if (!inserted) {
                d_->fatal(CALL_INFO, -1, "%s, Error inserting noncacheable request in mshr. Cmd = %s, Addr = 0x%" PRIx64 ", Time = %" PRIu64 "\n",getName().c_str(), CommandString[cmd], baseAddr, getCurrentSimTimeNano());
            }
            if (cmd == GetS) coherenceMgr->forwardMessage(event, baseAddr, event->getSize(), 0, NULL);
            else             coherenceMgr->forwardMessage(event, baseAddr, event->getSize(), 0, &event->getPayload());
            break;
        case GetSResp:
        case GetXResp:
            origRequest = mshrNoncacheable_->removeFront(baseAddr);
            if (origRequest->getID().second != event->getResponseToID().second) {
                d_->fatal(CALL_INFO, -1, "%s, Error: noncacheable response received does not match request at front of mshr. Resp cmd = %s, Resp addr = 0x%" PRIx64 ", Req cmd = %s, Req addr = 0x%" PRIx64 ", Time = %" PRIu64 "\n",
                        getName().c_str(),CommandString[cmd],baseAddr, CommandString[origRequest->getCmd()], origRequest->getBaseAddr(),getCurrentSimTimeNano());
            }
            coherenceMgr->sendResponseUp(origRequest, NULLST, &event->getPayload(), true, 0);
            delete origRequest;
            break;
        default:
            d_->fatal(CALL_INFO, -1, "Command does not exist. Command: %s, Src: %s\n", CommandString[cmd], event->getSrc().c_str());
    }
}


void Cache::handlePrefetchEvent(SST::Event* event) {
    selfLink_->send(1, event);
}

/* Handler for self events, namely prefetches */
void Cache::handleSelfEvent(SST::Event* event) {
    MemEvent* ev = static_cast<MemEvent*>(event);
    ev->setBaseAddr(toBaseAddr(ev->getAddr()));
    
    if (!clockIsOn_) {
        Cycle_t time = reregisterClock(defaultTimeBase_, clockHandler_); 
        timestamp_ = time - 1;
        coherenceMgr->updateTimestamp(timestamp_);
        int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
        for (int64_t i = 0; i < cyclesOff; i++) {           // TODO more efficient way to do this? Don't want to add in one-shot or we get weird averages/sum sq.
            statMSHROccupancy->addData(mshr_->getSize());
        }
        //d_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), time, timestamp_, getCurrentSimTimeNano());
        clockIsOn_ = true;
    }
    
    if (ev->getCmd() != NULLCMD && !mshr_->isFull() && (cf_.L1_ || !mshr_->isAlmostFull()))
        processEvent(ev, false);
}



void Cache::init(unsigned int phase) {
    if (topNetworkLink_) { // I'm connected to the network ONLY via a single NIC
        bottomNetworkLink_->init(phase);
            
        /*  */
        while(MemEvent *ev = bottomNetworkLink_->recvInitData()) {
            delete ev;
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
            highNetPort_->sendInitData(new MemEvent(this, 0, 0, NULLCMD));
        }
        if (!bottomNetworkLink_) {
            for (uint i = 0; i < lowNetPorts_->size(); i++) {
                lowNetPorts_->at(i)->sendInitData(new MemEvent(this, 10, 10, NULLCMD));
            }
        }
        
    }

    while ((ev = highNetPort_->recvInitData())) {
        MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
        if (!memEvent) { /* Do nothing */ }
        else if (memEvent->getCmd() == NULLCMD) {
            if (memEvent->getCmd() == NULLCMD) {    // Save upper level cache names
                upperLevelCacheNames_.push_back(memEvent->getSrc());
            }
        } else {
            if (bottomNetworkLink_) {
                bottomNetworkLink_->sendInitData(new MemEvent(*memEvent));
            } else {
                for (uint idp = 0; idp < lowNetPorts_->size(); idp++) {
                    lowNetPorts_->at(idp)->sendInitData(new MemEvent(*memEvent));
                }
            }
        }
        delete memEvent;
     }
    
    if (!bottomNetworkLink_) {  // Save names of caches below us
        for (uint i = 0; i < lowNetPorts_->size(); i++) {
            while ((ev = lowNetPorts_->at(i)->recvInitData())) {
                MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
                if (memEvent && memEvent->getCmd() == NULLCMD) {
                    lowerLevelCacheNames_.push_back(memEvent->getSrc());
                }
                delete memEvent;
            }
        }
    }
}


void Cache::setup() {
    if (lowerLevelCacheNames_.size() == 0) lowerLevelCacheNames_.push_back(""); // avoid segfault on accessing this
    if (upperLevelCacheNames_.size() == 0) upperLevelCacheNames_.push_back(""); // avoid segfault on accessing this
    coherenceMgr->setLowerLevelCache(&lowerLevelCacheNames_);
    coherenceMgr->setUpperLevelCache(&upperLevelCacheNames_);
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
        coherenceMgr->updateTimestamp(timestamp_);
        int64_t cyclesOff = timestamp_ - lastActiveClockCycle_;
        for (int64_t i = 0; i < cyclesOff; i++) {           // TODO more efficient way to do this? Don't want to add in one-shot or we get weird averages/sum sq.
            statMSHROccupancy->addData(mshr_->getSize());
        }
        //d_->debug(_L3_, "%s turning clock ON at cycle %" PRIu64 ", timestamp %" PRIu64 ", ns %" PRIu64 "\n", this->getName().c_str(), time, timestamp_, getCurrentSimTimeNano());
        clockIsOn_ = true;
    }
    processEvent(event, false);
}
