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

#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, Bus, fmt, ## args )

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

static const LinkId_t BUS_INACTIVE = (LinkId_t)(-2);
const char Bus::BUS_INFO_STR[] = "SST::MemHierarchy::Bus::Info:";

Bus::Bus(ComponentId_t id, Params_t& params) :
	Component(id)
{
	// get parameters
	numPorts = params.find_integer("numPorts", 0);
	if ( numPorts < 1 ) _abort(Bus,"couldn't find number of Ports (numPorts)\n");
	activePort.first = BUS_INACTIVE;
	busBusy = false;


	std::string delay = params.find_string("busDelay", "100 ns");
	delayTC = registerTimeBase(delay, false);
	busDelay = 1;

    atomicDelivery = params.find_integer("atomicDelivery", 0) != 0;

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
		DPRINTF("Port %lu = Link %d\n", ports[i]->getId(), i);
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
		while ( (ev = ports[i]->recvInitData()) != NULL ) {
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


void Bus::requestPort(LinkId_t link_id, Addr key)
{

	busRequests.push_back(std::make_pair(link_id, key));
	DPRINTF("(%lu, 0x%"PRIx64") [active = %lu] queue depth = %zu \n", link_id, key, activePort.first, busRequests.size());

	if ( activePort.first == BUS_INACTIVE ) {
		// Nobody's active.  Schedule it.
		selfLink->send(new SelfEvent(SelfEvent::Schedule));
	}
}


void Bus::cancelPortRequest(LinkId_t link_id, Addr key)
{
	DPRINTF("(%lu, 0x%"PRIx64") [active = %lu]\n", link_id, key, activePort.first);

    if ( link_id == activePort.first && (key == activePort.second || key == 0)) {
        DPRINTF("Canceling active.  Rescheduling\n");
        activePort.first = BUS_INACTIVE;
        selfLink->send(new SelfEvent(SelfEvent::Schedule));
    }

    for ( std::deque<std::pair<LinkId_t, Addr> >::iterator i = busRequests.begin() ; i != busRequests.end() ; ++i ) {
        if ( i->first == link_id && i->second == key) {
            busRequests.erase(i);
            DPRINTF("Canceling (%lu, 0x%"PRIx64"\n", link_id, key);
            break;
        }
    }


}


void Bus::sendMessage(MemEvent *ev, LinkId_t from_link)
{
    DPRINTF("(%s -> %s: (%"PRIu64", %d) %s 0x%"PRIx64") [active = %lu]\n",
            ev->getSrc().c_str(), ev->getDst().c_str(),
            ev->getID().first, ev->getID().second,
            CommandString[ev->getCmd()], ev->getAddr(),
            activePort.first);
    // Only should be sending data if have clear-to-send
    if ( from_link != activePort.first ) {
        _abort(Bus, "Port %ld tried talking, but %ld is active.\n", from_link, activePort.first);
    }
    // Can't send while already busy
    assert(!busBusy);

    // TODO:  Calcuate delay including message size

    for ( int i = 0 ; i < numPorts ; i++ ) {
        ports[i]->send(busDelay, delayTC, new MemEvent(ev));
    }
    selfLink->send(busDelay, delayTC, new SelfEvent(SelfEvent::BusFinish));
    busBusy = true;
}


std::pair<LinkId_t, Addr> Bus::arbitrateNext(void)
{
	if ( busRequests.size() > 0 ) {
        std::pair<LinkId_t, Addr> id = busRequests.front();
		busRequests.pop_front();
		return id;
	}
	return std::make_pair(BUS_INACTIVE, 0);
}



// incoming events are scanned and deleted
void Bus::handleEvent(Event *ev) {
	//printf("recv\n");
	MemEvent *event = static_cast<MemEvent*>(ev);
    LinkId_t link_id = event->getLinkId();
    switch(event->getCmd())
    {
    case RequestBus:
        requestPort(link_id, event->getAddr());
        break;
    case CancelBusRequest:
        cancelPortRequest(link_id, event->getAddr());
        break;
    default:
        // All other messages.  Send on bus
        sendMessage(event, link_id);
        break;
    }
    delete event;
}



void Bus::schedule(void)
{
	if ( activePort.first != BUS_INACTIVE || busBusy ) {
		// Most likely, two people asked to schedule in the same cycle.
		return;
	}

    std::pair<LinkId_t, Addr> next_id = arbitrateNext();
	if ( next_id.first != BUS_INACTIVE ) {
		activePort = next_id;
		DPRINTF("Setting activePort = (%lu, 0x%"PRIx64")\n", activePort.first, activePort.second);
		linkMap[next_id.first]->send(new MemEvent(this, next_id.second, BusClearToSend));
	}
}


void Bus::busFinish(void)
{
	DPRINTF("\n");
	busBusy = false;
	activePort.first = BUS_INACTIVE;
	schedule();
}


// incoming events are scanned and deleted
void Bus::handleSelfEvent(Event *ev) {
	//printf("recv\n");
	SelfEvent *event = dynamic_cast<SelfEvent*>(ev);
	if (event) {
        DPRINTF("Received selfEvent type %d\n", event->type);
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

