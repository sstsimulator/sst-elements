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

#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>


using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

bool Cache::clockTick(Cycle_t time) {
    timestamp_++; topCC_->timestamp_++; bottomCC_->timestamp_++;
    if(dirControllerExists_) directoryLink_->clock();
    topCC_->sendOutgoingCommands();
    bottomCC_->sendOutgoingCommands();
    
    if(UNLIKELY(!retryQueueNext_.empty())){
        retryQueue_ = retryQueueNext_;
        retryQueueNext_.clear();
        for(vector<MemEvent*>::iterator it = retryQueue_.begin(); it != retryQueue_.end();){
            d_->debug(_L1_,"RETRYING EVENT\n");
            processEvent(*it, true);
            retryQueue_.erase(it);
        }
    }
    else{
        
        //if(!L1_){
        if(!upperEventQueue_.empty()){  //TODO: don't call this upperEventQueue.. check where it is being used
            processEvent(upperEventQueue_.front().first, false);
            upperEventQueue_.pop();
        }
       // TODO:  Logic for <Tag Copies> goes here
       
    }
    
    return false;
}

void Cache::init(unsigned int phase){
    if(directoryLink_) directoryLink_->init(phase);
    if(!phase){
        for(uint idc = 0; idc < childrenLinks_->size(); idc++) childrenLinks_->at(idc)->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
        if(!dirControllerExists_)
            for(uint idp = 0; idp < parentLinks_->size(); idp++) parentLinks_->at(idp)->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
    }

    for(uint idc = 0; idc < childrenLinks_->size(); idc++) {
        SST::Event *ev;// = (childrenLinks_->at(idc))->recvInitData();
        while ((ev = (childrenLinks_->at(idc))->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else{
                if(dirControllerExists_) directoryLink_->sendInitData(memEvent);
                else{
                    for(uint idp = 0; idp < parentLinks_->size(); idp++)
                        parentLinks_->at(idp)->sendInitData(memEvent);
                }
            }
        }
    }    
}

void Cache::processIncomingEvent(SST::Event *ev){
    upperEventQueue_.push(make_pair(ev, timestamp_));
}
  
void Cache::processEvent(SST::Event* ev, bool reActivation) {
    MemEvent *event = static_cast<MemEvent*>(ev); assert_msg(event, "Event is Null!!");
    Command cmd     = event->getCmd(); string prefetch =  event->isPrefetch() ? "true" : "false";
    Addr addr       = event->getAddr(), baseAddr = toBaseAddr(addr);
    int childId     = getChildId(event);
    bool uncached   = event->queryFlag(MemEvent::F_UNCACHED);
    if(!reActivation){
        STAT_TotalInstructionsRecieved_++;
        registerNewRequest(event->getID());
        d_->debug(_L0_,"\n\n----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
    }

    d_->debug(_L0_,"Incoming Event. Name: %s, Cmd: %s, Addr: %"PRIx64", BsAddr: %"PRIx64", Src: %s, Dst: %s, LinkID: %i, PreF:%s, time: %llu... %s \n", this->getName().c_str(), CommandString[event->getCmd()], addr, baseAddr, event->getSrc().c_str(), event->getDst().c_str(), childId, prefetch.c_str(), timestamp_, uncached ? "un$" : "");
    if(uncached){
        processUncached(event, cmd, baseAddr);
        return;
    }
    
    try{

    switch (cmd) {
        case GetSEx:
            if(!reActivation) STAT_GetSExReceived_++;
        case GetS: 
        case GetX:
            if(!reActivation) STAT_NonCoherenceReqsReceived_++;
            if(mshr_->isHitAndStallNeeded(baseAddr, cmd)){
                mshr_->insert(baseAddr, event);
                d_->debug(_L1_,"Adding event to MSHR queue.  Wait till blocking event completes to proceed with this event.\n");
                return;
            }
        case PutM:
        case PutE:
        case PutS:
            mshr_->insert(baseAddr, event);
            processAccess(event, cmd, baseAddr, reActivation);
            break;
        case Inv: 
        case InvX:
            processInvalidate(event, cmd, baseAddr, reActivation);
            break;
        case GetSResp:
        case GetXResp:
            processAccessAcknowledge(event, baseAddr);
            break;
        case InvAck:
            processInvalidateAcknowledge(event, baseAddr, reActivation);
            break;
        case Fetch:
        case FetchInvalidate:
        case FetchInvalidateX:
            processFetch(event, baseAddr, reActivation);
            break;
        default:
            delete event;
            _abort(MemHierarchy::Cache, "Command does not exist. Command: %s, Src: %s\n", CommandString[cmd], event->getSrc().c_str());
            break;
    }
    }
    catch(stallException const& e){
        //e.what();
    }
}

void Cache::processUncached(MemEvent* event, Command cmd, Addr baseAddr){
    vector<mshrType*> mshrEntry;
    MemEvent* memEvent;
    switch( cmd ){
        case GetS:
            mshrUncached_->insert(baseAddr, event);
            bottomCC_->forwardMessage(event, baseAddr, lineSize_, NULL);
            break;
        case GetX:
            mshrUncached_->insert(baseAddr, event);
            bottomCC_->forwardMessage(event, baseAddr, lineSize_, &event->getPayload());
            break;
        case GetSResp:
        case GetXResp:
            mshrEntry = mshrUncached_->remove(baseAddr);
            int i = 0;
            for(vector<mshrType*>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
                memEvent = boost::get<MemEvent*>(mshrEntry.front()->elem);
                topCC_->sendResponse(memEvent, DUMMY, &event->getPayload(), getChildId(memEvent));
                mshrEntry.erase(i);
            }
            delete event;
            break;
        default:
            _abort(MemHierarchy::Cache, "Command does not exist. Command: %s, Src: %s\n", CommandString[cmd], event->getSrc().c_str());
            break;
    }
}


void Cache::handlePrefetchEvent(SST::Event *event) {
    selfLink_->send(1, event);
}

void Cache::handleSelfEvent(SST::Event *event){
    MemEvent* ev = static_cast<MemEvent*>(event);
    processEvent(event, ev->isPrefetch());
}

