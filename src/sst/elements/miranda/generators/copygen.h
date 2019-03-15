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


#ifndef _H_SST_MIRANDA_COPY_BENCH_GEN
#define _H_SST_MIRANDA_COPY_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class CopyGenerator : public RequestGenerator {

public:
	CopyGenerator( Component* owner, Params& params ) : RequestGenerator(owner, params) {
		const uint32_t verbose = params.find<uint32_t>("verbose", 0);
		out = new Output("CopyGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

		readAddr  = params.find<uint64_t>("read_start_address",  0);
		reqLength = params.find<uint64_t>("operandwidth", 8);
		itemCount = params.find<uint64_t>("request_count", 1024);

		n_per_call = params.find<uint64_t>("n_per_call", 2);

		// Write address default is sized for number of requests * req lengtgh
		writeAddr = params.find<uint64_t>("write_start_address",
			readAddr + (reqLength * itemCount));

		// Start generator at 0
		nextItem = 0;

		out->verbose(CALL_INFO, 1, 0, "Copy count is      %" PRIu64 "\n", itemCount);
		out->verbose(CALL_INFO, 1, 0, "operandwidth       %" PRIu64 "\n", reqLength);
		out->verbose(CALL_INFO, 1, 0, "read start       0x%" PRIx64 "\n", readAddr);
		out->verbose(CALL_INFO, 1, 0, "writestart       0x%" PRIx64 "\n", writeAddr);
		out->verbose(CALL_INFO, 1, 0, "N-per-generate     %" PRIu64 "\n", n_per_call);
	}

	~CopyGenerator() {
		delete out;
	}

	void generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	for ( int i = 0; i < n_per_call; i++ ) {
		if(nextItem == itemCount) {
			return;
		}

		MemoryOpRequest* read = new MemoryOpRequest(readAddr  + (nextItem * reqLength), reqLength, READ);
		MemoryOpRequest* write = new MemoryOpRequest(writeAddr + (nextItem * reqLength), reqLength, WRITE);
		write->addDependency(read->getRequestID());
		q->push_back(read);
		q->push_back(write);

		nextItem++;
    }
	}

	bool isFinished() {
		return (nextItem == itemCount);
	}

	void completed() {}

	SST_ELI_REGISTER_SUBCOMPONENT(
        	CopyGenerator,
       		"miranda",
        	"CopyGenerator",
       		SST_ELI_ELEMENT_VERSION(1,0,0),
       		"Creates a single copy of stream of reads/writes replicating an array copy pattern",
       		"SST::Miranda::RequestGenerator"
    	)

   	SST_ELI_DOCUMENT_PARAMS(
		{ "read_start_address",  "Sets the start read address for this generator", "0" },
    		{ "write_start_address", "Sets the start target address for writes for the generator", "1024" },
    		{ "request_size",        "Sets the size of each request in bytes", "8" },
    		{ "request_count",       "Sets the number of items to be copied", "128" },
    		{ "verbose",             "Sets the verbosity of the output", "0" }
    	)

private:
	uint64_t nextItem;
	uint64_t readAddr;
	uint64_t writeAddr;
	uint64_t itemCount;
	uint64_t reqLength;
	uint64_t n_per_call;
	Output*  out;

};

}
}

#endif
