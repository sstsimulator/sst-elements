// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

        if (!drain_ )
            break;
    }

    return false;
}


void Bus::broadcastEvent(SST::Event* ev) {
    MemEventBase* memEvent = static_cast<MemEventBase*>(ev);
    SST::Link* srcLink = lookupNode(memEvent->getSrc());

    for (int i = 0; i < numHighPorts_; i++) {
        if (highNetPorts_[i] == srcLink) continue;
        highNetPorts_[i]->send(memEvent->clone());
    }

    for (int i = 0; i < numLowPorts_; i++) {
        if (lowNetPorts_[i] == srcLink) continue;
        lowNetPorts_[i]->send(memEvent->clone());
    }

    delete memEvent;
}



void Bus::sendSingleEvent(SST::Event* ev) {
    MemEventBase *event = static_cast<MemEventBase*>(ev);
#ifdef __SST_DEBUG_OUTPUT__
    if (is_debug_event(event)) {
        dbg_.debug(_L3_, "E: %-20" PRIu64 " %-20" PRId32 " %-20s Event:New     (%s)\n",
                getCurrentSimCycle(), 0, getName().c_str(), event->getVerboseString().c_str());
        fflush(stdout);
    }
#endif
    SST::Link* dstLink = lookupNode(event->getDst());
    MemEventBase* forwardEvent = event->clone();
    dstLink->send(forwardEvent);

    delete event;
}

/*----------------------------------------
 * Helper functions
 *---------------------------------------*/

void Bus::mapNodeEntry(const std::string& name, SST::Link* link) {
    std::map<std::string, SST::Link*>::iterator it = nameMap_.find(name);
    if (it != nameMap_.end() ) {
        if (it->second != link)
            dbg_.fatal(CALL_INFO, -1, "%s, Error: Bus attempting to map node that has already been mapped\n", getName().c_str());
        return;
    }
    nameMap_[name] = link;
}

SST::Link* Bus::lookupNode(const std::string& name) {
    std::map<std::string, SST::Link*>::iterator it = nameMap_.find(name);
    if (nameMap_.end() == it) {
        dbg_.fatal(CALL_INFO, -1, "%s, Error: Bus lookup of node %s returned no mapping\n", getName().c_str(), name.c_str());
    }
    return it->second;
}

void Bus::configureLinks() {
    std::string linkprefix = "highlink";
    if (isPortConnected("high_network_0")) {
        dbg_.output("%s, DEPRECATION WARNING: The 'high_network_%%d' ports on MemHierarchy Buses have been renamed to 'highlink%%d'. MemHierarchy port names are being standardized. The 'high_network_%%d' ports will be removed in SST 16.\n", getName().c_str());
        linkprefix = "high_network_"; 
    }
    
    SST::Link* link;
    std::string linkname = linkprefix + "0";
    while (isPortConnected(linkname)) {
        link = configureLink(linkname, new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if (!link)
            dbg_.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
        highNetPorts_.push_back(link);
        numHighPorts_++;
        linkname = linkprefix + std::to_string(numHighPorts_);
    }
    
    linkprefix = "lowlink";
    if (isPortConnected("low_network_0")) {
        dbg_.output("%s, DEPRECATION WARNING: The 'low_network_%%d' ports on MemHierarchy Buses have been renamed to 'lowlink%%d'. MemHierarchy port names are being standardized. The 'low_network_%%d' ports will be removed in SST 16.\n", getName().c_str());
        linkprefix = "low_network_"; 
    }
    linkname = linkprefix + "0";
    while (isPortConnected(linkname)) {
        link = configureLink(linkname, new Event::Handler<Bus>(this, &Bus::processIncomingEvent));
        if (!link)
            dbg_.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
        lowNetPorts_.push_back(link);
        numLowPorts_++;
        linkname = linkprefix + std::to_string(numLowPorts_);
    }

    if (numLowPorts_ < 1 || numHighPorts_ < 1) dbg_.fatal(CALL_INFO, -1,"couldn't find number of Ports (numPorts)\n");

}

void Bus::configureParameters(SST::Params& params) {
    int debugLevel = params.find<int>("debug_level", 0);

    dbg_.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)     dbg_.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++)
        DEBUG_ADDR.insert(*it);

    numHighPorts_  = 0;
    numLowPorts_   = 0;

    idleMax_      = params.find<uint64_t>("idle_max", 6);
    std::string frequency = params.find<std::string>("bus_frequency", "Invalid");
    broadcast_    = params.find<bool>("broadcast", 0);
    drain_        = params.find<bool>("drain_bus", false);

    if (frequency == "Invalid") dbg_.fatal(CALL_INFO, -1, "Bus Frequency was not specified\n");
    
     /* Multiply Frequency times two.  This is because an SST Bus components has
        2 SST Links (highNEt & LowNet) and thus it takes a least 2 cycles for any
        transaction (a real bus should be allowed to have 1 cycle latency).  To overcome
        this we clock the bus 2x the speed of the cores */

    UnitAlgebra uA = UnitAlgebra(frequency);
    uA = uA * 2;
    frequency = uA.toString();

    clockHandler_ = new Clock::Handler<Bus>(this, &Bus::clockTick);
    defaultTimeBase_ = registerClock(frequency, clockHandler_);
}

void Bus::init(unsigned int phase) {
    SST::Event *ev;

    for (int i = 0; i < numHighPorts_; i++) {
        while ((ev = highNetPorts_[i]->recvUntimedData())) {
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);

            if (memEvent && memEvent->getCmd() == Command::NULLCMD) {
                dbg_.debug(_L10_, "bus %s broadcasting upper event to lower ports (%d): %s\n", getName().c_str(), numLowPorts_, memEvent->getVerboseString().c_str());
                mapNodeEntry(memEvent->getSrc(), highNetPorts_[i]);
                
                if (memEvent->getInitCmd() == MemEventInit::InitCommand::Region) {
                    MemEventInitRegion * mEvReg = static_cast<MemEventInitRegion*>(memEvent);
                    mEvReg->setGroup(MemEventInitRegion::ReachableGroup::Source);

                    for (int k = 0; k < numLowPorts_; k++) {
                        lowNetPorts_[k]->sendUntimedData(memEvent->clone());
                    }

                    mEvReg->setGroup(MemEventInitRegion::ReachableGroup::Peer);
                    for (int k = 0; k < numHighPorts_; k++) {
                        if (k == i) continue;
                        highNetPorts_[k]->sendUntimedData(memEvent->clone());
                    }
                } else {
                    for (int k = 0; k < numLowPorts_; k++) {
                        lowNetPorts_[k]->sendUntimedData(memEvent->clone()); 
                    }
                }
            } else if (memEvent) {
                dbg_.debug(_L10_, "bus %s broadcasting upper event to lower ports (%d): %s\n", getName().c_str(), numLowPorts_, memEvent->getVerboseString().c_str());
                for (int k = 0; k < numLowPorts_; k++)
                    lowNetPorts_[k]->sendUntimedData(memEvent->clone());
            }
            delete memEvent;
        }
    }

    for (int i = 0; i < numLowPorts_; i++) {
        while ((ev = lowNetPorts_[i]->recvUntimedData())) {
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);
            if (!memEvent) delete memEvent;
            else if (memEvent->getCmd() == Command::NULLCMD) {
                dbg_.debug(_L10_, "bus %s broadcasting lower event to upper ports (%d): %s\n", getName().c_str(), numHighPorts_, memEvent->getVerboseString().c_str());
                mapNodeEntry(memEvent->getSrc(), lowNetPorts_[i]);
                
                if (memEvent->getInitCmd() == MemEventInit::InitCommand::Region) {
                    MemEventInitRegion * mEvReg = static_cast<MemEventInitRegion*>(memEvent);
                    mEvReg->setGroup(MemEventInitRegion::ReachableGroup::Dest);

                    for (int k = 0; k < numHighPorts_; k++) {
                        highNetPorts_[k]->sendUntimedData(memEvent->clone());
                    }

                    mEvReg->setGroup(MemEventInitRegion::ReachableGroup::Peer);
                    for (int k = 0; k < numLowPorts_; k++) {
                        if (k == i) continue;
                        lowNetPorts_[k]->sendUntimedData(memEvent->clone());
                    }
                } else {
                    for (int i = 0; i < numHighPorts_; i++) {
                        highNetPorts_[i]->sendUntimedData(memEvent->clone());
                    }
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

