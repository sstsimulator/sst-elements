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
#include <sst/elements/miranda/generators/singlestream.h>

using namespace SST::Miranda;


SingleStreamGenerator::SingleStreamGenerator( ComponentId_t id, Params& params ) :
	RequestGenerator(id, params) {
            build(params);
        }

void SingleStreamGenerator::build(Params& params) {

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("SingleStreamGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	issueCount = params.find<uint64_t>("count", 1000);
	reqLength  = params.find<uint64_t>("length", 8);
	startAddr  = params.find<uint64_t>("startat", 0);
	maxAddr    = params.find<uint64_t>("max_address", 524288);

	nextAddr   = startAddr;

	std::string op = params.find<std::string>( "memOp", "Read" );
	if ( ! op.compare( "Read" ) ) {
		memOp = READ;
	} else if ( ! op.compare( "Write" ) ) {
		memOp = WRITE;
	} else {
		assert( 0 );
	}

	out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " %s operations\n",
				issueCount, memOp == READ ? "Read": "Write");
	out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
	out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIx64 "\n", maxAddr);
	out->verbose(CALL_INFO, 1, 0, "First address: %" PRIx64 "\n", nextAddr);
}

SingleStreamGenerator::~SingleStreamGenerator() {
	delete out;
}

void SingleStreamGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 4, 0, "Generating next request number: %" PRIu64 "\n", issueCount);

	q->push_back(new MemoryOpRequest(nextAddr, reqLength, memOp));

	// What is the next address?
	nextAddr = (nextAddr + reqLength) % maxAddr;
	if( nextAddr == 0 )
		nextAddr = startAddr;

	issueCount--;
}

bool SingleStreamGenerator::isFinished() {
	return (issueCount == 0);
}

void SingleStreamGenerator::completed() {

}
