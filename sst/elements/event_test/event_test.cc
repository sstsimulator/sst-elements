// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "sst/core/element.h"
#include "sst/core/simulation.h"
#include "sst/core/interfaces/TestEvent.h"

#include "event_test.h"

using namespace SST;

event_test::event_test(ComponentId_t id, Params_t& params) :
    Component(id),
    done(false)
{
    if ( params.find("id") == params.end() ) {
        _abort(event_test,"couldn't find node id\n");
    }
    my_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("count_to") == params.end() ) {
        _abort(event_test,"couldn't find count_to\n");
    }
    count_to = strtol( params[ "count_to" ].c_str(), NULL, 0 );

    if ( params.find("latency") == params.end() ) {
        _abort(event_test,"couldn't find latency\n");
    }
    latency = strtol( params[ "latency" ].c_str(), NULL, 0 );
    
    registerExit();

    Simulation::getSimulation()->requireEvent("interfaces.TestEvent");

//     EventHandler_t* linkHandler = new EventHandler<event_test,bool,Event*>
// 	(this,&event_test::handleEvent);

//     link = LinkAdd( "link", linkHandler );
    link = configureLink( "link", new Event::Handler<event_test>(this,&event_test::handleEvent) );
    if (NULL == link) abort();

    registerTimeBase("1ns");

    // Test the InitData capabilities
    link->sendInitData("test");
}


event_test::event_test() :
    Component(-1)
{
    // for serialization only
}


int
event_test::Setup()
{

    if ( link->recvInitDataString().compare("test") != 0 ) printf("InitData not working\n");
    
	if ( my_id == 0 ) {
		Interfaces::TestEvent* event = new Interfaces::TestEvent();
		event->count = 0;
		link->Send(latency,event);
		printf("Sending initial event\n");
	}
	else if ( my_id != 1) {
		_abort(event_test,"event_test class only works with two instances\n");	
	}
	return 0;
}


int
event_test::Finish()
{
    return 0;
}


void
event_test::handleEvent(Event* ev)
{
	Interfaces::TestEvent* event = static_cast<Interfaces::TestEvent*>(ev);
    // See if we have counted far enough, if not, increment and send
    // back
    if (event->count > count_to) {
	event->count++;
	link->Send(latency,event);
	if (!done) {
	    unregisterExit();
	    done = true;
	}
    }
    else {
	printf("%d: %d\n",my_id,event->count);
	event->count++;
	link->Send(latency,event);
    }    
}


// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(event_test)

static Component*
create_event_test(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new event_test( id, params );
}

static const ElementInfoComponent components[] = {
    { "event_test",
      "Event test driver",
      NULL,
      create_event_test
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo event_test_eli = {
        "event_test",
        "Event serialization / passing test driver",
        components,
    };
}
