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
#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/miranda/generators/randomgen.h>

using namespace SST::Miranda;

RandomGenerator::RandomGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("RandomGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	issueCount = (uint64_t) params.find_integer("count", 1000);
	reqLength  = (uint64_t) params.find_integer("length", 8);
	maxAddr    = (uint64_t) params.find_integer("max_address", 524288);

	rng = new MarsagliaRNG(11, 31);

	out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " operations\n", issueCount);
	out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
	out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIu64 "\n", maxAddr);

	issueOpFences = params.find_string("issue_op_fences", "yes") == "yes";
}

RandomGenerator::~RandomGenerator() {
	delete out;
	delete rng;
}

void RandomGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request number: %" PRIu64 "\n", issueCount);

	const uint64_t rand_addr = rng->generateNextUInt64();
	// Ensure we have a reqLength aligned request
	const uint64_t addr_under_limit = (rand_addr % maxAddr);
	const uint64_t addr = (addr_under_limit < reqLength) ? addr_under_limit :
		(rand_addr % maxAddr) - (rand_addr % reqLength);

	const double op_decide = rng->nextUniform();

	// Populate request
	q->push_back(new MemoryOpRequest(addr, reqLength, (op_decide < 0.5) ? READ : WRITE));

	if(issueOpFences) {
		q->push_back(new FenceOpRequest());
	}

	issueCount--;
}

bool RandomGenerator::isFinished() {
	return (issueCount == 0);
}

void RandomGenerator::completed() {

}
