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

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/simulation.h>

#include "trivialCPU.h"

using namespace SST;
using namespace SST::MemHierarchy;


trivialCPU::trivialCPU(ComponentId_t id, Params_t& params) : Component(id)
{

	// get parameters
	if ( params.find("workPerCycle") == params.end() ) {
		_abort(event_test,"couldn't find work per cycle\n");
	}
	workPerCycle = strtol( params[ "workPerCycle" ].c_str(), NULL, 0 );

	if ( params.find("commFreq") == params.end() ) {
		_abort(event_test,"couldn't find communication frequency\n");
	}
	commFreq = strtol( params[ "commFreq" ].c_str(), NULL, 0 );

	maxAddr = (uint32_t)params.find_integer("memSize", -1) -1;
	if ( !maxAddr ) {
		_abort(trivialCPU, "Must set memSize\n");
	}

	do_write = (bool)params.find_integer("do_write", 1);

	// init randomness
	srand(1);

	// tell the simulator not to end without us
	registerExit();

	// configure out links
	mem_link = configureLink( "mem_link",
			new Event::Handler<trivialCPU>(this,
				&trivialCPU::
				handleEvent) );
	assert(mem_link);
	//mem_link->sendInitData("SST::Interfaces::MemEvent");

	registerTimeBase("1 ns", true);
	//set our clock
	registerClock( "1GHz",
			new Clock::Handler<trivialCPU>(this,
				&trivialCPU::clockTic ) );
	num_reads_issued = num_reads_returned = 0;


	// Let the Simulation know we use the interface
    Simulation::getSimulation()->requireEvent("interfaces.MemEvent");

}

trivialCPU::trivialCPU() :
	Component(-1)
{
	// for serialization only
}

// incoming events are scanned and deleted
void trivialCPU::handleEvent(Event *ev)
{
	//printf("recv\n");
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		// May receive invalidates.  Just ignore 'em.
		if ( event->getCmd() == Invalidate ) return;

		std::map<MemEvent::id_type, SimTime_t>::iterator i = requests.find(event->getResponseToID());
		if ( i == requests.end() ) {
			_abort(trivialCPU, "Event (%lu, %d) not found!\n", event->getResponseToID().first, event->getResponseToID().second);
		} else {
			SimTime_t et = getCurrentSimTime() - i->second;
			printf("%s: Received MemEvent with command %d (response to %lu, addr 0x%lx) [Time: %lu]\n",
					getName().c_str(),
					event->getCmd(), event->getResponseToID().first, event->getAddr(), et);
			requests.erase(i);
			num_reads_returned++;
		}

		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool trivialCPU::clockTic( Cycle_t )
{
	// do work
	// loop becomes:
	/*  00001ab5        movl    0xe0(%ebp),%eax
		00001ab8        incl    %eax
		00001ab9        movl    %eax,0xe0(%ebp)
		00001abc        incl    %edx
		00001abd        cmpl    %ecx,%edx
		00001abf        jne     0x00001ab5

		6 instructions.
		*/

	volatile int v = 0;
	for (int i = 0; i < workPerCycle; ++i) {
		v++;
	}

	// communicate?
	if ((rand() % commFreq) == 0) {
		if ( requests.size() > 10 ) {
			printf("%s: Not issuing read.  Too many outstanding requests.\n",
					getName().c_str());
		} else {

			// yes, communicate
			// create event
			// x4 to prevent splitting blocks
			Addr addr = ((((Addr) rand()) % maxAddr)>>2) << 2;

			bool doWrite = do_write && (((rand() % 10) == 0));

			MemEvent *e = new MemEvent(this, addr, doWrite ? WriteReq : ReadReq);
			e->setSize(4); // Load 4 bytes
			if ( doWrite ) {
				e->setPayload(4, (uint8_t*)&addr);
			}
			mem_link->Send(e);
			requests.insert(std::make_pair(e->getID(), getCurrentSimTime()));

			printf("%s: Issued %s (%lu) for address 0x%lx\n",
					getName().c_str(), doWrite ? "Write" : "Read", e->getID().first, addr);
			num_reads_issued++;
		}

	}

	// return false so we keep going
	return false;
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(trivialCPU)


