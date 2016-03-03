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


#ifndef _H_SST_MIRANDA_RANDOM_GEN
#define _H_SST_MIRANDA_RANDOM_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>
#include <sst/core/rng/sstrng.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class RandomGenerator : public RequestGenerator {

public:
	RandomGenerator( Component* owner, Params& params );
	~RandomGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;
	uint64_t maxAddr;
	uint64_t issueCount;
	bool issueOpFences;
	SSTRandom* rng;
	Output*  out;

};

}
}

#endif
