
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "simpleRNGComponent.h"

using namespace SST;
using namespace SST::SimpleRNGComponent;

simpleRNGComponent::simpleRNGComponent(ComponentId_t id, Params_t& params) :
  Component(id) {

  unsigned int m_z = 0;
  unsigned int  m_w = 0;

  if( params.find("seed_w") != params.end() ) {
	m_w = (unsigned int) strtoul( params["seed_w"].c_str(), NULL, 10);
  }

  if( params.find("seed_z") != params.end() ) {
	m_z = (unsigned int) strtoul( params["seed_z"].c_str(), NULL, 10);
  }

  rng_count = 1000;
  if( params.find("count") != params.end() ) {
	rng_count = (unsigned int) atoi( params["count"].c_str() );
  }

  if(m_w == 0 || m_z == 0) {
	rng = new SSTRandom();
  } else {
	rng = new SSTRandom(m_z, m_w);
  }

  rng_noseed = new SSTRandom();

  // tell the simulator not to end without us
  registerExit();

  //set our clock
  registerClock( "1GHz", 
		 new Clock::Handler<simpleRNGComponent>(this, 
			&simpleRNGComponent::tick ) );
}

simpleRNGComponent::simpleRNGComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleRNGComponent::tick( Cycle_t ) {
	rng_count--;

	std::cout << "Next uniform random: " <<
		rng->nextUniform() << " / No seed=" <<
		rng_noseed->nextUniform() << std::endl;

  	// return false so we keep going
  	if(rng_count == 0) {
		unregisterExit();
  		return true;
  	} else {
  		return false;
  	}
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleRNGComponent)

static Component*
create_simpleRNGComponent(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new simpleRNGComponent( id, params );
}

static const ElementInfoComponent components[] = {
    { "simpleRNGComponent",
      "Random number generation component",
      NULL,
      create_simpleRNGComponent
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo simpleRNGComponent_eli = {
        "simpleRNGComponent",
        "Random number generation component",
        components,
    };
}
