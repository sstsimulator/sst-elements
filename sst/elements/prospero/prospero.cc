
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

	trace_input = fopen(params["trace"].c_str(), "rb");

	if(trace_input == NULL) {
		std::cerr << "TRACE:  Unable to open trace file: " <<
			params["trace"] << ", component will halt." << std::endl;
		exit(-1);
	}
	//trace_input.open(params["trace"].c_str(), fstream::in);
  }

  if( params.find("request_limit") == params.end() ) {
	pending_request_limit = 16;
  } else {
	pending_request_limit = atoi( params[ "request_limit" ].c_str() );

	if(output_level > 0) {
		std::cout << "TRACE:  Configuring traces for a pending request limit of " << pending_request_limit << std::endl;
	}
  }

  if( params.find("page_size") == params.end() ) {
	page_size = 4096;
  } else {
	page_size = atoi( params[ "page_size" ].c_str() );

	if(output_level > 0) {
		std::cout << "TRACE:  Configuring traces for a page size of " << page_size << " bytes" << std::endl;
	}
  }
  next_page_start = 0;

  if( params.find("max_ticks") == params.end() ) {
	max_tick_count = 4611686018427390000;
  } else {
	max_tick_count = atol( params[ "max_ticks" ].c_str() );

	if(output_level > 0) {
		std::cout << "TRACE:  Max ticks configured for " << max_tick_count << std::endl;
	}
  }

  if( params.find("clock") != params.end() ) {
	string clock_rate = params[ "clock" ];

  	// register the prospero clock for generating addresses
  	registerClock( clock_rate, new Clock::Handler<prospero>(this, &prospero::tick ) );

	if(output_level > 0) {
		std::cout << "TRACE:  Component will tick at " << clock_rate << std::endl;
        }
  } else {
	if(output_level > 0) {
		std::cout << "TRACE:  Component will tick at 2GHz (not specified in input)." << std::endl;
        }

  	// register the prospero clock for generating addresses
  	registerClock( "2GHz", new Clock::Handler<prospero>(this, &prospero::tick ) );

  }

  // tell the simulator not to end without us
  registerExit();

  next_request.memory_address = 0;
  tick_count = 0;
}

void copy(void* dest, void* source, int source_offset, int length) {
	char* dest_c = (char*) dest;
	char* source_c = (char*) source;
	int counter;

	for(counter = 0; counter < length; counter++) {
		dest_c[counter] = source_c[source_offset + counter];
	}
}

read_trace_return prospero::readNextRequest(memory_request* req) {
	if(feof(trace_input)) {
		return READ_FAILED_EOF;
	}

	const int record_length = sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t);
	char record_buffer[ record_length ];
	fread(record_buffer, record_length, 1, trace_input);

	char op_type;

	copy(&(req->instruction_count), record_buffer, 0, sizeof(uint64_t));
	copy(&op_type, record_buffer, sizeof(uint64_t), sizeof(char));
	copy(&(req->memory_address), record_buffer, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t));
	copy(&(req->size), record_buffer, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t));

	// decode memory operation to category the upstream components can use
	if(op_type == 0) 
		req->memory_op_type = READ;
	else if (op_type == 1)
		req->memory_op_type = WRITE;
	else {
		std::cerr << "TRACE:  Unknown memory operation type: " << op_type << ", component will exit." << std::endl;
		exit(-1);
	}

        if(output_level > 0) {
		std::cout << "TRACE:  Performing page table lookup..." << std::endl;
	}

	uint64_t remainder = (req->memory_address % page_size);
	uint64_t page_lookup = req->memory_address - remainder;
        uint64_t page_start;
        if(page_table.find(page_lookup) == page_table.end()) {
		// page fault
		page_table[page_lookup] = next_page_start;
		page_start = next_page_start;
		next_page_start = next_page_start + page_size;
		new_page_creates++;

		if(output_level > 0) {
			std::cout << "TRACE:  Page fault, " << page_lookup << " not found, created new page at: " <<
				page_table[page_lookup] << " request offset: " << remainder << std::endl;
		}
        } else {
		page_start = page_table[page_lookup];
        }

	req->memory_address = page_start + remainder;

	if(output_level > 0) {
		std::cout << "TRACE: Next record: Ins=" << req->instruction_count <<
			"/ MA=" << req->memory_address << "/ Size=" << req->size << std::endl;
	}

	requests_generated++;
	return READ_SUCCESS;
}

prospero::prospero() :
    Component(-1)
{
    // for serialization only
}

bool prospero::tick( Cycle_t ) {
	if(output_level > 0) {
		std::cout << "TRACE: Tick count " << tick_count << std::endl;
	}

	// hold result for any trace reads we need to do
	read_trace_return result;

	if(tick_count == 0) {
		if(output_level > 0) {
			std::cout << "TRACE:  Reading initial memory request record..." << std::endl;
		}

		result = readNextRequest( &next_request );

		if(result == READ_FAILED_EOF) {
			std::cout << "TRACE: Read failed on first record, are you sure file was created successfully?" << std::endl;
			exit(-1);
		}
	}

	std::cout << "Request instruction: " << (int) next_request.instruction_count << ", current tick=" << 
		(int) tick_count << std::endl;

	if(next_request.instruction_count <= tick_count) {
		std::cout << "TRACE:  Process request on this iteration." << std::endl;

		int continue_processing = 1;
		while(continue_processing) {
			// process the next_request object

			result = readNextRequest(&next_request);
			if(result == READ_FAILED_EOF) {
				// we're done reading trace information, let the simulation end gracefully
				// other components may need to drain
				return true;
			} else if(result == READ_SUCCESS) {
				// Process the next request
				if(next_request.instruction_count > tick_count) {
					break;
				}
			}
		}
	} // else we just let the component tick forth as needed

	tick_count++;

	if(tick_count == max_tick_count) {
		std::cout << "TRACE: Stopping due to max ticks reached." << std::endl;
		return true;
	} else {
		return false;
	}
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
    { "prospero",
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
