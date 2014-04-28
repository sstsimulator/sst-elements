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

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "simpleComponent.h"

#include <assert.h>

#include "sst/core/debug.h"
#include "sst/core/element.h"
#include "sst/core/params.h"

#include "simpleEvent.h"

using namespace SST;
using namespace SST::SimpleComponent;

simpleComponent::simpleComponent(ComponentId_t id, Params& params) :
  Component(id) {
  bool found;

  // get parameters
  workPerCycle = params.find_integer("workPerCycle", 0, found);
  if (!found) {
    _abort(event_test,"couldn't find work per cycle\n");
  }

  commFreq = params.find_integer("commFreq", 0, found);
  if (!found) {
    _abort(event_test,"couldn't find communication frequency\n");
  }

  commSize = params.find_integer("commSize", 0, found);
  if (!found) {
    _abort(event_test,"couldn't find communication size\n");
  }
  
  // init randomness
  srand(1);
  neighbor = rand() % 4;

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  // configure out links
  N = configureLink( "Nlink", 
		     new Event::Handler<simpleComponent>(this,
							 &simpleComponent::
							 handleEvent) );
  S = configureLink( "Slink", 
		     new Event::Handler<simpleComponent>(this,
							 &simpleComponent::
							 handleEvent) );
  E = configureLink( "Elink", 
		     new Event::Handler<simpleComponent>(this,
							 &simpleComponent::
							 handleEvent) );
  W = configureLink( "Wlink", 
		     new Event::Handler<simpleComponent>(this,
							 &simpleComponent::
							 handleEvent) );
  assert(N);
  assert(S);
  assert(E);
  assert(W);

  //set our clock
  registerClock( "1GHz", 
		 new Clock::Handler<simpleComponent>(this, 
						     &simpleComponent::clockTic ) );
}

simpleComponent::simpleComponent() :
    Component(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted
void simpleComponent::handleEvent(Event *ev) {
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
bool simpleComponent::clockTic( Cycle_t ) {
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
    neighbor = neighbor+1;
    neighbor = neighbor % 4;
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
  }

  // return false so we keep going
  return false;
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleEvent)
BOOST_CLASS_EXPORT(simpleComponent)

static Component*
create_simpleComponent(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new simpleComponent( id, params );
}

static const ElementInfoParam component_params[] = {
    { "workPerCycle", "Count of busy work to do during a clock tick.", NULL},
    { "commFreq", "Approximate frequency of sending an event during a clock tick.", NULL},
    { "commSize", "Size of communication to send.", NULL},
    { NULL, NULL, NULL}
};

static const char *port_events[] = { "simpleComponent.simpleEvent", NULL };

static const ElementInfoPort component_ports[] = {
    {"Nlink", "Link to the SimpleComponent to the North", port_events},
    {"Slink", "Link to the SimpleComponent to the South", port_events},
    {"Elink", "Link to the SimpleComponent to the East", port_events},
    {"Wlink", "Link to the SimpleComponent to the West", port_events},
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "simpleComponent",
      "Simple Demo Component",
      NULL,
      create_simpleComponent,
      component_params,
      component_ports,
      COMPONENT_CATEGORY_PROCESSOR
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

extern "C" {
    ElementLibraryInfo simpleComponent_eli = {
        "simpleComponent",
        "Demo Component",
        components,
    };
}
