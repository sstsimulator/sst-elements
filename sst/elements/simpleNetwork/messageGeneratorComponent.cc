
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "messageGeneratorComponent.h"
#include "simpleMessage.h"

using namespace SST;

messageGeneratorComponent::messageGeneratorComponent(ComponentId_t id, Params_t& params) :
  Component(id) {
  
  if( params.find("clock") == params.end() ) {
  	clock_frequency_str = "1GHz";
  } else {
  	clock_frequency_str = params[ "clock" ];
  }
  
  message_counter_recv = 0;
  message_counter_sent = 0;
  
  // tell the simulator not to end without us
  registerExit();

  remote_component = configureLink( "remoteComponent",
  						new Event::Handler<messageGeneratorComponent>(this,
  									&messageGeneratorComponent::handleEvent) );

  assert(remote_component);

  //set our clock
  registerClock( "1GHz", 
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
	
	std::cout << "Received message: " << message_counter_recv << std::endl;
	delete event;
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool messageGeneratorComponent::tick( Cycle_t ) {
	
	simpleMessage* msg = new simpleMessage();
	remote_component->Send(msg);
	
	std::cout << "Sent message: " << message_counter_sent << std::endl;

	message_counter_sent++;

  	// return false so we keep going
  	if(message_counter_sent == 1000) {
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
