// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "streamCPU.h"

#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include "memEvent.h"


using namespace SST;
using namespace SST::MemHierarchy;


streamCPU::streamCPU(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    out.init("", 0, 0, Output::NONE);

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
		_abort(streamCPU, "Must set memSize\n");
	}

	do_write = (bool)params.find_integer("do_write", 1);

    numLS = params.find_integer("num_loadstore", -1);


	// tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

	// configure out links
	mem_link = configureLink( "mem_link",
			new Event::Handler<streamCPU>(this,
				&streamCPU::
				handleEvent) );
	assert(mem_link);

	registerTimeBase("1 ns", true);
	//set our clock
    clockHandler = new Clock::Handler<streamCPU>(this, &streamCPU::clockTic);
	clockTC = registerClock( "1GHz", clockHandler );
	num_reads_issued = num_reads_returned = 0;
	nextAddr = 0;
}

streamCPU::streamCPU() :
	Component(-1)
{
	// for serialization only
}


void streamCPU::init(unsigned int phase)
{
	if ( !phase ) {
		mem_link->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
	}
}

// incoming events are scanned and deleted
void streamCPU::handleEvent(Event *ev)
{
	//out.output("recv\n");
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		// May receive invalidates.  Just ignore 'em.
		if ( event->getCmd() == Inv) return;

		std::map<MemEvent::id_type, SimTime_t>::iterator i = requests.find(event->getResponseToID());
		if ( i == requests.end() ) {
			_abort(streamCPU, "Event (%"PRIx64", %d) not found!\n", event->getResponseToID().first, event->getResponseToID().second);
		} else {
			SimTime_t et = getCurrentSimTime() - i->second;
			requests.erase(i);
			out.output("%s: Received MemEvent with command %d (response to %"PRIx64", addr 0x%"PRIx64") [Time: %" PRIx64 "] [%zu outstanding requests]\n",
					getName().c_str(),
					event->getCmd(), event->getResponseToID().first, event->getAddr(), et,
                    requests.size());
			num_reads_returned++;
		}

		delete event;
	} else {
		out.output("Error! Bad Event Type!\n");
	}
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool streamCPU::clockTic( Cycle_t )
{
	volatile int v = 0;
	for (int i = 0; i < workPerCycle; ++i) {
		v++;
	}

	// communicate?
	if ((numLS != 0) && ((rng.generateNextUInt32() % commFreq) == 0)) {
		if ( requests.size() > 10 ) {
			out.output("%s: Not issuing read.  Too many outstanding requests.\n",
					getName().c_str());
		} else {

			// yes, communicate
			// create event
			// x4 to prevent splitting blocks
			//Addr addr = ((((Addr) rng.generateNextUInt64()) % maxAddr)>>2) << 2;

			bool doWrite = do_write && (((rng.generateNextUInt32() % 10) == 0));

			MemEvent *e = new MemEvent(this, nextAddr, nextAddr, doWrite ? GetX : GetS);
			e->setSize(4); // Load 4 bytes
			if ( doWrite ) {
				e->setPayload(4, (uint8_t*)&nextAddr);
			}
			mem_link->send(e);
			requests.insert(std::make_pair(e->getID(), getCurrentSimTime()));

			//out.output("%s: %d Issued %s (%"PRIu64") for address 0x%""\n",
			//		getName().c_str(), numLS, doWrite ? "Write" : "Read", e->getID().first, nextAddr);
			std::cout << getName() << " " << numLS << " issued: " <<
				(doWrite ? "write" : "read") << " (id=" << e->getID().first << ", Addr=" << nextAddr <<
				std::endl;
			num_reads_issued++;
			nextAddr = (nextAddr + 8);

			if(nextAddr > maxAddr) {
				nextAddr = 0;
			}

	        	numLS--;
		}

	}

    if ( numLS == 0 && requests.size() == 0 ) {
        primaryComponentOKToEndSim();
        return true;
    }

	// return false so we keep going
	return false;
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(streamCPU)


