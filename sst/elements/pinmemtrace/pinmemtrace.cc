
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "pinmemtrace.h"

using namespace SST;

PinMemTrace::PinMemTrace(ComponentId_t id, Params_t& params) :
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

  PIN_MEM_TRACE_TYPE trace_type = TRACE_FILE;
  if(params.find("tracetype") != params.end() ) {
	if( params[ "tracetype" ] == "pin" ) {
		trace_type = EXECUTE_PIN;

		if(output_level > 0) std::cout << "TRACE:  Trace input type is execute on PIN" << std::endl;
	} else if ( params[ "tracetype" ] == "file" ) {
		trace_type = TRACE_FILE;
		if(output_level > 0) std::cout << "TRACE:  Trace input type is load from file" << std::endl;
	} else {
		std::cerr << "Unknown trace type: " << params[ "tracetype" ] << std::endl;
		exit(-1);
	}
  }

  if(params.find("pinpath") == params.end() ) {
	pin_path = "pin";
  } else {
	pin_path = params[ "pinpath" ];
  }

  if(output_level > 0 && trace_type == EXECUTE_PIN) {
	std::cout << "TRACE:  Using PIN-tool located at: " << pin_path << std::endl;
  }

  if( params.find("trace") == params.end() ) {
	std::cerr << "Could not find \'trace\' parameter in simulation SDL file." << std::endl;
	exit(-1);
  } else {
	if(output_level > 0) {
		std::cout << "TRACE:  Load trace information from: " << params[ "trace" ] << std::endl;
	}
	// Load the trace based on the type interpretation from above
	//trace_input = params[ "trace" ].c_str();
  }

  // tell the simulator not to end without us
  registerExit();

  if(output_level > 0) {
	std::cout << "Trace Input: " << trace_input << std::endl;

	if( trace_type == TRACE_FILE ) {
		std::cout << "Trace Type:  Trace File" << std::endl;
	} else if (trace_type == EXECUTE_PIN ) {
		std::cout << "Trace Type:  Execute via PIN" << std::endl;
	}
  }

  //set our clock
  //registerClock( clock_frequency_str, 
  //		 new Clock::Handler<PinMemTrace>(this, 
  //			&PinMemTrace::tick ) );
}

PinMemTrace::PinMemTrace() :
    Component(-1)
{
    // for serialization only
}

bool PinMemTrace::tick( Cycle_t ) {
	return true;
}

// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(PinMemTrace)

static Component*
create_PinMemTrace(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new PinMemTrace( id, params );
}

static const ElementInfoComponent components[] = {
    { "PinMemTraceComponent",
      "Clock benchmark component",
      NULL,
      create_PinMemTrace
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo PinMemTraceComponent_eli = {
        "PinMemTraceComponent",
        "Clock benchmark component",
        components,
    };
}
