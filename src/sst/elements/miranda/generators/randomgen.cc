// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/miranda/generators/randomgen.h>

using namespace SST::Miranda;


RandomGenerator::RandomGenerator( ComponentId_t id, Params& params ) :
	RequestGenerator(id, params) {
            build(params);
        }

void RandomGenerator::build(Params& params) {
	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("RandomGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	issueCount = params.find<uint64_t>("count", 1000);
	reqLength  = params.find<uint64_t>("length", 8);
	maxAddr    = params.find<uint64_t>("max_address", 524288);

	rng = new MarsagliaRNG(11, 31);

	out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " operations\n", issueCount);
	out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
	out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIu64 "\n", maxAddr);

	issueOpFences = params.find<std::string>("issue_op_fences", "yes") == "yes";

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

	if (issueOpFences) {
	    q->push_back(new FenceOpRequest());
	}

	issueCount--;
}

bool RandomGenerator::isFinished() {
	return (issueCount == 0);
}

void RandomGenerator::completed() {

}
