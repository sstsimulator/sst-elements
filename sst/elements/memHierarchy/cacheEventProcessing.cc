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
        if(!incomingEventQueue_.empty()){
            processEvent(incomingEventQueue_.front().first, false);
            incomingEventQueue_.pop();
        }
       
       
    }
    
    return false;
}

void Cache::init(unsigned int phase){
    
    /*
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
    */
    ///*
    SST::Event *ev;
    if(directoryLink_) directoryLink_->init(phase);
    
    if(!phase){
        if(L1_) for(uint idc = 0; idc < childrenLinks_->size(); idc++) childrenLinks_->at(idc)->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
        else{
            for(uint i = 0; i < childrenLinks_->size(); i++) {
                childrenLinks_->at(i)->sendInitData(new MemEvent(this, 0, NULLCMD));
            }
        }
        if(!dirControllerExists_){
            for(uint i = 0; i < parentLinks_->size(); i++){
                parentLinks_->at(i)->sendInitData(new MemEvent(this, 10, NULLCMD));
            }
        }
    }

    //leave as is!!!!!!!!!!!!!
    for(uint idc = 0; idc < childrenLinks_->size(); idc++) {
        while ((ev = (childrenLinks_->at(idc))->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            //assert(memEvent->getSrc());
            if(!memEvent) delete memEvent;
            else{
                if(dirControllerExists_) directoryLink_->sendInitData(memEvent);
                else{
                    for(uint idp = 0; idp < parentLinks_->size(); idp++){
                        parentLinks_->at(idp)->sendInitData(memEvent);
                    }
                }
            }
        }
    }
    
    if(!dirControllerExists_){
        for(uint i = 0; i < parentLinks_->size(); i++) {
            while ((ev = parentLinks_->at(i)->recvInitData())){
                MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
                if(!memEvent) delete memEvent;
                else if(memEvent->getCmd() == NULLCMD){
                    nextLevelCacheName_ = memEvent->getSrc();
                    delete memEvent;
                }
            }
        }
    }
    //*/
    
}

void Cache::setup(){
    bottomCC_->setNextLevelCache(nextLevelCacheName_);
}

void Cache::processIncomingEvent(SST::Event *ev){
    incomingEventQueue_.push(make_pair(ev, timestamp_));
}
  
void Cache::processEvent(SST::Event* ev, bool reActivation) {
    MemEvent *event = static_cast<MemEvent*>(ev); assert_msg(event, "Event is Null!!");
    assert(event);
    
    Command cmd     = event->getCmd(); string prefetch =  event->isPrefetch() ? "true" : "false";
    Addr addr       = event->getAddr(), baseAddr = toBaseAddr(addr);
    int childId     = getChildId(event);
    bool uncached   = event->queryFlag(MemEvent::F_UNCACHED);
    
    //if(event->getSrc().empty()) return;
    
    if(!reActivation){
        STAT_TotalInstructionsRecieved_++;
        registerNewRequest(event->getID());
        d_->debug(_L0_,"\n\n----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
    }

    d_->debug(_L0_,"Incoming Event. Name: %s, Cmd: %s, Addr: %"PRIx64", BsAddr: %"PRIx64", Src: %s, Dst: %s, PreF:%s, time: %llu... %s \n", this->getName().c_str(), CommandString[event->getCmd()], addr, baseAddr, event->getSrc().c_str(), event->getDst().c_str(), prefetch.c_str(), timestamp_, uncached ? "un$" : "");
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
            mshr_->insert(baseAddr, event);
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
            mshr_->insert(baseAddr, event);
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
    catch(mshrException const& e){
        _abort(MemHierarchy::Cache, "Limited MSHR is not supported yet, increment the number of MSHR entries\n");
        //topCC_->sendNACK(event);
    }
}

void Cache::processUncached(MemEvent* event, Command cmd, Addr baseAddr){
    vector<mshrType*> mshrEntry;
    MemEvent* memEvent;
    int i = 0;
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
            mshrEntry = mshrUncached_->removeAll(baseAddr);
            for(vector<mshrType*>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
                memEvent = boost::get<MemEvent*>(mshrEntry.front()->elem);
                topCC_->sendResponse(memEvent, DUMMY, &event->getPayload(), getChildId(memEvent));
                mshrEntry.erase(it);
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

