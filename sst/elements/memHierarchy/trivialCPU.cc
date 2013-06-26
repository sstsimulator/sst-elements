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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "trivialCPU.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;


trivialCPU::trivialCPU(ComponentId_t id, Params_t& params) :
    Component(id), rng(id, 13)
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

    numLS = params.find_integer("num_loadstore", -1);

    uncachedRangeStart = (uint64_t)params.find_integer("uncachedRangeStart", 0);
    uncachedRangeEnd = (uint64_t)params.find_integer("uncachedRangeEnd", 0);


	// tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

	// configure out links
	mem_link = configureLink( "mem_link",
			new Event::Handler<trivialCPU>(this,
				&trivialCPU::
				handleEvent) );
	assert(mem_link);

	registerTimeBase("1 ns", true);
	//set our clock
    clockHandler = new Clock::Handler<trivialCPU>(this, &trivialCPU::clockTic);
	clockTC = registerClock( "1GHz", clockHandler );
	num_reads_issued = num_reads_returned = 0;

}

trivialCPU::trivialCPU() :
	Component(-1)
{
	// for serialization only
}


void trivialCPU::init(unsigned int phase)
{
	if ( !phase ) {
		mem_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}
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
			_abort(trivialCPU, "Event (%"PRIu64", %d) not found!\n", event->getResponseToID().first, event->getResponseToID().second);
		} else {
			SimTime_t et = getCurrentSimTime() - i->second;
			requests.erase(i);
			printf("%s: Received MemEvent with command %d (response to %"PRIu64", addr 0x%"PRIx64") [Time: %"PRIu64"] [%zu outstanding requests]\n",
					getName().c_str(),
					event->getCmd(), event->getResponseToID().first, event->getAddr(), et,
                    requests.size());
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
	volatile int v = 0;
	for (int i = 0; i < workPerCycle; ++i) {
		v++;
	}

	// communicate?
	if ((numLS != 0) && ((rng.generateNextUInt32() % commFreq) == 0)) {
		if ( requests.size() > 10 ) {
			printf("%s: Not issuing read.  Too many outstanding requests.\n",
					getName().c_str());
		} else {

			// yes, communicate
			// create event
			// x4 to prevent splitting blocks
			Addr addr = ((((Addr) rng.generateNextUInt64()) % maxAddr)>>2) << 2;

			bool doWrite = do_write && (((rng.generateNextUInt32() % 10) == 0));

			MemEvent *e = new MemEvent(this, addr, doWrite ? WriteReq : ReadReq);
			e->setSize(4); // Load 4 bytes
			if ( doWrite ) {
				e->setPayload(4, (uint8_t*)&addr);
			}

            bool uncached = ( addr >= uncachedRangeStart && addr < uncachedRangeEnd );
            if ( uncached )
                e->setFlag(MemEvent::F_UNCACHED);

			mem_link->send(e);
			requests.insert(std::make_pair(e->getID(), getCurrentSimTime()));

			printf("%s: %d Issued %s%s (%"PRIu64") for address 0x%"PRIx64"\n",
					getName().c_str(), numLS, uncached ? "Uncached " : "" , doWrite ? "Write" : "Read", e->getID().first, addr);
			num_reads_issued++;

            numLS--;
		}

	}

    if ( numLS == 0 && requests.empty() ) {
        primaryComponentOKToEndSim();
        return true;
    }

	// return false so we keep going
	return false;
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(trivialCPU)


