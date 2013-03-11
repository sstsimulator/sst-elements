
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "prospero.h"

using namespace SST;

prospero::prospero(ComponentId_t id, Params_t& params) :
  Component(id) {

  // Work out how much we're supposed to be reporting.
  if( params.find("outputlevel") != params.end() ) {
	output_level = params.find_integer("outputlevel", 0);
  }

  if( params.find("maximum_items") == params.end() ) {
  	max_trace_count = 4611686018427390000;

	if(output_level > 0) {
		std::cout << "TRACE:  Did not find trace limit, setting to: " << max_trace_count << std::endl;
	}
  } else {
  	max_trace_count = atol( params[ "maximum_items" ].c_str() );

	if(output_level > 0) {
		std::cout << "TRACE:  Found maximum trace limit: " << max_trace_count << std::endl;
	}
  }

  if( params.find("trace") == params.end() ) {
	std::cerr << "Could not find \'trace\' parameter in simulation SDL file." << std::endl;
	exit(-1);
  } else {
	if(output_level > 0) {
		std::cout << "TRACE:  Load trace information from: " << params[ "trace" ] << std::endl;
	}

	trace_input = fopen(params["trace"].c_str(), "rt");
  }

  // tell the simulator not to end without us
  registerExit();

  //set our clock
  //registerClock( clock_frequency_str, 
  //		 new Clock::Handler<prospero>(this, 
  //			&prospero::tick ) );
}

prospero::prospero() :
    Component(-1)
{
    // for serialization only
}

bool prospero::tick( Cycle_t ) {
	return true;
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(prospero)

static Component*
create_prospero(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new prospero( id, params );
}

static const ElementInfoComponent components[] = {
    { "Prospero",
      "Memory tracing component",
      NULL,
      create_prospero
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo prospero_eli = {
        "prospero",
        "Memory tracing component",
        components,
    };
}
