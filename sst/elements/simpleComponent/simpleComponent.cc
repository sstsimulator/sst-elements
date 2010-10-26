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

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "simpleComponent.h"
#include "simpleEvent.h"

using namespace SST;

simpleComponent::simpleComponent(ComponentId_t id, Params_t& params) :
  Component(id) {

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
  
  // init randomness
  srandomdev();
  neighbor = random() % 4;

  // tell the simulator not to end without us
  registerExit();

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
  volatile int v = 0;
  for (int i = 0; i < workPerCycle; ++i) {
    v++;
  }

  // communicate?
  if ((random() % commFreq) == 0) {
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
      N->Send(e);
      break;
    case 1:
      S->Send(e);
      break;
    case 2:
      E->Send(e);
      break;
    case 3:
      W->Send(e);
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
                  SST::Component::Params_t& params)
{
    return new simpleComponent( id, params );
}

static const ElementInfoComponent components[] = {
    { "simpleComponent",
      "Simple Demo Component",
      NULL,
      create_simpleComponent
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo simpleComponent_eli = {
        "simpleComponent",
        "Demo Component",
        components,
    };
}
