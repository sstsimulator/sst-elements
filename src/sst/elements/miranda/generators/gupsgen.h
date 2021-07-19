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


#ifndef _H_SST_MIRANDA_GUPS_GEN
#define _H_SST_MIRANDA_GUPS_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/sstrng.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class GUPSGenerator : public RequestGenerator {

public:
	GUPSGenerator( ComponentId_t id, Params& params );
        void build(Params &params);
	~GUPSGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
               	GUPSGenerator,
               	"miranda",
                "GUPSGenerator",
               	SST_ELI_ELEMENT_VERSION(1,0,0),
		"Creates a random stream of accesses to read-modify-write",
                SST::Miranda::RequestGenerator
       	)

        SST_ELI_DOCUMENT_PARAMS(
		{ "verbose",          "Sets the verbosity output of the generator", "0" },
   	 	{ "seed_a",           "Sets the seed-a for the random generator", "11" },
    		{ "seed_b",           "Sets the seed-b for the random generator", "31" },
    		{ "count",            "Count for number of items being requested", "1024" },
    		{ "length",           "Length of requests", "8" },
    		{ "iterations",       "Number of iterations to perform", "1" },
    		{ "max_address",      "Maximum address allowed for generation", "536870912" /* 512MB */ },
	    	{ "issue_op_fences",  "Issue operation fences, \"yes\" or \"no\", default is yes", "yes" }
        )

private:
	uint64_t reqLength;

	uint64_t memLength;
	uint64_t memStart;
	uint64_t issueCount;
	uint64_t iterations;
	uint64_t seed_a;
	uint64_t seed_b;
	SSTRandom* rng;
	Output*  out;
	bool issueOpFences;

};

}
}

#endif
