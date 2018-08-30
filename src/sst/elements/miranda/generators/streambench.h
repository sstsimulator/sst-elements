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


#ifndef _H_SST_MIRANDA_STREAM_BENCH_GEN
#define _H_SST_MIRANDA_STREAM_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class STREAMBenchGenerator : public RequestGenerator {

public:
	STREAMBenchGenerator( Component* owner, Params& params );
	~STREAMBenchGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

	SST_ELI_REGISTER_SUBCOMPONENT(
                STREAMBenchGenerator,
                "miranda",
                "STREAMBenchGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0),
		"Creates a representation of the STREAM benchmark",
                "SST::Miranda::RequestGenerator"
        )

	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose",          "Sets the verbosity output of the generator", "0" },
    		{ "n",                "Sets the number of elements in the STREAM arrays", "10000" },
    		{ "n_per_call",       "Sets the number of iterations to generate per call to the generation function", "1"},
    		{ "operandwidth",     "Sets the length of the request, default=8 (i.e. one double)", "8" },
    		{ "start_a",          "Sets the start address of the array a", "0" },
    		{ "start_b",          "Sets the start address of the array b", "1024" },
    		{ "start_c",          "Sets the start address of the array c", "2048" }
       	)

private:
	uint64_t reqLength;

	uint64_t start_a;
	uint64_t start_b;
	uint64_t start_c;

	uint64_t n;
	uint64_t n_per_call;
	uint64_t i;

	Output*  out;

};

}
}

#endif
