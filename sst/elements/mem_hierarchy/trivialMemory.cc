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

#include "sst/core/element.h"

#include "trivialMemory.h"

using namespace SST;
using namespace SST::MemHierarchy;

trivialMemory::trivialMemory(ComponentId_t id, Params_t& params) : Component(id)
{

	// get parameters
	memSize = (uint32_t)params.find_integer("memSize", 0);
	if ( !memSize ) _abort(TrivialMem, "no memSize set\n");

	accessTime = (SimTime_t)params.find_integer("accessTime", 1000);

	data = new uint8_t[memSize];

	// tell the simulator not to end without us
	registerExit();

	// configure out links
	bus_link = configureLink( "bus_link",
			new Event::Handler<trivialMemory>(this,
				&trivialMemory::
				handleRequest) );
	registerTimeBase("1 ns", true);
	assert(bus_link);
	bus_requested = false;

}

trivialMemory::trivialMemory() :
	Component(-1)
{
	// for serialization only
}

// incoming events are scanned and deleted
void trivialMemory::handleRequest(Event *ev)
{
	//printf("recv\n");
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		switch (event->getCmd()) {
		case ReadReq:
			handleReadRequest(event);
			break;
		case WriteReq:
			handleWriteRequest(event);
			break;
		case BusClearToSend:
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
	if ( a > memSize ) _abort(TrivialMem, "Bad address 0x%lx\n", a);
	MemEvent *resp = ev->makeResponse();
	for ( uint32_t i = 0 ; i < ev->getSize() ; i++ ) {
		resp->getPayload()[i] = data[a+i];
	}
	sendEvent(resp);
}


void trivialMemory::handleWriteRequest(MemEvent *ev)
{
	Addr a = ev->getAddr();
	if ( a > memSize ) _abort(TrivialMem, "Bad address 0x%lx\n", a);
	MemEvent *resp = ev->makeResponse();
	for ( uint32_t i = 0 ; i < ev->getSize() ; i++ ) {
		data[a+i] = ev->getPayload()[i];
	}
	sendEvent(resp);
}


void trivialMemory::sendBusPacket(void)
{
	assert(reqs.size() > 0);

	std::pair<MemEvent*, SimTime_t> pair = reqs.front();
	reqs.pop_front();
	bus_link->Send(pair.second, pair.first);
	bus_requested = false;
	if ( reqs.size() > 0 ) {
		// Re-request bus
		sendEvent(NULL);
	}
}


void trivialMemory::sendEvent(MemEvent *ev, SimTime_t delay)
{
	if ( ev != NULL ) {
		reqs.push_back(std::make_pair<MemEvent*, SimTime_t>(ev, delay));
	}
	if (!bus_requested) {
		bus_link->Send(new MemEvent(NULL, RequestBus));
		bus_requested = true;
	}
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(trivialMemory)

