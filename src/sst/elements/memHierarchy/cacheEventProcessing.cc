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
        if (mshr_->isFull() || (!L1_ && !replay && mshr_->isAlmostFull()  && !(cacheHit == 0))) { 
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 0));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 1));
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 0));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 1));
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 2));
                    } else if (cacheHit == 2) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 3));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 4));
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 2));
                    } else if (cacheHit == 2) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 3));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 4));
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 5));
                    } else if (cacheHit == 2) { 
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 6));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 7));
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
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 5));
                    } else if (cacheHit == 2) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 6));
                    } else if (cacheHit == 3) {
                        missTypeList_.insert(std::pair<MemEvent*,int>(event, 7));
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
    if (is_debug_event(ev)) {
        if (replay) {
            d_->debug(_L3_, "Replay. Name: %s. Cycles: %" PRIu64 ". Event: (%s)\n", this->getName().c_str(), timestamp_, ev->getVerboseString().c_str());
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
                    profileEvent(event, cmd, replay, canStall);
                    sendNACK(event);
                    break;
                } else if (canStall) {
                    d_->debug(_L6_,"Stalling request...MSHR almost full\n");
                    return false;
                }
            }
            
            profileEvent(event, cmd, replay, canStall);
            
            if (mshr_->isHit(baseAddr) && canStall) {
                // Drop local prefetches if there are outstanding requests for the same address NOTE this includes replacements/inv/etc.
                if (event->isPrefetch() && event->getRqstr() == this->getName()) {
                    if (is_debug_addr(baseAddr))
                        d_->debug(_L6_, "Drop prefetch: cache hit\n");
                    statPrefetchDrop->addData(1);
                    delete event;
                    break;
                }
                if (processRequestInMSHR(baseAddr, event)) {
                    if (is_debug_addr(baseAddr)) 
                        d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                    
                    event->setBlocked(true);
                }
                // track times in our separate queue
                if (startTimeList_.find(event) == startTimeList_.end()) {
                    startTimeList_.insert(std::pair<MemEvent*,uint64>(event, timestamp_));
                }

                break;
            }
            
            // track times in our separate queue
            if (startTimeList_.find(event) == startTimeList_.end()) {
                startTimeList_.insert(std::pair<MemEvent*,uint64>(event, timestamp_));
            }
            
            processCacheRequest(event, cmd, baseAddr, replay);
            break;
        case Command::GetSResp:
        case Command::GetXResp:
        case Command::FlushLineResp:
            profileEvent(event, cmd, replay, canStall);
            processCacheResponse(event, baseAddr);
            break;
        case Command::PutS:
        case Command::PutM:
        case Command::PutE:
            profileEvent(event, cmd, replay, canStall);
            processCacheReplacement(event, cmd, baseAddr, replay);
            break;
        case Command::NACK:
            profileEvent(event, cmd, replay, canStall);
            origEvent = event->getNACKedEvent();
            processIncomingNACK(origEvent);
            delete event;
            break;
        case Command::FetchInv:
        case Command::Fetch:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            profileEvent(event, cmd, replay, canStall);
            processCacheInvalidate(event, baseAddr, replay);
            break;
        case Command::AckPut:
        case Command::AckInv:
        case Command::FetchResp:
        case Command::FetchXResp:
            profileEvent(event, cmd, replay, canStall);
            processFetchResp(event, baseAddr);
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            profileEvent(event, cmd, replay, canStall);
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


void Cache::handlePrefetchEvent(SST::Event* ev) {
    prefetchLink_->send(prefetchDelay_, ev);
}

/* Handler for self events, namely prefetches */
void Cache::processPrefetchEvent(SST::Event* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    event->setRqstr(this->getName());

    if (!clockIsOn_) {
        turnClockOn();
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
    if (linkUp_ == linkDown_) {
        linkDown_->init(phase);

        if (!phase)
            linkDown_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Cache, type_ == "inclusive", type_ != "inclusive", cacheArray_->getLineSize(), true));

        /*  */
        while(MemEventInit *event = linkDown_->recvInitData()) {
            if (event->getCmd() == Command::NULLCMD) {
                d_->debug(_L10_, "%s received init event: %s\n", 
                        this->getName().c_str(), event->getVerboseString().c_str());
            }
            /* If event is from one of our destinations, update parameters - link only returns events from destinations */
            if (event->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(event);
                if (eventC->getType() != Endpoint::Memory) { // All other types do coherence
                    isLL = false;
                }
                if (eventC->getTracksPresence() || !isLL) {
                    silentEvict = false;
                }
                if (!eventC->getInclusive()) {
                    lowerIsNoninclusive = true; // TODO better checking if multiple caches below us
                }
                if (eventC->getWBAck()) {
                    expectWritebackAcks = true;
                }
            }
            delete event;
        }
        return;
    }
    
    linkUp_->init(phase);
    linkDown_->init(phase);
    
    if (!phase) {
        // MemEventInit: Name, NULLCMD, Endpoint type, inclusive of all upper levels, will send writeback acks, line size
        linkUp_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Cache, type_ == "inclusive", type_ != "inclusive", cacheArray_->getLineSize(), true));
        linkDown_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Cache, type_ == "inclusive", type_ != "inclusive", cacheArray_->getLineSize(), true));
    }

    while (MemEventInit * memEvent = linkUp_->recvInitData()) {
        if (memEvent->getCmd() == Command::NULLCMD) {
            d_->debug(_L10_, "%s received init event %s\n", getName().c_str(), memEvent->getVerboseString().c_str());
            upperLevelCacheNames_.push_back(memEvent->getSrc());
        } else {
            d_->debug(_L10_, "%s received init event %s\n", getName().c_str(), memEvent->getVerboseString().c_str());
            MemEventInit * mEv = memEvent->clone();
            mEv->setSrc(getName());
            mEv->setDst(linkDown_->findTargetDestination(mEv->getRoutingAddress()));
            linkDown_->sendInitData(mEv);
        }
        delete memEvent;
    }
    
    while (MemEventInit * memEvent = linkDown_->recvInitData()) {
        if (memEvent->getCmd() == Command::NULLCMD) {
            d_->debug(_L10_, "%s received init event %s\n", getName().c_str(), memEvent->getVerboseString().c_str());
            
            if (linkDown_->isDest(memEvent->getSrc()) && memEvent->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(memEvent);
                if (eventC->getType() != Endpoint::Memory) { // All other types to coherence
                    isLL = false;
                }
                if (eventC->getTracksPresence() || !isLL) {
                    silentEvict = false;
                }
                if (!eventC->getInclusive()) {
                    lowerIsNoninclusive = true; // TODO better checking if multiple caches below us
                }
                if (eventC->getWBAck()) {
                    expectWritebackAcks = true;
                }

                lowerLevelCacheNames_.push_back(eventC->getSrc());
            }
        }
        delete memEvent;
    }
}


void Cache::setup() {
    // Check that our sources and destinations exist or configure if needed
    
    std::set<MemLinkBase::EndpointInfo> * names = linkUp_->getSources();

    if (names->empty()) {
        std::set<MemLinkBase::EndpointInfo> srcNames;
        if (upperLevelCacheNames_.empty()) upperLevelCacheNames_.push_back(""); // TODO is this a carry over from the old init or is it needed to avoid segfaults still?
        for (int i = 0; i < upperLevelCacheNames_.size(); i++) {
            MemLinkBase::EndpointInfo info;
            info.name = upperLevelCacheNames_[i];
            info.addr = 0;
            info.id = 0;
            info.region.setDefault();
            srcNames.insert(info);
        }
        linkUp_->setSources(srcNames);
    }
    names = linkUp_->getSources();
    if (names->empty()) 
        out_->fatal(CALL_INFO, -1,"%s did not find any sources\n", getName().c_str());

    names = linkDown_->getDests();
    if (names->empty()) {
        std::set<MemLinkBase::EndpointInfo> dstNames;
        if (lowerLevelCacheNames_.empty()) lowerLevelCacheNames_.push_back(""); // TODO is this a carry over from the old init or is it needed to avoid segfaults still?
        uint64_t ilStep = 0;
        uint64_t ilSize = 0;
        if (lowerLevelCacheNames_.size() > 1) { // RR slice addressing
            ilStep = cacheArray_->getLineSize() * lowerLevelCacheNames_.size();
            ilSize = cacheArray_->getLineSize();
        }
        for (int i = 0; i < lowerLevelCacheNames_.size(); i++) {
            MemLinkBase::EndpointInfo info;
            info.name = lowerLevelCacheNames_[i];
            info.addr = 0;
            info.id = 0;
            info.region.setDefault();
            info.region.interleaveStep = ilStep;
            info.region.interleaveSize = ilSize;
            dstNames.insert(info);
        }
        linkDown_->setDests(dstNames);
    }

    names = linkDown_->getDests();
    if (names->empty())
        out_->fatal(CALL_INFO, -1, "%s did not find any destinations\n", getName().c_str());

    linkUp_->setup();
    if (linkUp_ != linkDown_) linkDown_->setup();

    coherenceMgr_->setupLowerStatus(silentEvict, isLL, expectWritebackAcks, lowerIsNoninclusive);
}


void Cache::finish() {
    if (!clockIsOn_) { // Correct statistics
        turnClockOn();
    }
    listener_->printStats(*d_);
    linkDown_->finish();
    if (linkUp_ != linkDown_) linkUp_->finish();
}

/* Main handler for links to upper and lower caches/cores/buses/etc */
void Cache::processIncomingEvent(SST::Event* ev) {
    MemEventBase* event = static_cast<MemEventBase*>(ev);
    if (!clockIsOn_) {
        turnClockOn();
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

/* Clock handler */
bool Cache::clockTick(Cycle_t time) {
    timestamp_++;
    bool queuesEmpty = coherenceMgr_->sendOutgoingCommands(getCurrentSimTimeNano());
        
    bool nicIdle = true;
    if (clockLink_) nicIdle = linkDown_->clock();

    if (checkMaxWaitInterval_ > 0 && timestamp_ % checkMaxWaitInterval_ == 0) checkMaxWait();
        
    // MSHR occupancy
    statMSHROccupancy->addData(mshr_->getSize());
        
    // Clear bank status and issue conflicted requests
    bool conflicts = false;
    for (unsigned int bank = 0; bank < bankStatus_.size(); bank++) {
        bankStatus_[bank] = false;
        while (!bankConflictBuffer_[bank].empty() && !bankStatus_[bank]) {
            conflicts = true;
            if (is_debug_event(bankConflictBuffer_[bank].front())) 
                d_->debug(_L9_,"%s, Retrying event from bank conflict. Bank: %u, Event: %s\n", getName().c_str(), bank, bankConflictBuffer_[bank].front()->getBriefString().c_str());
                
            bool processed = processEvent(bankConflictBuffer_[bank].front(), false);
            if (!processed) break;
            bankConflictBuffer_[bank].pop();
        }
    }

    // Accept any incoming requests that were delayed because of port limits
    requestsThisCycle_ = 0;
    std::queue<MemEventBase*>   tmpBuffer;
    while (!requestBuffer_.empty()) {
        if (requestsThisCycle_ == maxRequestsPerCycle_) {
            break;
        }
        
        bool wasProcessed = processEvent(requestBuffer_.front(), false);
        if (wasProcessed) {
            requestsThisCycle_++;
        } else {
            tmpBuffer.push(requestBuffer_.front());
        }
            
        requestBuffer_.pop();
        queuesEmpty = false;
    }
    if (!tmpBuffer.empty()) {
        while (!requestBuffer_.empty()) {
            tmpBuffer.push(requestBuffer_.front());
            requestBuffer_.pop();
        }
        requestBuffer_.swap(tmpBuffer);
    }
    // Disable lower-level cache clocks if they're idle
    if (queuesEmpty && nicIdle && clockIsOn_ && !conflicts) {
        turnClockOff();
        return true;
    }
    return false;
}

void Cache::turnClockOn() {
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

void Cache::turnClockOff() {
    clockIsOn_ = false;
    lastActiveClockCycle_ = timestamp_;
    if (!maxWaitWakeupExists_) {
        maxWaitWakeupExists_ = true;
        maxWaitSelfLink_->send(1, NULL);
    }
}
