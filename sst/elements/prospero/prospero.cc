
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

  if( params.find("cache_line") == params.end() ) {
	cache_line_size = 64;
  } else {
	cache_line_size = atol( params[ "cache_line" ].c_str() );

	if(output_level > 0) {
		std::cout << "TRACE:  Cache line size configured for " << cache_line_size << std::endl;
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

  // configure link to cache components
  cache_link = configureLink( "cache_link", new Event::Handler<prospero>(this,
                                &prospero::handleEvent) );
  assert(cache_link);

  // Create a big buffer from which to take write request data
  zero_buffer = (uint8_t*) malloc(sizeof(uint8_t) * 2048);
  memset(zero_buffer, 0, sizeof(char) * 2048);

  // tell the simulator not to end without us
  registerExit();

  next_request.memory_address = 0;
  tick_count = 0;
  total_bytes_read = 0;
  total_bytes_written = 0;
  keep_generating = true;
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
		std::cerr << "TRACE:  Unknown memory operation type: " << ((char) op_type) << ", component will exit." << std::endl;
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

void prospero::handleEvent(Event *ev)
{
        MemEvent *event = dynamic_cast<MemEvent*>(ev);
        if (event) {
                // Branden says to ignore invalidates
                if ( event->getCmd() == Invalidate ) {
			delete event;
			return;
		}

		std::map<MemEvent::id_type, memory_request*>::iterator req_itr = pending_requests.find(event->getResponseToID());

		if(req_itr == pending_requests.end()) {
			// Something bad happen?
			std::cerr << "TRACE: Recv an ID which was not in the pending requests queue." << std::endl;
			exit(-1);
		} else {
			// Free the event copy we made and then clear out the map, we're done with this
			// event.
			free(pending_requests[event->getResponseToID()]);
			pending_requests.erase(req_itr);
		}

                delete event;
        } else {
                printf("Error! Bad Event Type!\n");
        }
}

void prospero::createPendingRequest(memory_request mem_req) {
	bool is_read = true;

	if(mem_req.memory_op_type == READ) {
		is_read = true;
	} else if (mem_req.memory_op_type == WRITE) {
		is_read = false;
	}

	if((mem_req.memory_address % cache_line_size) + mem_req.size > cache_line_size) {
		// have to perform a split load, this needs TWO events
		int e_lower_size = (cache_line_size) - (mem_req.memory_address % cache_line_size);
		int e_upper_size = mem_req.size - e_lower_size;

		if(output_level > 0) {
			std::cout << "TRACE: Generating a split cache request: " <<
				mem_req.memory_address << " is not aligned on cache line of: " <<
				cache_line_size << ", offset is: " <<
				(mem_req.memory_address % cache_line_size) << 
				" request size is: " << mem_req.size << std::endl;
			std::cout << "TRACE: Split into lower size of: " << e_lower_size <<
				" from @ " << mem_req.memory_address << 
				" and upper size of: " << e_upper_size << 
				" from @ " << (mem_req.memory_address + e_lower_size) << std::endl;
		}

		split_request_count++;

		MemEvent *e_lower = new MemEvent(this, mem_req.memory_address, is_read ? ReadReq : WriteReq);
		e_lower->setSize(e_lower_size);

		MemEvent *e_upper = new MemEvent(this, mem_req.memory_address + e_lower_size, is_read ? ReadReq : WriteReq);
		e_upper->setSize(e_upper_size);

		if(is_read) {
			total_bytes_read += mem_req.size;
			read_req_generated += 2;
		} else {
			e_lower->setPayload(e_lower_size, zero_buffer);
			e_upper->setPayload(e_upper_size, zero_buffer);
			total_bytes_written += mem_req.size;
			write_req_generated += 2;
		}

		cache_link->Send(e_lower);
		cache_link->Send(e_upper);

		memory_request* e_lower_req = (memory_request*) malloc(sizeof(memory_request));
		memory_request* e_upper_req = (memory_request*) malloc(sizeof(memory_request));

		e_lower_req->memory_op_type = mem_req.memory_op_type;
		e_upper_req->memory_op_type = mem_req.memory_op_type;

		e_lower_req->memory_address = mem_req.memory_address;
		e_upper_req->memory_address = mem_req.memory_address + e_lower_size;

		e_lower_req->size = e_lower_size;
		e_upper_req->size = e_upper_size;

		e_lower_req->instruction_count = mem_req.instruction_count;
		e_upper_req->instruction_count = mem_req.instruction_count;

		pending_requests[e_lower->getID()] = e_lower_req;
		pending_requests[e_upper->getID()] = e_upper_req;
	} else {
		MemEvent *e = new MemEvent(this, mem_req.memory_address, is_read ? ReadReq : WriteReq);
		e->setSize(mem_req.size);

		memory_request* e_req = (memory_request*) malloc(sizeof(memory_request));
		e_req->memory_op_type = mem_req.memory_op_type;
		e_req->memory_address = mem_req.memory_address;
		e_req->size = mem_req.size;
		e_req->instruction_count = mem_req.instruction_count;

		if(is_read) {
			total_bytes_read += mem_req.size;
			read_req_generated++;
		} else {
			e->setPayload(mem_req.size, zero_buffer);
			total_bytes_written += mem_req.size;
			write_req_generated++;
		}

		cache_link->Send(e);
		pending_requests[e->getID()] = e_req;
	}
}

bool prospero::tick( Cycle_t ) {
	if(keep_generating) {
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

	if(next_request.instruction_count <= tick_count) {
		if(output_level > 0) {
			std::cout << "TRACE:  Process request on this iteration. Pending queue count is: " <<
				pending_requests.size() << std::endl;
		}

		if(pending_requests.size() < pending_request_limit) {
			createPendingRequest(next_request);

			int continue_processing = 1;
			while(continue_processing) {
				// process the next_request object

				result = readNextRequest(&next_request);

				if(result == READ_FAILED_EOF) {
					// we're done reading trace information, let the simulation end gracefully
					// other components may need to drain
					keep_generating = false;
				} else if(result == READ_SUCCESS) {
					if(next_request.instruction_count < tick_count) {
					// Process the next request, if we have room put it in the queue
						if(pending_requests.size() < pending_request_limit) {
							createPendingRequest(next_request);
						} else {
							break;
						}
					} else {
						break;
					}
				} else {
					std::cerr << "Unknown response from a request read." << std::endl;
					exit(-1);
				}
			}
		} // else wait until next cycle
	} // else we just let the component tick forth as needed

		tick_count++;

		if(tick_count == max_tick_count) {
			std::cout << "TRACE: Stopping due to max ticks reached." << std::endl;
			std::cout << "TRACE: Will allow pending requests to drain." << std::endl;
			keep_generating = false;
		}


		if(pending_requests.size() < pending_request_limit) {
			cycles_pending_not_full++;
		}

		return false;
	} else {
		if(pending_requests.size() == 0) {
			return true;
		}
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
