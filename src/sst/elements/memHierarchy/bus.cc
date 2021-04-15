// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>

#include <sstream>

#include "bus.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include "memEvent.h"
#include "memEventBase.h"

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

    if (eventQueue_.empty())
        idleCount_++;

    if (idleCount_ > idleMax_) {
        busOn_ = false;
        idleCount_ = 0;
        return true;
    }

    while (!eventQueue_.empty()) {
        SST::Event* event = eventQueue_.front();

        if (broadcast_)
            broadcastEvent(event);
        else
            sendSingleEvent(event);

        eventQueue_.pop();
        idleCount_ = 0;

        if (drain_ == 0 )
            break;
    }

    return false;
}


void Bus::broadcastEvent(SST::Event* ev) {
    MemEventBase* memEvent = static_cast<MemEventBase*>(ev);
    LinkId_t srcLinkId = lookupNode(memEvent->getSrc());
    SST::Link* srcLink = linkIdMap_[srcLinkId];

    for (int i = 0; i < numHighNetPorts_; i++) {
        if (highNetPorts_[i] == srcLink) continue;
        highNetPorts_[i]->send(memEvent->clone());
    }

    for (int i = 0; i < numLowNetPorts_; i++) {
        if (lowNetPorts_[i] == srcLink) continue;
        lowNetPorts_[i]->send(memEvent->clone());
    }

    delete memEvent;
}



void Bus::sendSingleEvent(SST::Event* ev) {
    MemEventBase *event = static_cast<MemEventBase*>(ev);
#ifdef __SST_DEBUG_OUTPUT__
    if (is_debug_event(event)) {
        dbg_.debug(_L3_,"\n\n");
        dbg_.debug(_L3_,"----------------------------------------------------------------------------------------\n");    //raise(SIGINT);
        dbg_.debug(_L3_,"Incoming Event. Name: %s, Event: %s\n",
                   this->getName().c_str(), event->getBriefString().c_str());
    }
#endif
    LinkId_t dstLinkId = lookupNode(event->getDst());
    SST::Link* dstLink = linkIdMap_[dstLinkId];
    MemEventBase* forwardEvent = event->clone();
#ifdef __SST_DEBUG_OUTPUT__
    if (is_debug_event(forwardEvent)) {
        dbg_.debug(_L3_,"BCmd = %s \n", CommandString[(int)forwardEvent->getCmd()]);
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
	if (it != nameMap_.end() ) {
            if (it->second != id)
                dbg_.fatal(CALL_INFO, -1, "%s, Error: Bus attempting to map node that has already been mapped\n", getName().c_str());
            return;
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
    std::string linkprefix = "high_network_";
    std::string linkname = linkprefix + "0";
    while (isPortConnected(linkname)) {
        link = configureLink(linkname, "50 ps", new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if (!link)
            dbg_.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
        highNetPorts_.push_back(link);
        linkIdMap_[highNetPorts_[numHighNetPorts_]->getId()] = highNetPorts_[numHighNetPorts_];
        dbg_.output(CALL_INFO, "Port %d = Link %d\n", numHighNetPorts_, highNetPorts_[numHighNetPorts_]->getId());
        numHighNetPorts_++;
        linkname = linkprefix + std::to_string(numHighNetPorts_);
    }

    linkprefix = "low_network_";
    linkname = linkprefix + "0";
    while (isPortConnected(linkname)) {
        link = configureLink(linkname, "50 ps", new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if (!link)
            dbg_.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
        lowNetPorts_.push_back(link);
        linkIdMap_[lowNetPorts_[numLowNetPorts_]->getId()] = lowNetPorts_[numLowNetPorts_];
        dbg_.output(CALL_INFO, "Port %d = Link %d\n", numLowNetPorts_, lowNetPorts_[numLowNetPorts_]->getId());
        numLowNetPorts_++;
        linkname = linkprefix + std::to_string(numLowNetPorts_);
    }

    if (numLowNetPorts_ < 1 || numHighNetPorts_ < 1) dbg_.fatal(CALL_INFO, -1,"couldn't find number of Ports (numPorts)\n");

}

void Bus::configureParameters(SST::Params& params) {
    int debugLevel = params.find<int>("debug_level", 0);

    dbg_.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)     dbg_.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++)
        DEBUG_ADDR.insert(*it);

    numHighNetPorts_  = 0;
    numLowNetPorts_   = 0;

    latency_      = params.find<uint64_t>("bus_latency_cycles", 1);
    idleMax_      = params.find<uint64_t>("idle_max", 6);
    busFrequency_ = params.find<std::string>("bus_frequency", "Invalid");
    broadcast_    = params.find<bool>("broadcast", 0);
    fanout_       = params.find<bool>("fanout", 0);  /* TODO:  Fanout: Only send messages to lower level caches */
    drain_        = params.find<bool>("drain_bus", 0);

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
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);

            if (memEvent && memEvent->getCmd() == Command::NULLCMD) {
                dbg_.debug(_L10_, "bus %s broadcasting upper event to lower ports (%d): %s\n", getName().c_str(), numLowNetPorts_, memEvent->getVerboseString().c_str());
                mapNodeEntry(memEvent->getSrc(), highNetPorts_[i]->getId());
                for (int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(memEvent->clone());
            } else if (memEvent) {
                dbg_.debug(_L10_, "bus %s broadcasting upper event to lower ports (%d): %s\n", getName().c_str(), numLowNetPorts_, memEvent->getVerboseString().c_str());
                for (int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(memEvent->clone());
            }
            delete memEvent;
        }
    }

    for (int i = 0; i < numLowNetPorts_; i++) {
        while ((ev = lowNetPorts_[i]->recvInitData())) {
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);
            if (!memEvent) delete memEvent;
            else if (memEvent->getCmd() == Command::NULLCMD) {
                dbg_.debug(_L10_, "bus %s broadcasting lower event to upper ports (%d): %s\n", getName().c_str(), numHighNetPorts_, memEvent->getVerboseString().c_str());
                mapNodeEntry(memEvent->getSrc(), lowNetPorts_[i]->getId());
                for (int i = 0; i < numHighNetPorts_; i++) {
                    highNetPorts_[i]->sendInitData(memEvent->clone());
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

