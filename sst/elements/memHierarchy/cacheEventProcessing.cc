// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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


void Cache::profileEvent(MemEvent* event, Command cmd, bool _mshrHit) {
    if (mshr_->isHitAndStallNeeded(event->getBaseAddr(), cmd)) return; // will block this event, profile it later
    int cacheHit = isCacheHit(event, cmd, event->getBaseAddr());
    bool wasBlocked = event->blocked();                             // Event was blocked, now we're starting to handle it
    if (wasBlocked) event->setBlocked(false);

    switch(cmd) {
        case GetS:
            if (!_mshrHit) {                // New event
                if (cacheHit == 0) stats_[0].newReqGetSHits_++;
                else {
                    stats_[0].newReqGetSMisses_++;
                    if (cacheHit == 1 || cacheHit == 2) stats_[0].GetS_IS++;
                    else if (cacheHit == 3) stats_[0].GetS_M++;
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) stats_[0].blockedReqGetSHits_++;
                else {
                    stats_[0].blockedReqGetSMisses_++;
                    if (cacheHit == 1 || cacheHit == 2) stats_[0].GetS_IS++;
                    else if (cacheHit == 3) stats_[0].GetS_M++;
                }
            }
            break;
        case GetX:
            if (!_mshrHit) {                // New event
                if (cacheHit == 0) stats_[0].newReqGetXHits_++;
                else {
                    stats_[0].newReqGetXMisses_++;
                    if (cacheHit == 1) stats_[0].GetX_IM++;
                    else if (cacheHit == 2) stats_[0].GetX_SM++;
                    else if (cacheHit == 3) stats_[0].GetX_M++;
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) stats_[0].blockedReqGetXHits_++;
                else {
                    stats_[0].blockedReqGetXMisses_++;
                    if (cacheHit == 1) stats_[0].GetX_IM++;
                    else if (cacheHit == 2) stats_[0].GetX_SM++;
                    else if (cacheHit == 3) stats_[0].GetX_M++;
                }
            }
            break;
        case GetSEx:
            if (!_mshrHit) {                // New event
                if (cacheHit == 0) stats_[0].newReqGetSExHits_++;
                else {
                    stats_[0].newReqGetSExMisses_++;
                    if (cacheHit == 1) stats_[0].GetSE_IM++;
                    else if (cacheHit == 2) stats_[0].GetSE_SM++;
                    else if (cacheHit == 3) stats_[0].GetSE_M++;
                }
            } else if (wasBlocked) {        // Blocked event, now unblocked
                if (cacheHit == 0) stats_[0].blockedReqGetSExHits_++;
                else {
                    stats_[0].blockedReqGetSExMisses_++;
                    if (cacheHit == 1) stats_[0].GetSE_IM++;
                    else if (cacheHit == 2) stats_[0].GetSE_SM++;
                    else if (cacheHit == 3) stats_[0].GetSE_M++;
                }
            }
            break;
        case GetSResp:
        case GetXResp:
        case PutS:
        case PutE:
        case PutX:
        case PutXE:
        case PutM:
        case FetchInv:
        case FetchInvX:
        case Inv:
        case InvX:
        case NACK:
        default:
            break;

    }
}


void Cache::processEvent(MemEvent* event, bool _mshrHit) {
    Command cmd     = event->getCmd();
    if(L1_) event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
    bool noncacheable   = event->queryFlag(MemEvent::F_NONCACHEABLE) || cf_.allNoncacheableRequests_;
    MemEvent* origEvent;
    
    if(!_mshrHit){ 
        incTotalRequestsReceived(groupId);
        d2_->debug(_L3_,"\n\n-------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"); 
        cout << flush;
    }
    else incTotalMSHRHits(groupId);

    /* Set requestor field if this is the first cache that's seen this event */
    if (event->getRqstr() == "None") { event->setRqstr(this->getName()); }

    d_->debug(_L3_,"Incoming Event. Name: %s, Cmd: %s, BsAddr: %"PRIx64", Addr: %"PRIx64", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Size = %u, time: %"PRIu64", %s%s \n",
                   this->getName().c_str(), CommandString[event->getCmd()], baseAddr, event->getAddr(), event->getRqstr().c_str(), event->getSrc().c_str(), event->getDst().c_str(), event->isPrefetch() ? "true" : "false", event->getSize(), timestamp_, noncacheable ? "noncacheable" : "cacheable", _mshrHit ? ", replay" : "");
    
    if(noncacheable || cf_.allNoncacheableRequests_){
        processNoncacheable(event, cmd, baseAddr);
        return;
    }
    profileEvent(event, cmd, _mshrHit);

    switch(cmd){
        case GetS:
        case GetX:
        case GetSEx:
            // Determine if request should be NACKed: Request cannot be handled immediately and there are no free MSHRs to buffer the request
            if(mshr_->isFull() || (!L1_ && !_mshrHit && mshr_->isAlmostFull()  && (!isCacheHit(event,cmd,baseAddr) || mshr_->isHitAndStallNeeded(baseAddr, cmd)))){ 
                // Requests can cause deadlock because requests and fwd requests (inv, fetch, etc) share mshrs -> always leave one mshr free for fwd requests
                sendNACK(event);
                break;
            }
            if(mshr_->isHitAndStallNeeded(baseAddr, cmd)){
                if(processRequestInMSHR(baseAddr, event)){
                    d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                    event->setBlocked(true);
                }
                break;
            }
            processCacheRequest(event, cmd, baseAddr, _mshrHit);
            break;
        case GetSResp:
        case GetXResp:
            processCacheResponse(event, baseAddr);
            break;
        case PutS:
        case PutM:
        case PutE:
        case PutX:
        case PutXE:
            processCacheReplacement(event, cmd, baseAddr, _mshrHit);
            break;
        case Inv:
        case InvX:
            processCacheInvalidate(event, cmd, baseAddr, _mshrHit);
            break;
        case NACK:
            origEvent = event->getNACKedEvent();
            processIncomingNACK(origEvent);
            delete event;
            break;
        case FetchInv:
        case FetchInvX:
            processFetch(event, baseAddr, _mshrHit);
            break;
        default:
            _abort(MemHierarchy::Cache, "Command not supported, cmd = %s", CommandString[cmd]);
    }
}

void Cache::processNoncacheable(MemEvent* _event, Command _cmd, Addr _baseAddr){
    vector<mshrType> mshrEntry;
    MemEvent* origRequest;
    bool inserted;
    _event->setFlag(MemEvent::F_NONCACHEABLE);
    
    switch(_cmd){
        case GetS:
        case GetX:
        case GetSEx:
	    if (_cmd == GetSEx) d_->debug(_WARNING_, "WARNING: Noncachable atomics have undefined behavior; atomicity not preserved\n"); 
            inserted = mshrNoncacheable_->insert(_baseAddr, _event);
            assert(inserted);
            _event->setStartTime(timestamp_);
            if(_cmd == GetS) bottomCC_->forwardMessage(_event, _baseAddr, _event->getSize(), NULL);
            else             bottomCC_->forwardMessage(_event, _baseAddr, _event->getSize(), &_event->getPayload());
            break;
        case GetSResp:
        case GetXResp:
            origRequest = mshrNoncacheable_->removeFront(_baseAddr);
            assert(origRequest->getID().second == _event->getResponseToID().second);
            topCC_->sendResponse(origRequest, DUMMY, &_event->getPayload(), true);
            delete origRequest;
            break;
        default:
            _abort(MemHierarchy::Cache, "Command does not exist. Command: %s, Src: %s\n", CommandString[_cmd], _event->getSrc().c_str());
            break;
    }
}


void Cache::handlePrefetchEvent(SST::Event* _event) {
    selfLink_->send(1, _event);
}

void Cache::handleSelfEvent(SST::Event* _event){
    MemEvent* ev = static_cast<MemEvent*>(_event);
    ev->setBaseAddr(toBaseAddr(ev->getAddr()));
    ev->setInMSHR(false);
    ev->setStatsUpdated(false);
    
    if(ev->getCmd() != NULLCMD && !mshr_->isFull() && (L1_ || !mshr_->isAlmostFull()))
        processEvent(ev, false);
}



void Cache::init(unsigned int _phase){
    if (cf_.topNetwork_ == "cache") { // I'm connected to the network ONLY via a single NIC
        bottomNetworkLink_->init(_phase);
            
        /*  */
        while(MemEvent *ev = bottomNetworkLink_->recvInitData()){
            delete ev;
        }
        return;
    }
    
    SST::Event *ev;
    if(bottomNetworkLink_) {
        bottomNetworkLink_->init(_phase);
    }
    
    if(!_phase){
        if(L1_) {
            for(uint idc = 0; idc < highNetPorts_->size(); idc++) {
                highNetPorts_->at(idc)->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
            }
        } else{
            for(uint i = 0; i < highNetPorts_->size(); i++) {
                highNetPorts_->at(i)->sendInitData(new MemEvent(this, 0, 0, NULLCMD));
            }
        }
        if(cf_.bottomNetwork_ == ""){
            for(uint i = 0; i < lowNetPorts_->size(); i++) {
                lowNetPorts_->at(i)->sendInitData(new MemEvent(this, 10, 10, NULLCMD));
            }
        }
        
    }

    for(uint idc = 0; idc < highNetPorts_->size(); idc++) {
        while ((ev = (highNetPorts_->at(idc))->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else{
                if(cf_.bottomNetwork_ != "") 
                {
                    bottomNetworkLink_->sendInitData(new MemEvent(*memEvent));
                }
                else{
                    for(uint idp = 0; idp < lowNetPorts_->size(); idp++) {
                        lowNetPorts_->at(idp)->sendInitData(new MemEvent(*memEvent));
                    }
                }
            }
            delete memEvent;
        }
    }
    
    if(cf_.bottomNetwork_ == ""){
        for(uint i = 0; i < lowNetPorts_->size(); i++) {
            while ((ev = lowNetPorts_->at(i)->recvInitData())){
                MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
                if(!memEvent) delete memEvent;
                else if(memEvent->getCmd() == NULLCMD) {
                    nextLevelCacheNames_->push_back(memEvent->getSrc());
                }
                delete memEvent;
            }
        }
    }
}


void Cache::setup(){
    if (nextLevelCacheNames_->size() == 0) nextLevelCacheNames_->push_back(""); // avoid segfault on accessing this
    bottomCC_->setNextLevelCache(nextLevelCacheNames_);
}


void Cache::finish(){
    uint64_t averageLatency;
    if(upgradeCount_ > 0) averageLatency = totalUpgradeLatency_/upgradeCount_;
    else averageLatency = 0;

    bottomCC_->printStats(statsFile_, cf_.statGroupIds_, stats_, averageLatency);
    topCC_->printStats(statsFile_);
    listener_->printStats(*d_);
    delete cf_.cacheArray_;
    delete cf_.rm_;
    delete d_;
    retryQueue_.clear();
    retryQueueNext_.clear();
    linkIdMap_.clear();
    nameMap_.clear();
}


void Cache::processIncomingEvent(SST::Event* _ev){
    MemEvent* event = static_cast<MemEvent*>(_ev);
    event->setInMSHR(false);
    event->setStatsUpdated(false);
    processEvent(event, false);
}
