// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_RANDOM_GEN
#define _H_SST_MIRANDA_RANDOM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/rng.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class RandomGenerator : public RequestGenerator {

public:
	SST_ELI_REGISTER_SUBCOMPONENT(
        RandomGenerator,
        "miranda",
        "RandomGenerator",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Creates a random stream of accesses to/from memory",
        SST::Miranda::RequestGenerator
    )

	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose",          "Sets the verbosity output of the generator", "0" },
        { "count",            "Count for number of items being requested", "1024" },
        { "length",           "Length of requests", "8" },
        { "max_address",	  "Maximum address allowed for generation", "16384" },
        { "issue_op_fences",  "Issue operation fences, \"yes\" or \"no\", default is yes", "yes" }
    )

	RandomGenerator( ComponentId_t id, Params& params );
	RandomGenerator() = default;
	~RandomGenerator();
	void build(Params& params);
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

    virtual void serialize_order(SST::Core::Serialization::serializer& ser) override {
        SST::Miranda::RequestGenerator::serialize_order(ser);
		SST_SER(reqLength);
		SST_SER(maxAddr);
		SST_SER(issueCount);
		SST_SER(issueOpFences);
		SST_SER(rng);
		SST_SER(out);
    }

    ImplementSerializable(SST::Miranda::RandomGenerator)

private:
	uint64_t reqLength;
	uint64_t maxAddr;
	uint64_t issueCount;
	bool issueOpFences;
	Random* rng;
	Output*  out;

};

}
}

#endif
