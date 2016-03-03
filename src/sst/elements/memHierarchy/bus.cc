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

Bus::Bus(ComponentId_t id, Params& params) : Component(id) {
	configureParameters(params);
    configureLinks();
    idleCount_ = 0;
    busOn_ = true;
}


void Bus::processIncomingEvent(SST::Event* ev) {
    eventQueue_.push(ev);
    if (!busOn_) {
        reregisterClock(defaultTimeBase_, clockHandler_);
        busOn_ = true;
        idleCount_ = 0;
    }
}

bool Bus::clockTick(Cycle_t time) {

    if (!eventQueue_.empty()) {
        SST::Event* event = eventQueue_.front();
        
        if (broadcast_) broadcastEvent(event);
        else sendSingleEvent(event);
        
        eventQueue_.pop();
        idleCount_ = 0;
    } else if (busOn_) idleCount_++;
    
    
    if (idleCount_ > idleMax_) {
        busOn_ = false;
        idleCount_ = 0;
        return true;
    }
    
    return false;
}


void Bus::broadcastEvent(SST::Event* ev) {
    MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
    LinkId_t srcLinkId = lookupNode(memEvent->getSrc());
    SST::Link* srcLink = linkIdMap_[srcLinkId];

    for (int i = 0; i < numHighNetPorts_; i++) {
        if (highNetPorts_[i] == srcLink) continue;
        highNetPorts_[i]->send(new MemEvent(*memEvent));
    }
    
    for (int i = 0; i < numLowNetPorts_; i++) {
        if (lowNetPorts_[i] == srcLink) continue;
        lowNetPorts_[i]->send( new MemEvent(*memEvent));
    }
    
    delete memEvent;
}



void Bus::sendSingleEvent(SST::Event* ev) {
    MemEvent *event = static_cast<MemEvent*>(ev);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
        dbg_.debug(_L3_,"\n\n");
        dbg_.debug(_L3_,"----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
        dbg_.debug(_L3_,"Incoming Event. Name: %s, Cmd: %s, Addr: %" PRIx64 ", BsAddr: %" PRIx64 ", Src: %s, Dst: %s, LinkID: %ld \n",
                   this->getName().c_str(), CommandString[event->getCmd()], event->getAddr(), event->getBaseAddr(), event->getSrc().c_str(), event->getDst().c_str(), event->getDeliveryLink()->getId());
    }
#endif
    LinkId_t dstLinkId = lookupNode(event->getDst());
    SST::Link* dstLink = linkIdMap_[dstLinkId];
    MemEvent* forwardEvent = new MemEvent(*event);
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == forwardEvent->getBaseAddr()) {
        dbg_.debug(_L3_,"BCmd = %s \n", CommandString[forwardEvent->getCmd()]);
        dbg_.debug(_L3_,"BDst = %s \n", forwardEvent->getDst().c_str());
        dbg_.debug(_L3_,"BSrc = %s \n", forwardEvent->getSrc().c_str());
    }
#endif
    dstLink->send(forwardEvent);
    
    delete event;
}

/*----------------------------------------
 * Helper functions
 *---------------------------------------*/

void Bus::mapNodeEntry(const std::string& name, LinkId_t id) {
	std::map<std::string, LinkId_t>::iterator it = nameMap_.find(name);
	if (nameMap_.end() != it) {
            dbg_.fatal(CALL_INFO, -1, "%s, Error: Bus attempting to map node that has already been mapped\n", getName().c_str());
        }
    nameMap_[name] = id;
}

LinkId_t Bus::lookupNode(const std::string& name) {
	std::map<std::string, LinkId_t>::iterator it = nameMap_.find(name);
    if (nameMap_.end() == it) {
        dbg_.fatal(CALL_INFO, -1, "%s, Error: Bus lookup of node %s returned no mapping\n", getName().c_str(), name.c_str());
    }
    return it->second;
}

void Bus::configureLinks() {
    SST::Link* link;
    for ( int i = 0 ; i < maxNumPorts_ ; i++ ) {
        std::ostringstream linkName;
        linkName << "high_network_" << i;
        std::string ln = linkName.str();
        link = configureLink(ln, "50 ps", new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if (link) {
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
        if (link) {
            lowNetPorts_.push_back(link);
            numLowNetPorts_++;
            linkIdMap_[lowNetPorts_[i]->getId()] = lowNetPorts_[i];
            dbg_.output(CALL_INFO, "Port %lu = Link %d\n", lowNetPorts_[i]->getId(), i);
        }
    }
    if (numLowNetPorts_ < 1 || numHighNetPorts_ < 1) dbg_.fatal(CALL_INFO, -1,"couldn't find number of Ports (numPorts)\n");

}

void Bus::configureParameters(SST::Params& params) {
    int debugLevel = params.find_integer("debug_level", 0);
    
    dbg_.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)     dbg_.fatal(CALL_INFO, -1, "Debugging level must be betwee 0 and 10. \n");
    int dAddr         = params.find_integer("debug_addr", -1);
    if (dAddr == -1) {
        DEBUG_ADDR = (Addr) dAddr;
        DEBUG_ALL = true;
    } else {
        DEBUG_ADDR = (Addr) dAddr;
        DEBUG_ALL = false;
    }
    numHighNetPorts_  = 0;
    numLowNetPorts_   = 0;
    maxNumPorts_      = 500;
    
    latency_      = params.find_integer("bus_latency_cycles", 1);
    idleMax_      = params.find_integer("idle_max", 6);
    busFrequency_ = params.find_string("bus_frequency", "Invalid");
    broadcast_    = params.find_integer("broadcast", 0);
    fanout_       = params.find_integer("fanout", 0);  /* TODO:  Fanout: Only send messages to lower level caches */

    if (busFrequency_ == "Invalid") dbg_.fatal(CALL_INFO, -1, "Bus Frequency was not specified\n");
    
     /* Multiply Frequency times two.  This is because an SST Bus components has
        2 SST Links (highNEt & LowNet) and thus it takes a least 2 cycles for any
        transaction (a real bus should be allowed to have 1 cycle latency).  To overcome
        this we clock the bus 2x the speed of the cores */

    UnitAlgebra uA = UnitAlgebra(busFrequency_);
    uA = uA * 2;
    busFrequency_ = uA.toString();
    
    clockHandler_ = new Clock::Handler<Bus>(this, &Bus::clockTick);
    defaultTimeBase_ = registerClock(busFrequency_, clockHandler_);
}

void Bus::init(unsigned int phase) {
    SST::Event *ev;

    for (int i = 0; i < numHighNetPorts_; i++) {
        while ((ev = highNetPorts_[i]->recvInitData())) {
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);

            if (memEvent && memEvent->getCmd() == NULLCMD) {
                mapNodeEntry(memEvent->getSrc(), highNetPorts_[i]->getId());
                for (int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(new MemEvent(*memEvent));
            } else if (memEvent) {
                for (int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(new MemEvent(*memEvent));
            }
            delete memEvent;
        }
    }
    
    for (int i = 0; i < numLowNetPorts_; i++) {
        while ((ev = lowNetPorts_[i]->recvInitData())) {
            MemEvent* memEvent = dynamic_cast<MemEvent*>(ev);
            if (!memEvent) delete memEvent;
            else if (memEvent->getCmd() == NULLCMD) {
                mapNodeEntry(memEvent->getSrc(), lowNetPorts_[i]->getId());
                for (int i = 0; i < numHighNetPorts_; i++) {
                    highNetPorts_[i]->sendInitData(new MemEvent(*memEvent));
                }
                delete memEvent;
            }
            else{
                /*Ignore responses */
                delete memEvent;
            }
        }
    }
}

