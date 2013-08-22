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

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "cpu_data.h"

#include <assert.h>

#include "sst/core/debug.h"
#include "sst/core/element.h"
#include "sst/core/params.h"

#include "simpleEvent.h"

using namespace SST;
using namespace SST::CPU_data;

Cpu_data::Cpu_data(ComponentId_t id, Params& params) :
  IntrospectedComponent(id) {

  // get parameters
  if ( params.find("workPerCycle") == params.end() ) {
    _abort(event_test,"couldn't find work per cycle\n");
  }
  workPerCycle = strtol( params[ "workPerCycle" ].c_str(), NULL, 0 );

  if ( params.find("commFreq") == params.end() ) {
    _abort(event_test,"couldn't find communication frequency\n");
  }
  commFreq = strtol( params[ "commFreq" ].c_str(), NULL, 0 );

  if ( params.find("commSize") == params.end() ) {
    _abort(event_test,"couldn't find communication size\n");
  }
  commSize = strtol( params[ "commSize" ].c_str(), NULL, 0 );
  if (params.find("push_introspector") != params.end() ) {
  pushIntrospector = params[ "push_introspector" ];
  }
        
 
  // init randomness
  srand(1);
  neighbor = rand() % 4;

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  // configure out links
  N = configureLink( "Nlink", 
		     new Event::Handler<Cpu_data>(this,
							 &Cpu_data::
							 handleEvent) );
  S = configureLink( "Slink", 
		     new Event::Handler<Cpu_data>(this,
							 &Cpu_data::
							 handleEvent) );
  E = configureLink( "Elink", 
		     new Event::Handler<Cpu_data>(this,
							 &Cpu_data::
							 handleEvent) );
  W = configureLink( "Wlink", 
		     new Event::Handler<Cpu_data>(this,
							 &Cpu_data::
							 handleEvent) );
  assert(N);
  assert(S);
  assert(E);
  assert(W);

  //set our clock
  registerClock( "1GHz", 
		 new Clock::Handler<Cpu_data>(this, 
						     &Cpu_data::clockTic ) );

  //for introspection
  counts = 0;
	    num_il1_read = 0;
	    num_branch_read = 0;
	    num_branch_write = 0;
	    num_RAS_read = 0;
	    num_RAS_write = 0;
	    if (getId() == 2)
        	mycore_temperature = 360.5;
    	    else
		mycore_temperature = 300.1;
}

Cpu_data::Cpu_data() :
    IntrospectedComponent(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted
void Cpu_data::handleEvent(Event *ev) {
  //printf("recv\n");
  simpleEvent *event = dynamic_cast<simpleEvent*>(ev);
  if (event) {
    // scan through each element in the payload and do something to it
    volatile int sum = 0;
    for (simpleEvent::dataVec::iterator i = event->payload.begin();
	 i != event->payload.end(); ++i) {
      sum += *i;
    }
    delete event;
  } else {
    printf("Error! Bad Event Type!\n");
  }
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool Cpu_data::clockTic( Cycle_t ) {
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
    // yes, communicate
    // create event
    simpleEvent *e = new simpleEvent();
    // fill payload with commSize bytes
    for (int i = 0; i < (commSize); ++i) {
      e->payload.push_back(1);
    }
    // find target
    neighbor = (++neighbor) % 4;
    // send
    switch (neighbor) {
    case 0:
      N->send(e);  
      break;
    case 1:
      S->send(e);
      break;
    case 2:
      E->send(e);
      break;
    case 3:
      W->send(e);
      break;
    default:
      printf("bad neighbor\n");
    }
    //printf("sent\n");
    //for introspection
    printf("Communicating with neighbors and updating usage counts\n");
    if (getId() == 2){
	       counts++;
	       num_il1_read++;
	       num_branch_read = num_branch_read + 2;
	       num_RAS_read = num_RAS_read + 2;
    }
    else{
		counts = counts+2;
		num_il1_read = num_il1_read +2;
    }
  }

  // return false so we keep going
  return false;
}

uint64_t Cpu_data::someCalculation()
{
	return (num_il1_read + num_branch_read + num_RAS_read);
}

double Cpu_data::updateTemperature()
{
	if (getId() == 0)
            mycore_temperature += 1.5;
        else
	    mycore_temperature += 1.0;
	return (mycore_temperature);
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleEvent)
BOOST_CLASS_EXPORT(Cpu_data)

static Component*
create_cpu_data(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new Cpu_data( id, params );
}

static const ElementInfoComponent components[] = {
    { "cpu_data",
      "Simple Demo Component",
      NULL,
      create_cpu_data
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo cpu_data_eli = {
        "Cpu_data",
        "Demo Component",
        components,
    };
}
