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
 * File:   coherenceControllers.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sstream>

#include "bus.h"

#include <csignal>
#include <boost/variant.hpp>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include "memEvent.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

const Bus::key_t Bus::ANY_KEY = std::pair<uint64_t, int>((uint64_t)-1, -1);

Bus::Bus(ComponentId_t _id, Params& _params) : Component(_id){
	configureParameters(_params);
    configureLinks();
    idleCount_ = 0;
    busOn_ = true;
}

Bus::Bus() : Component(-1) {}

void Bus::processIncomingEvent(SST::Event* _ev){
    eventQueue_.push(_ev);
    if(!busOn_){
        //std::cout << "HEREEEE" << std::endl;
        reregisterClock(defaultTimeBase_, clockHandler_);
        busOn_ = true;
        idleCount_ = 0;
    }
}

bool Bus::clockTick(Cycle_t _time) {

    if(!eventQueue_.empty()){
        SST::Event* event = eventQueue_.front();
        
        if(broadcast_) broadcastEvent(event);
        else sendSingleEvent(event);
        
        eventQueue_.pop();
        idleCount_ = 0;
    }
    else if(busOn_) idleCount_++;
    
    
    if(idleCount_ > idleMax_){
        busOn_ = false;
        idleCount_ = 0;
        return true;
    }
    
    return false;
}



void Bus::broadcastEvent(SST::Event* _ev){
    MemEvent* memEvent = dynamic_cast<MemEvent*>(_ev);
    LinkId_t srcLinkId = lookupNode(memEvent->getSrc());
    SST::Link* srcLink = linkIdMap_[srcLinkId];

    for(int i = 0; i < numHighNetPorts_; i++) {
        if(highNetPorts_[i] == srcLink) continue;
        highNetPorts_[i]->send(new MemEvent(memEvent));
    }
    
    for(int i = 0; i < numLowNetPorts_; i++) {
        if(lowNetPorts_[i] == srcLink) continue;
        lowNetPorts_[i]->send( new MemEvent(memEvent));
    }
    
    delete memEvent;
}



void Bus::sendSingleEvent(SST::Event* _ev){
    MemEvent *event = static_cast<MemEvent*>(_ev);
    dbg_.debug(_L3_,"\n\n");
    dbg_.debug(_L3_,"----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
    dbg_.debug(_L3_,"Incoming Event. Name: %s, Cmd: %s, Addr: %"PRIx64", BsAddr: %"PRIx64", Src: %s, Dst: %s, LinkID: %ld \n",
                   this->getName().c_str(), CommandString[event->getCmd()], event->getAddr(), event->getBaseAddr(), event->getSrc().c_str(), event->getDst().c_str(), event->getDeliveryLink()->getId());

    LinkId_t dstLinkId = lookupNode(event->getDst());
    SST::Link* dstLink = linkIdMap_[dstLinkId];
    dstLink->send(new MemEvent(event));
    
    delete event;
}

/*----------------------------------------
 * Helper functions
 *---------------------------------------*/

void Bus::mapNodeEntry(const std::string& _name, LinkId_t _id){
	std::map<std::string, LinkId_t>::iterator it = nameMap_.find(_name);
	assert(nameMap_.end() == it);
    nameMap_[_name] = _id;
}

LinkId_t Bus::lookupNode(const std::string& _name){
	std::map<std::string, LinkId_t>::iterator it = nameMap_.find(_name);
	assert(nameMap_.end() != it);
    return it->second;
}

void Bus::configureLinks(){
    SST::Link* link;
	for ( int i = 0 ; i < maxNumPorts_ ; i++ ) {
		std::ostringstream linkName;
		linkName << "high_network_" << i;
		std::string ln = linkName.str();
		link = configureLink(ln, "50 ps", new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
		if(link){
            highNetPorts_.push_back(link);
            numHighNetPorts_++;
            linkIdMap_[highNetPorts_[i]->getId()] = highNetPorts_[i];
            dbg_.output(CALL_INFO, "Port %lu = Link %d\n", highNetPorts_[i]->getId(), i);
        }
    }
    
    for ( int i = 0 ; i < maxNumPorts_ ; i++ ) {
		std::ostringstream linkName;
		linkName << "low_network_" << i;
		std::string ln = linkName.str();
		link = configureLink(ln, "50 ps", new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if(link){
            lowNetPorts_.push_back(link);
            numLowNetPorts_++;
            linkIdMap_[lowNetPorts_[i]->getId()] = lowNetPorts_[i];
            dbg_.output(CALL_INFO, "Port %lu = Link %d\n", lowNetPorts_[i]->getId(), i);
        }
	}
    if(numLowNetPorts_ < 1 || numHighNetPorts_ < 1) _abort(Bus,"couldn't find number of Ports (numPorts)\n");

}

void Bus::configureParameters(SST::Params& _params){
    dbg_.init("" + getName() + ": ", 0, 0, (Output::output_location_t)_params.find_integer("debug", 0));
    numLowNetPortsX_  = _params.find_integer("low_network_ports", 0);
	numHighNetPortsX_ = _params.find_integer("high_network_ports", 0);
    numHighNetPorts_  = 0;
    numLowNetPorts_   = 0;
    maxNumPorts_      = 500;
    
	latency_      = _params.find_integer("bus_latency_cycles", 1);
    idleMax_      = _params.find_integer("idle_max", 6);
    busFrequency_ = _params.find_string("bus_frequency", "Invalid");
    broadcast_    = _params.find_integer("broadcast", 0);
    fanout_       = _params.find_integer("fanout", 0);

    if(busFrequency_ == "Invalid") _abort(Bus, "Bus Frequency was not specified\n");
    if(broadcast_ < 0 || broadcast_ > 1) _abort(Bus, "Broadcast feature was not specified correctly\n");
    clockHandler_ = new Clock::Handler<Bus>(this, &Bus::clockTick);
    defaultTimeBase_ = registerClock(busFrequency_, clockHandler_);
}

void Bus::init(unsigned int _phase){
    SST::Event *ev;

    for(int i = 0; i < numHighNetPorts_; i++) {
        while ((ev = highNetPorts_[i]->recvInitData())){
            
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else if(memEvent->getCmd() == NULLCMD){
                mapNodeEntry(memEvent->getSrc(), highNetPorts_[i]->getId());
            }
            else{
                for(int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(new MemEvent(memEvent));
            }
            delete memEvent;
        }
    }
    
    for(int i = 0; i < numLowNetPorts_; i++) {
        while ((ev = lowNetPorts_[i]->recvInitData())){
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if(!memEvent) delete memEvent;
            else if(memEvent->getCmd() == NULLCMD){
                mapNodeEntry(memEvent->getSrc(), lowNetPorts_[i]->getId());
                for(int i = 0; i < numHighNetPorts_; i++) {
                    highNetPorts_[i]->sendInitData(new MemEvent(memEvent));
                    //cout << "sent memEvent, src: " << temp->getSrc() << " linkID: " << highNetPorts_[i]->getId() << endl;
                }
            }
            else{/*Ignore responses */}
            delete memEvent;
        }
    }
}

