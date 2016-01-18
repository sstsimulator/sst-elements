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


#include <sst_config.h>
#include "emberdumpitrace.h"

#include <dumpi/libundumpi/callbacks.h>
#include <dumpi/libundumpi/libundumpi.h>

using namespace SST::Ember;

extern "C" {
static int ember_dumpi_callback(const void *parsearg,
        uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall,
        const dumpi_perfinfo *perf, void *userarg) {

	printf("Ember - DUMPI Callback CALLED!");

	return 0;
}
}

EmberDUMPITraceGenerator::EmberDUMPITraceGenerator(SST::Component* owner,
                                            Params& params) :
	EmberMessagePassingGenerator(owner, params, "DUMPITrace")
{
	std::string trace_file_name = params.find_string("arg.tracefile", "");

	if( "" == trace_file_name ) {
		// Error goes her
	} else {
		trace_file = undumpi_open(trace_file_name.c_str());

		if( NULL == trace_file ) {
			// Load Error Here
		} else {
			dumpi_header* trace_header = undumpi_read_header(trace_file);
		}
	}
}

bool EmberDUMPITraceGenerator::generate( std::queue<EmberEvent*>& evQ )
{
	int mpi_finalized = 0;

	int send_success = libundumpi_grab_send(trace_file,
		ember_dumpi_callback, (void*) this);

	if( send_success > 0 ) {
		printf("Call to send successful.\n");
	} else {
		printf("Call to send failed.\n");
	}

    	return false;
}
