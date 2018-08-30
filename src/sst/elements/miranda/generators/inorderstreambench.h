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


#ifndef _H_SST_MIRANDA_INORDER_STREAM_BENCH_GEN
#define _H_SST_MIRANDA_INORDER_STREAM_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class InOrderSTREAMBenchGenerator : public RequestGenerator {

public:
	InOrderSTREAMBenchGenerator( Component* owner, Params& params ) :
		RequestGenerator(owner, params) {

		const uint32_t verbose = params.find<uint32_t>("verbose", 0);
		out = new Output("InOrderSTREAMBench[@p:@l]: ", verbose, 0,
			Output::STDOUT);

		n = params.find<uint64_t>("n", 10000);
		requestLen = params.find<uint64_t>("operandwidth", 8);

		start_a = params.find<uint64_t>("start_a", 0);
		start_b = params.find<uint64_t>("start_b", start_a + (n * requestLen));
		start_c = params.find<uint64_t>("start_c", start_b + (n * requestLen));

		block_per_call = params.find<uint64_t>("block_per_call", 1);

		i = 0;
	}

	~InOrderSTREAMBenchGenerator() {
		delete out;
	}

	void generate(MirandaRequestQueue<GeneratorRequest*>* q) {
		if(i == n) {
			return;
		}

		MemoryOpRequest** b_reads  = (MemoryOpRequest**) malloc(sizeof(MemoryOpRequest*) * block_per_call);
		MemoryOpRequest** c_reads  = (MemoryOpRequest**) malloc(sizeof(MemoryOpRequest*) * block_per_call);
		MemoryOpRequest** a_writes = (MemoryOpRequest**) malloc(sizeof(MemoryOpRequest*) * block_per_call);

		for(uint32_t j = 0; j < block_per_call; j++) {
			b_reads[j]  = new MemoryOpRequest(start_b + ((i + j) * requestLen), requestLen, READ);
			c_reads[j]  = new MemoryOpRequest(start_c + ((i + j) * requestLen), requestLen, READ);
			a_writes[j] = new MemoryOpRequest(start_a + ((i + j) * requestLen), requestLen, WRITE);

			a_writes[j]->addDependency(b_reads[j]->getRequestID());
			a_writes[j]->addDependency(c_reads[j]->getRequestID());
		}

		// Issue all the reads
		for(uint32_t j = 0; j < block_per_call; j++) {
			q->push_back(b_reads[j]);
			q->push_back(c_reads[j]);
		}

		// Then issue all the writes
		for(uint32_t j = 0; j < block_per_call; j++) {
			q->push_back(a_writes[j]);
		}

		free(b_reads);
		free(c_reads);
		free(a_writes);

		i += block_per_call;
	}

	bool isFinished() {
		return i == n;
	}

	void completed() {}

       	SST_ELI_REGISTER_SUBCOMPONENT(
               	InOrderSTREAMBenchGenerator,
                "miranda",
                "InOrderSTREAMBenchGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0),
		"Creates a representation of the STREAM benchmark for in-order CPUs",
                "SST::Miranda::RequestGenerator"
        )

	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose",          "Sets the verbosity output of the generator", "0" },
    		{ "n",                "Sets the number of elements in the STREAM arrays", "10000" },
    		{ "block_per_call",   "Sets the number of iterations to generate per call to the generation function", "1"},
    		{ "operandwidth",     "Sets the length of the request, default=8 (i.e. one double)", "8" },
    		{ "start_a",          "Sets the start address of the array a", "0" },
    		{ "start_b",          "Sets the start address of the array b", "1024" },
    		{ "start_c",          "Sets the start address of the array c", "2048" },
        )

private:
	uint64_t requestLen;

	uint64_t start_a;
	uint64_t start_b;
	uint64_t start_c;

	uint64_t n;
	uint64_t block_per_call;
	uint64_t i;

	Output*  out;

};

}
}

#endif
