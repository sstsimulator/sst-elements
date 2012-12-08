
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "simpleClocker.h"

using namespace SST;

simpleClocker::simpleClocker(ComponentId_t id, Params_t& params) :
  Component(id) {

  if( params.find("clock") == params.end() ) {
  	clock_frequency_str = "1GHz";
  } else {
  	clock_frequency_str = params[ "clock" ];
  }

  std::cout << "Clock is configured for: " << clock_frequency_str << std::endl;

  if( params.find("clockcount") == params.end() ) {
	clock_count = 1000;
  } else {
	clock_count = strtol( params[ "clockcount" ].c_str(), NULL, 0);
  }

  // tell the simulator not to end without us
  registerExit();

  //set our clock
  registerClock( clock_frequency_str, 
		 new Clock::Handler<simpleClocker>(this, 
			&simpleClocker::tick ) );
}

simpleClocker::simpleClocker() :
    Component(-1)
{
    // for serialization only
}

bool simpleClocker::tick( Cycle_t ) {
	clock_count--;

  	// return false so we keep going
  	if(clock_count == 0) {
		unregisterExit();
  		return true;
  	} else {
  		return false;
  	}
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleClocker)

static Component*
create_simpleClocker(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new simpleClocker( id, params );
}

static const ElementInfoComponent components[] = {
    { "simpleClockerComponent",
      "Clock benchmark component",
      NULL,
      create_simpleClocker
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo simpleClockerComponent_eli = {
        "simpleClockerComponent",
        "Clock benchmark component",
        components,
    };
}
