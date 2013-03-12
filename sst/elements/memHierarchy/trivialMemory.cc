// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "trivialMemory.h"


#define DPRINTF( fmt, args...) __DBG( DBG_MEMORY, Memory, "%s: " fmt, getName().c_str(), ## args )

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

trivialMemory::trivialMemory(ComponentId_t id, Params_t& params) : Component(id)
{

	// get parameters
	memSize = (uint32_t)params.find_integer("memSize", 0);
	if ( !memSize ) _abort(TrivialMem, "no memSize set\n");

	data = new uint8_t[memSize];

	// configure out links
	bus_link = configureLink( "bus_link", "50 ps",
			new Event::Handler<trivialMemory>(this,
				&trivialMemory::
				handleRequest) );
	assert(bus_link);

	self_link = configureSelfLink("Self", params.find_string("accessTime", "1000 ns"),
			new Event::Handler<trivialMemory>(this, &trivialMemory::handleSelfEvent));

	registerTimeBase("1 ns", true);
	bus_requested = false;

}

trivialMemory::trivialMemory() :
	Component(-1)
{
	// for serialization only
}


void trivialMemory::init(unsigned int phase)
{
	if ( !phase ) {
		bus_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}

	SST::Event *ev = NULL;
	while ( (ev = bus_link->recvInitData()) != NULL ) {
		MemEvent *me = dynamic_cast<MemEvent*>(ev);
		if ( me ) {
			/* Push data to memory */
			if ( me->getCmd() == WriteReq ) {
				for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
					data[me->getAddr() + i] = me->getPayload()[i];
				}
			} else {
				printf("TrivialMemory received unexpected Init Command: %d\n", me->getCmd() );
			}
		}
		delete ev;
	}
}



void trivialMemory::handleSelfEvent(Event *ev)
{
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	assert(event);
	if ( !canceled(event) )
		sendEvent(event);
}


// incoming events are scanned and deleted
void trivialMemory::handleRequest(Event *ev)
{
	//printf("recv\n");
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		bool to_me = ( event->getDst() == getName() || event->getDst() == BROADCAST_TARGET );
		switch (event->getCmd()) {
		case RequestData:
		case ReadReq:
			if ( to_me )
				handleReadRequest(event);
			break;
		case ReadResp:
			if ( event->getSrc() != getName() ) // don't cancel from our own response.
				cancelEvent(event);
			break;
		case WriteReq:
		case SupplyData:
			if ( event->queryFlag(MemEvent::F_WRITEBACK) )
				handleWriteRequest(event);
			else
				cancelEvent(event);

			break;
		case BusClearToSend:
			if ( to_me )
				sendBusPacket();
			break;
		default:
			/* Ignore */
			break;
		}

		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}


void trivialMemory::handleReadRequest(MemEvent *ev)
{
	Addr a = ev->getAddr();
	DPRINTF("Read for addr 0x%lx\n", a);
	if ( a > memSize ) _abort(TrivialMem, "Bad address 0x%lx\n", a);
	MemEvent *resp = ev->makeResponse(this);
	for ( uint32_t i = 0 ; i < ev->getSize() ; i++ ) {
		resp->getPayload()[i] = data[a+i];
	}
	outstandingReqs[ev->getAddr()] = false;
	self_link->Send(resp);
}


void trivialMemory::handleWriteRequest(MemEvent *ev)
{
	Addr a = ev->getAddr();
	DPRINTF("Write for addr 0x%lx\n", a);
	if ( a > memSize ) _abort(TrivialMem, "Bad address 0x%lx\n", a);
	for ( uint32_t i = 0 ; i < ev->getSize() ; i++ ) {
		data[a+i] = ev->getPayload()[i];
	}
	outstandingReqs[ev->getAddr()] = false;
	MemEvent *resp = ev->makeResponse(this);
	if ( resp->getCmd() != NULLCMD ) {
		self_link->Send(resp);
	} else {
		delete resp;
	}
}


void trivialMemory::sendBusPacket(void)
{
	for (;;) {
		if ( reqs.size() == 0 ) {
			bus_link->Send(new MemEvent(this, NULL, CancelBusRequest));
			bus_requested = false;
			break;
		} else {
			MemEvent *ev = reqs.front();
			reqs.pop_front();
			if ( !canceled(ev) ) {
				DPRINTF("Sending (%lu, %d) in response to (%lu, %d) 0x%lx\n",
						ev->getID().first, ev->getID().second,
						ev->getResponseToID().first, ev->getResponseToID().second,
						ev->getAddr());
				bus_link->Send(0, ev);
				bus_requested = false;
				if ( reqs.size() > 0 ) {
					// Re-request bus
					sendEvent(NULL);
				}
				break;
			}
				outstandingReqs.erase(ev->getAddr());
		}
	}
}


void trivialMemory::sendEvent(MemEvent *ev)
{
	if ( ev != NULL ) {
		reqs.push_back(ev);
	}
	if (!bus_requested) {
		bus_link->Send(new MemEvent(this, NULL, RequestBus));
		bus_requested = true;
	}
}


bool trivialMemory::canceled(MemEvent *ev)
{
	std::map<Addr, bool>::iterator i = outstandingReqs.find(ev->getAddr());
	if ( i == outstandingReqs.end() ) return false;
	return i->second;
}


void trivialMemory::cancelEvent(MemEvent* ev)
{
	std::map<Addr, bool>::iterator i = outstandingReqs.find(ev->getAddr());
	DPRINTF("Looking to cancel (0x%lx)\n", ev->getAddr());
	if ( i != outstandingReqs.end() ) {
		outstandingReqs[ev->getAddr()] = true;
	}
}




BOOST_CLASS_EXPORT(trivialMemory)

