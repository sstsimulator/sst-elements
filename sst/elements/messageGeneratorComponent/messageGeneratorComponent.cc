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
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "messageGeneratorComponent.h"
#include "simpleMessage.h"

using namespace SST;
using namespace SST::MessageGenerator;

messageGeneratorComponent::messageGeneratorComponent(ComponentId_t id, Params_t& params) :
  Component(id) {

  if( params.find("clock") == params.end() ) {
  	clock_frequency_str = "1GHz";
  } else {
  	clock_frequency_str = params[ "clock" ];
  }

  std::cout << "Clock is configured for: " << clock_frequency_str << std::endl;

  if( params.find("sendcount") == params.end() ) {
	total_message_send_count = 1000;
  } else {
	total_message_send_count = strtol( params[ "sendcount" ].c_str(), NULL, 0);
  }

  if( params.find("outputinfo") == params.end() ) {
	output_message_info = 1;
  } else {
	output_message_info = strtol( params["outputinfo"].c_str(), NULL, 0);
  }

  message_counter_recv = 0;
  message_counter_sent = 0;

  // tell the simulator not to end without us
//  registerExit();  // Renamed Per Issue 70 - ALevine
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  remote_component = configureLink( "remoteComponent",
  						new Event::Handler<messageGeneratorComponent>(this,
  									&messageGeneratorComponent::handleEvent) );

  assert(remote_component);

  //set our clock
  registerClock( clock_frequency_str, 
		 new Clock::Handler<messageGeneratorComponent>(this, 
						     &messageGeneratorComponent::tick ) );
}

messageGeneratorComponent::messageGeneratorComponent() :
    Component(-1)
{
    // for serialization only
}

void messageGeneratorComponent::handleEvent(Event *event) {
	message_counter_recv++;

	if(output_message_info)
		std::cout << "Received message: " << message_counter_recv << " (time=" << getCurrentSimTimeMicro() << "us)" << std::endl;
	
	delete event;

	if(message_counter_recv == total_message_send_count) {
//		unregisterExit();  // Renamed Per Issue 70 - ALevine
    primaryComponentOKToEndSim();
	}
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool messageGeneratorComponent::tick( Cycle_t ) {

	simpleMessage* msg = new simpleMessage();
	remote_component->send(msg);   // Renamed per Issue 70 - ALevine

	if(output_message_info)
		std::cout << "Sent message: " << message_counter_sent << " (time=" << getCurrentSimTimeMicro() << "us)" << std::endl;

	message_counter_sent++;

  	// return false so we keep going
  	if(message_counter_sent == total_message_send_count) {
  		return true;
  	} else {
  		return false;
  	}
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleMessage)
BOOST_CLASS_EXPORT(messageGeneratorComponent)

static Component*
create_messageGeneratorComponent(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new messageGeneratorComponent( id, params );
}

static const ElementInfoComponent components[] = {
    { "messageGeneratorComponent",
      "Messaging rate benchmark component",
      NULL,
      create_messageGeneratorComponent
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo messageGeneratorComponent_eli = {
        "messageGeneratorComponent",
        "Messaging rate benchmark component",
        components,
    };
}
