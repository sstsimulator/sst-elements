// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
	GUPSGenerator( Component* owner, Params& params );
	~GUPSGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

private:
	uint64_t reqLength;
	uint64_t maxAddr;
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
