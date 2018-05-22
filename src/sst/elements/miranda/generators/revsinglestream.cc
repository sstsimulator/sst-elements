// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/revsinglestream.h>

using namespace SST::Miranda;

ReverseSingleStreamGenerator::ReverseSingleStreamGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("ReverseSingleStreamGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	stopIndex   = params.find<uint64_t>("stop_at", 0);
	startIndex  = params.find<uint64_t>("start_at", 1024);
	datawidth   = params.find<uint64_t>("datawidth", 8);
	stride      = params.find<uint64_t>("stride", 1);

	if(startIndex < stopIndex) {
		out->fatal(CALL_INFO, -1, "Start address (%" PRIu64 ") must be greater than stop address (%" PRIu64 ") in reverse stream generator",
			startIndex, stopIndex);
	}

	out->verbose(CALL_INFO, 1, 0, "Start Address:         %" PRIu64 "\n", startIndex);
	out->verbose(CALL_INFO, 1, 0, "Stop Address:          %" PRIu64 "\n", stopIndex);
	out->verbose(CALL_INFO, 1, 0, "Data width:            %" PRIu64 "\n", datawidth);
	out->verbose(CALL_INFO, 1, 0, "Stride:                %" PRIu64 "\n", stride);

	nextIndex = startIndex;
}

ReverseSingleStreamGenerator::~ReverseSingleStreamGenerator() {
	delete out;
}

void ReverseSingleStreamGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request at address: %" PRIu64 "\n", nextIndex);

	q->push_back(new MemoryOpRequest(nextIndex * datawidth, datawidth, READ));

	// What is the next address?
	nextIndex = nextIndex - stride;
}

bool ReverseSingleStreamGenerator::isFinished() {
	return (nextIndex == stopIndex);
}

void ReverseSingleStreamGenerator::completed() {

}
