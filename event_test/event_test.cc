// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "event_test.h"

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/simulation.h"
#include "sst/core/interfaces/TestEvent.h"


using namespace SST;
using namespace SST::EventTest;

event_test::event_test(ComponentId_t id, Params& params) :
    Component(id),
    done(false)
{
    bool found;

    Output &out = Simulation::getSimulation()->getSimulationOutput();

    my_id = params.find_integer("id", -1, found);
    if (!found) {
        out.fatal(CALL_INFO, -1,"couldn't find node id\n");
    }

    count_to = params.find_integer("count_to", 0, found);
    if (!found) {
        out.fatal(CALL_INFO, -1,"couldn't find count_to\n");
    }
        
    latency = params.find_integer("latency", 0, found);
    if (!found) {
        out.fatal(CALL_INFO, -1,"couldn't find latency\n");
    }
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

//     EventHandler_t* linkHandler = new EventHandler<event_test,bool,Event*>
// 	(this,&event_test::handleEvent);

//     link = LinkAdd( "link", linkHandler );
    link = configureLink( "link", new Event::Handler<event_test>(this,&event_test::handleEvent) );
    if (NULL == link) abort();

    registerTimeBase("1ns");

    // Test the InitData capabilities
    //link->sendInitData("test");
}


event_test::event_test() :
    Component(-1)
{
    // for serialization only
}

void event_test::setup()
{

    //if ( link->recvInitDataString().compare("test") != 0 ) printf("InitData not working\n");
    
	if ( my_id == 0 ) {
		Interfaces::TestEvent* event = new Interfaces::TestEvent();
		event->count = 0;
		link->send(latency,event);
		printf("Sending initial event\n");
	}
	else if ( my_id != 1) {
        Output &out = Simulation::getSimulation()->getSimulationOutput();
		out.fatal(CALL_INFO, -1,"event_test class only works with two instances\n");	
	}
//	return 0;
}


void event_test::finish() 
{
}


void
event_test::init(unsigned int phase)
{
    Interfaces::TestEvent* event;
    if ( phase == 0 || phase == 1 ) {
	event = new Interfaces::TestEvent();
	event->print_on_delete = true;
	printf("%d: sending event during phase %d\n",my_id,phase);
	link->sendInitData(event);
    }

    if ( phase == 0 || phase == 1 ) {
	event = static_cast<Interfaces::TestEvent*>(link->recvInitData());
	if ( event != NULL ) {
	    printf("%d: received init event during phase %d\n",my_id,phase);
	    delete event;
	}
    }
}

void
event_test::handleEvent(Event* ev)
{
	Interfaces::TestEvent* event = static_cast<Interfaces::TestEvent*>(ev);
    // See if we have counted far enough, if not, increment and send
    // back
    if (event->count > count_to) {
	event->count++;
	link->send(latency,event); 
	if (!done) {
      primaryComponentOKToEndSim();
	    done = true;
	}
    }
    else {
	printf("%d: %d\n",my_id,event->count);
	event->count++;
	link->send(latency,event); 
    }    
}


// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(SST::EventTest::event_test)

static Component*
create_event_test(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new event_test( id, params );
}

static const ElementInfoParam component_params[] = {
    { "id", "Node id" },
    { "count_to", "Number of iterations of the test" },
    { "latency", "Send latency for event" },
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "event_test",
      "Event test driver",
      NULL,
      create_event_test,
      component_params
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
