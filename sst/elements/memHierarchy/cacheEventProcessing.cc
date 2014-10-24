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


void Cache::processEvent(MemEvent* event, bool _mshrHit) {
    Command cmd     = event->getCmd();
    if(L1_) event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
    bool noncacheable   = event->queryFlag(MemEvent::F_NONCACHEABLE) || cf_.allNoncacheableRequests_;
    MemEvent* origEvent;
    
    if(!_mshrHit){
        incTotalRequestsReceived(groupId);
        d2_->debug(_L3_,"\n\n-------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");    //raise(SIGINT);
        cout << flush;
    }
    else incTotalMSHRHits(groupId);

    /* Set requestor field if this is the first cache that's seen this event */
    if (event->getRqstr() == "None") { event->setRqstr(this->getName()); }

    d_->debug(_L3_,"Incoming Event. Name: %s, Cmd: %s, BsAddr: %"PRIx64", Addr: %"PRIx64", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Size = %u, time: %"PRIu64", %s \n",
                   this->getName().c_str(), CommandString[event->getCmd()], baseAddr, event->getAddr(), event->getRqstr().c_str(), event->getSrc().c_str(), event->getDst().c_str(), event->isPrefetch() ? "true" : "false", event->getSize(), timestamp_, noncacheable ? "noncacheable" : "cacheable");
    
    if(noncacheable || cf_.allNoncacheableRequests_){
        processNoncacheable(event, cmd, baseAddr);
        return;
    }
    
    switch(cmd){
        case GetS:
        case GetX:
        case GetSEx:
            if(mshr_->isHitAndStallNeeded(baseAddr, cmd)){
                if(processRequestInMSHR(baseAddr, event)){
                    d_->debug(_L9_,"Added event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                }
                break;
            }
            if(mshr_->isFull()){
                sendNACK(event);
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
            processCacheRequest(event, cmd, baseAddr, _mshrHit);
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
            inserted = mshrNoncacheable_->insert(_baseAddr, _event);
            assert(inserted);
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

    if(ev->getCmd() != NULLCMD && !mshr_->isFull())
        processEvent(ev, ev->isPrefetch());
}



void Cache::init(unsigned int _phase){
    
    SST::Event *ev;
    if(directoryLink_) directoryLink_->init(_phase);
    
    if(!_phase){
        if(L1_) for(uint idc = 0; idc < highNetPorts_->size(); idc++) highNetPorts_->at(idc)->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
        else{
            for(uint i = 0; i < highNetPorts_->size(); i++)
                highNetPorts_->at(i)->sendInitData(new MemEvent(this, 0, 0, NULLCMD));
        }
        if(!cf_.dirControllerExists_){
            for(uint i = 0; i < lowNetPorts_->size(); i++)
                lowNetPorts_->at(i)->sendInitData(new MemEvent(this, 10, 10, NULLCMD));
        }
        
    }

    for(uint idc = 0; idc < highNetPorts_->size(); idc++) {
        while ((ev = (highNetPorts_->at(idc))->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else{
                if(cf_.dirControllerExists_) directoryLink_->sendInitData(new MemEvent(*memEvent));
                else{
                    for(uint idp = 0; idp < lowNetPorts_->size(); idp++)
                        lowNetPorts_->at(idp)->sendInitData(new MemEvent(*memEvent));
                }
            }
            delete memEvent;
        }
    }
    
    if(!cf_.dirControllerExists_){
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
    //assert(!event->inMSHR());
    //assert(!event->statsUpdated());
    event->setInMSHR(false);
    event->setStatsUpdated(false);
    processEvent(event, false);
}
