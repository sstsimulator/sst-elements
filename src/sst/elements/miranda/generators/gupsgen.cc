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
#include <sst/elements/miranda/generators/gupsgen.h>

using namespace SST::Miranda;

GUPSGenerator::GUPSGenerator( ComponentId_t id, Params& params ) : RequestGenerator(id, params) {
    build(params);
}


void GUPSGenerator::build(Params &params) {
    const uint32_t verbose = params.find<uint32_t>("verbose", 0);

    out = new Output("GUPSGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

    iterations = params.find<uint64_t>("iterations", 1);
    issueCount = (params.find<uint64_t>("count", 1000)) * iterations;
    reqLength  = params.find<uint64_t>("length", 8);
    memStart    = params.find<uint64_t>("min_address", 0);
    memLength   = params.find<uint64_t>("max_address", 524288) - memStart;
    seed_a     = params.find<uint64_t>("seed_a", 11);
    seed_b     = params.find<uint64_t>("seed_b", 31);
    rng = new MarsagliaRNG(seed_a, seed_b);

    out->verbose(CALL_INFO, 1, 0, "Will issue %" PRIu64 " operations\n", issueCount);
    out->verbose(CALL_INFO, 1, 0, "Request lengths: %" PRIu64 " bytes\n", reqLength);
    out->verbose(CALL_INFO, 1, 0, "Minimum address: %" PRIu64 "\n", memStart);
    out->verbose(CALL_INFO, 1, 0, "Maximum address: %" PRIu64 "\n", memStart + memLength);

    issueOpFences = params.find<std::string>("issue_op_fences", "yes") == "yes";
}

GUPSGenerator::~GUPSGenerator() {
    delete out;
    delete rng;
}

void GUPSGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {

    const uint64_t rand_addr = rng->generateNextUInt64();
    // Ensure we have a reqLength aligned request

    uint64_t addr = (rand_addr % ( memLength / reqLength ) );
    addr *= reqLength;
    addr += memStart;

    out->verbose(CALL_INFO, 4, 0, "Generating next request number: %" PRIu64 " at address %" PRIu64 "\n", issueCount, addr);

    MemoryOpRequest* readAddr = new MemoryOpRequest(addr, reqLength, READ);
    MemoryOpRequest* writeAddr = new MemoryOpRequest(addr, reqLength, WRITE);

    writeAddr->addDependency(readAddr->getRequestID());

    q->push_back(readAddr);
    q->push_back(writeAddr);

    issueCount--;
}

bool GUPSGenerator::isFinished() {
    return (issueCount == 0);
}

void GUPSGenerator::completed() {

}
