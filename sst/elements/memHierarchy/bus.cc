// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sstream>
#include <assert.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/memEvent.h>

#include "bus.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

static const LinkId_t BUS_INACTIVE = (LinkId_t)(-2);
const char Bus::BUS_INFO_STR[] = "SST::MemHierarchy::Bus::Info:";
const Bus::key_t Bus::ANY_KEY = std::pair<uint64_t, int>((uint64_t)-1, -1);

Bus::Bus(ComponentId_t id, Params_t& params) :
	Component(id)
{
	// get parameters
    dbg.init("@t:Bus::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));
	numPorts = params.find_integer("numPorts", 0);
	if ( numPorts < 1 ) _abort(Bus,"couldn't find number of Ports (numPorts)\n");
	activePort.first = BUS_INACTIVE;
	busBusy = false;


	std::string delay = params.find_string("busDelay", "100 ns");
	delayTC = registerTimeBase(delay, false);
	busDelay = 1;

    atomicDelivery = (0 != params.find_integer("atomicDelivery", 0));

	ports = new SST::Link*[numPorts];

	for ( int i = 0 ; i < numPorts ; i++ ) {
		std::ostringstream linkName;
		linkName << "port" << i;
		std::string ln = linkName.str();
		ports[i] = configureLink(ln, "50 ps",
				new Event::Handler<Bus>(this, &Bus::handleEvent));
		//ports[i]->setDefaultTimeBase(registerTimeBase("1 ns"));
		assert(ports[i]);
		linkMap[ports[i]->getId()] = ports[i];
		dbg.output(CALL_INFO, "Port %lu = Link %d\n", ports[i]->getId(), i);
	}

	selfLink = configureSelfLink("Self", "50 ps",
				new Event::Handler<Bus>(this, &Bus::handleSelfEvent));

}

Bus::Bus() :
	Component(-1)
{
	// for serialization only
}


void Bus::init(unsigned int phase)
{
	if ( !phase ) {
        char buf[512];
        sprintf(buf, "%s\tNumACKPeers: %d", BUS_INFO_STR, atomicDelivery ? 1 : numPorts);
        std::string busInfo(buf);
		for ( int i = 0 ; i < numPorts ; i++ ) {
			ports[i]->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
            ports[i]->sendInitData(new StringEvent(busInfo));
		}
	}

	/* Pass on any messages */
	for ( int i = 0 ; i < numPorts ; i++ ) {
		SST::Event *ev = NULL;
		while ( NULL != (ev = ports[i]->recvInitData()) ) {
			MemEvent *me = dynamic_cast<MemEvent*>(ev);
			if ( me ) {
				for ( int j = 0 ; j < numPorts ; j++ ) {
					if ( i == j ) continue;
					ports[j]->sendInitData(new MemEvent(me));
				}
			}
			delete ev;
		}
	}

}


void Bus::printStatus(Output &out)
{
    out.output("MemHierarchy::Bus %s\n", getName().c_str());
    out.output("\tStatus: %s\n", busBusy ? "BUSY" : "IDLE");
    if ( busBusy ) {
        out.output("\tCurrent Message:\tLink %ld,  key:  (%"PRIu64", %d)\n",
                activePort.first, activePort.second.first, activePort.second.second);
    }
    out.output("\tQueue Depth:\t%zu\n", busRequests.size());
    for ( std::deque<std::pair<LinkId_t, key_t> >::iterator i = busRequests.begin() ; i != busRequests.end() ; ++i ) {
        out.output("\t\tLink %ld,  key:  (%"PRIu64", %d)\n", i->first, i->second.first, i->second.second);
    }


}

void Bus::requestPort(LinkId_t link_id, key_t key)
{

	busRequests.push_back(std::make_pair(link_id, key));
	dbg.output(CALL_INFO, "(%lu, (%"PRIu64", %d)) [active = %lu] queue depth = %zu \n", link_id, key.first, key.second, activePort.first, busRequests.size());

	if ( BUS_INACTIVE == activePort.first ) {
		// Nobody's active.  Schedule it.
		selfLink->send(new SelfEvent(SelfEvent::Schedule));
	}
}


void Bus::cancelPortRequest(LinkId_t link_id, key_t key)
{
	dbg.output(CALL_INFO, "(%lu, (%"PRIu64", %d)) [active = %lu]\n", link_id, key.first, key.second, activePort.first);

    if ( link_id == activePort.first && (key == activePort.second || ANY_KEY == key )) {
        dbg.output(CALL_INFO, "Canceling active.  Rescheduling\n");
        activePort.first = BUS_INACTIVE;
        selfLink->send(new SelfEvent(SelfEvent::Schedule));
    }

    for ( std::deque<std::pair<LinkId_t, key_t> >::iterator i = busRequests.begin() ; i != busRequests.end() ; ++i ) {
        if ( i->first == link_id && i->second == key) {
            busRequests.erase(i);
            dbg.output(CALL_INFO, "Canceling (%lu, (%"PRIu64", %d)\n", link_id, key.first, key.second);
            break;
        }
    }


}


void Bus::sendMessage(BusEvent *ev, LinkId_t from_link)
{
    dbg.output(CALL_INFO, "(%s -> %s: (%"PRIu64", %d) %s 0x%"PRIx64") [active = %lu]\n",
            ev->payload->getSrc().c_str(), ev->payload->getDst().c_str(),
            ev->payload->getID().first, ev->payload->getID().second,
            CommandString[ev->payload->getCmd()], ev->payload->getAddr(),
            activePort.first);
    // Only should be sending data if have clear-to-send
    if ( from_link != activePort.first ) {
        _abort(Bus, "Port %ld tried talking, but %ld is active.\n", from_link, activePort.first);
    }
    // Can't send while already busy
    assert(!busBusy);

    if ( ev->key != activePort.second ) {
        _abort(Bus, "Port %ld sent us key (%"PRIu64", %d), but key (%"PRIu64", %d) is active.\n",
                from_link, ev->key.first, ev->key.second, activePort.second.first, activePort.second.second);
    }

    // TODO:  Calcuate delay including message size
    for ( int i = 0 ; i < numPorts ; i++ ) {
        ports[i]->send(busDelay, delayTC, new BusEvent(new MemEvent(ev->payload)));
    }
    //delete ev->payload;
    selfLink->send(busDelay, delayTC, new SelfEvent(SelfEvent::BusFinish));
    busBusy = true;
}


std::pair<LinkId_t, Bus::key_t> Bus::arbitrateNext(void)
{
	if ( busRequests.size() > 0 ) {
        std::pair<LinkId_t, key_t> id = busRequests.front();
		busRequests.pop_front();
		return id;
	}
	return std::make_pair(BUS_INACTIVE, ANY_KEY);
}



// incoming events are scanned and deleted
void Bus::handleEvent(Event *ev) {
	BusEvent *event = static_cast<BusEvent*>(ev);
    LinkId_t link_id = event->getLinkId();
    switch(event->cmd)
    {
    case BusEvent::RequestBus:
        requestPort(link_id, event->key);
        break;
    case BusEvent::CancelRequest:
        cancelPortRequest(link_id, event->key);
        break;
    case BusEvent::SendData:
        sendMessage(event, link_id);
        delete event->payload;
        event->payload = NULL;
        break;
    default:
        _abort(Bus, "Bus should never receive a ClearToSend.\n");
    }
    delete event;
}



void Bus::schedule(void)
{
	if ( BUS_INACTIVE != activePort.first || busBusy ) {
		// Most likely, two people asked to schedule in the same cycle.
		return;
	}

    std::pair<LinkId_t, key_t> next_id = arbitrateNext();
	if ( BUS_INACTIVE != next_id.first ) {
		activePort = next_id;
		dbg.output(CALL_INFO, "Setting activePort = (%lu, (%"PRIu64", %d))\n", activePort.first, activePort.second.first, activePort.second.second);
		linkMap[next_id.first]->send(new BusEvent(BusEvent::ClearToSend, activePort.second));
	}
}


void Bus::busFinish(void)
{
	dbg.output(CALL_INFO, "\n");
	busBusy = false;
	activePort.first = BUS_INACTIVE;
	schedule();
}


// incoming events are scanned and deleted
void Bus::handleSelfEvent(Event *ev) {
	SelfEvent *event = dynamic_cast<SelfEvent*>(ev);
	if (event) {
        dbg.output(CALL_INFO, "Received selfEvent type %d\n", event->type);
		switch(event->type)
		{
		case SelfEvent::Schedule:
			schedule();
			break;
		case SelfEvent::BusFinish:
			busFinish();
			break;
		}
		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}


// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(Bus)
BOOST_CLASS_EXPORT(BusEvent)

