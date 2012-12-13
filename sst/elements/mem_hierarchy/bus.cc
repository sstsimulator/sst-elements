// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sstream>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "bus.h"
#include "memEvent.h"

#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, Bus, fmt, ## args )

using namespace SST;
using namespace SST::MemHierarchy;

static const LinkId_t BUS_INACTIVE = (LinkId_t)(-2);

Bus::Bus(ComponentId_t id, Params_t& params) :
	Component(id)
{

	// get parameters
	numPorts = params.find_integer("numPorts", 0);
	if ( numPorts < 1 ) _abort(Bus,"couldn't find number of Ports (numPorts)\n");
	activePort = BUS_INACTIVE;
	busBusy = false;

	// TODO:  Use proper time conversion
	busDelay = params.find_integer("busDelay", 0);
	if ( busDelay < 1 ) _abort(Bus,"couldn't find bus delay (busDelay)\n");

	// tell the simulator not to end without us
	registerExit();

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


void Bus::requestPort(LinkId_t link_id)
{
	DPRINTF("(%lu) [active = %lu]\n", link_id, activePort);

	busRequests.push_back(link_id);

	if ( activePort == BUS_INACTIVE ) {
		// Nobody's active.  Schedule it.
		selfLink->Send(new SelfEvent(SelfEvent::Schedule));
	}
}


void Bus::cancelPortRequest(LinkId_t link_id)
{
	DPRINTF("(%lu) [active = %lu]\n", link_id, activePort);
	// You don't cancel a request until you're the owner
	assert(activePort == link_id);
	// You don't cancel a request if you're already sending a msg
	assert(!busBusy);

	activePort = BUS_INACTIVE;
	// We're cleared now
	selfLink->Send(new SelfEvent(SelfEvent::Schedule));
}


void Bus::sendMessage(MemEvent *ev, LinkId_t from_link)
{
	DPRINTF("(%s -> %s) [active = %lu]\n", ev->getSrc().c_str(), ev->getDst().c_str(), activePort);
	// Only should be sending data if have clear-to-send
	assert(from_link == activePort);
	// Can't send while already busy
	assert(!busBusy);

	// TODO:  Calcuate delay including message size

	for ( int i = 0 ; i < numPorts ; i++ ) {
		ports[i]->Send(busDelay, new MemEvent(ev));
	}
	selfLink->Send(busDelay, new SelfEvent(SelfEvent::BusFinish));
	busBusy = true;
}


LinkId_t Bus::arbitrateNext(void)
{
	if ( busRequests.size() > 0 ) {
		int id = busRequests.front();
		busRequests.pop_front();
		return id;
	}
	return BUS_INACTIVE;
}



// incoming events are scanned and deleted
void Bus::handleEvent(Event *ev) {
	//printf("recv\n");
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		LinkId_t link_id = event->getLinkId();
		switch(event->getCmd())
		{
		case RequestBus:
			requestPort(link_id);
			break;
		case CancelBusRequest:
			cancelPortRequest(link_id);
			break;
		default:
			// All other messages.  Send on bus
			sendMessage(event, link_id);
			break;
		}
		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}



void Bus::schedule(void)
{
	if ( activePort != BUS_INACTIVE || busBusy ) {
		// Most likely, two people asked to schedule in the same cycle.
		return;
	}

	LinkId_t next_id = arbitrateNext();
	if ( next_id != BUS_INACTIVE ) {
		activePort = next_id;
		DPRINTF("Setting activePort = %lu\n", activePort);
		linkMap[next_id]->Send(new MemEvent(getName(), NULL, BusClearToSend));
	}
}


void Bus::busFinish(void)
{
	DPRINTF("\n");
	busBusy = false;
	activePort = BUS_INACTIVE;
	schedule();
}


// incoming events are scanned and deleted
void Bus::handleSelfEvent(Event *ev) {
	//printf("recv\n");
	SelfEvent *event = dynamic_cast<SelfEvent*>(ev);
	if (event) {
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

