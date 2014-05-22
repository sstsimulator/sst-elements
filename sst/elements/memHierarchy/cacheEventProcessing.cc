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


void Cache::init(unsigned int _phase){
    
    SST::Event *ev;
    if(directoryLink_) directoryLink_->init(_phase);
    
    if(!_phase){
        if(L1_) for(uint idc = 0; idc < highNetPorts_->size(); idc++) highNetPorts_->at(idc)->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
        else{
            for(uint i = 0; i < highNetPorts_->size(); i++)
                highNetPorts_->at(i)->sendInitData(new MemEvent(this, 0, NULLCMD));
        }
        if(!dirControllerExists_){
            for(uint i = 0; i < lowNetPorts_->size(); i++)
                lowNetPorts_->at(i)->sendInitData(new MemEvent(this, 10, NULLCMD));
        }
        
    }

    for(uint idc = 0; idc < highNetPorts_->size(); idc++) {
        while ((ev = (highNetPorts_->at(idc))->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else{
                if(dirControllerExists_) directoryLink_->sendInitData(new MemEvent(memEvent));
                else{
                    for(uint idp = 0; idp < lowNetPorts_->size(); idp++)
                        lowNetPorts_->at(idp)->sendInitData(new MemEvent(memEvent));
                }
            }
            delete memEvent;
        }
    }
    
    if(!dirControllerExists_){
        for(uint i = 0; i < lowNetPorts_->size(); i++) {
            while ((ev = lowNetPorts_->at(i)->recvInitData())){
                MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
                if(!memEvent) delete memEvent;
                else if(memEvent->getCmd() == NULLCMD) nextLevelCacheName_ = memEvent->getSrc();
                delete memEvent;
            }
        }
    }
}


void Cache::setup(){
    bottomCC_->setNextLevelCache(nextLevelCacheName_);
}


void Cache::finish(){
    uint64_t averageLatency;
    if(mshrHits_ > 0) averageLatency = totalUpgradeLatency_/mshrHits_;
    else averageLatency = 0;
    
    bottomCC_->printStats(stats_, STAT_GetSExReceived_, STAT_InvalidateWaitingForUserLock_,
                          STAT_TotalRequestsRecieved_, STAT_TotalMSHRHits_, averageLatency);
    topCC_->printStats(stats_);
    listener_->printStats(*d_);
    delete cArray_;
    delete replacementMgr_;
    delete d_;
    retryQueue_.clear();
    retryQueueNext_.clear();
    linkIdMap_.clear();
    nameMap_.clear();
}

void Cache::processIncomingEvent(SST::Event* _ev){
    incomingEventQueue_.push(make_pair(_ev, timestamp_));
    if(!clockOn_){
        timestamp_ = reregisterClock(defaultTimeBase_, clockHandler_);
        clockOn_ = true;
        memNICIdleCount_ = 0;
        idleCount_ = 0;
    }
}
  
void Cache::processEvent(SST::Event* _ev, bool _mshrHit) {
    MemEvent *event = static_cast<MemEvent*>(_ev);
    Command cmd     = event->getCmd();
    Addr baseAddr   = toBaseAddr(event->getAddr());
    bool uncached   = event->queryFlag(MemEvent::F_UNCACHED);
    MemEvent* origEvent;
    
    if(!_mshrHit){
        STAT_TotalRequestsRecieved_++;
        d2_->debug(_L0_,"\n\n----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
    }
    else STAT_TotalMSHRHits_++;

    d_->debug(_L0_,"Incoming Event. Name: %s, Cmd: %s, BsAddr: %"PRIx64", Addr: %"PRIx64", EventID = %"PRIx64", Src: %s, Dst: %s, PreF:%s, time: %llu... %s \n",
                   this->getName().c_str(), CommandString[event->getCmd()], baseAddr, event->getAddr(), event->getID().first, event->getSrc().c_str(), event->getDst().c_str(), event->isPrefetch() ? "true" : "false", timestamp_, uncached ? "un$" : "");
    
    if(uncached){
        processUncached(event, cmd, baseAddr);
        return;
    }
    
    switch(cmd){
        case GetS:
        case GetX:
        case GetSEx:
        
            //TODO: refactor, save isHitAnd.. to bool, and only call processRequestinMSHR once
            if(mshr_->isHitAndStallNeeded(baseAddr, cmd)){
                if(processRequestInMSHR(baseAddr, event)) d_->debug(_L0_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                return;
            }
            if(mshr_->isFull()){
                sendNACK(event);
                return;
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
            processCacheRequest(event, cmd, baseAddr, _mshrHit);
            break;
        case Inv:
        case InvX:
            processCacheInvalidate(event, cmd, baseAddr, _mshrHit);
            break;
        case NACK:
            origEvent = event->getNACKedEvent();
            assert(origEvent);      //TODO: delete
            d_->debug(_L0_,"Orig Cmd NACKed = %s \n", CommandString[origEvent->getCmd()]);
            processIncomingNACK(origEvent);
            //delete event;         //TODO: delete
            break;
        case FetchInv:
        case FetchInvX:
            processFetch(event, baseAddr, _mshrHit);
            break;
        default:
            _abort(MemHierarchy::Cache, "Command not supported, cmd = %s", CommandString[cmd]);
    }
}

void Cache::processUncached(MemEvent* _event, Command _cmd, Addr _baseAddr){
    vector<mshrType> mshrEntry;
    MemEvent* memEvent;
    int i = 0;
    switch(_cmd){
        case GetS:
        case GetX:
            if(mshrUncached_->isHitAndStallNeeded(_baseAddr, _cmd)){
                mshrUncached_->insert(_baseAddr, _event);
                return;
            }
            mshrUncached_->insert(_baseAddr, _event);
            if(_cmd == GetS) bottomCC_->forwardMessage(_event, _baseAddr, lineSize_, NULL);
            else bottomCC_->forwardMessage(_event, _baseAddr, lineSize_, &_event->getPayload());
            break;
        case GetSResp:
        case GetXResp:
            mshrEntry = mshrUncached_->removeAll(_baseAddr);
            for(vector<mshrType>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
                memEvent = boost::get<MemEvent*>(mshrEntry.front().elem);
                topCC_->sendResponse(memEvent, DUMMY, &_event->getPayload(), true);
                delete memEvent;
                mshrEntry.erase(it);
                
            }
            delete _event;
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
    processEvent(_event, ev->isPrefetch());
}

