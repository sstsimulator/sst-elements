// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_ALL_PINGPONG_MOTIF
#define _H_EMBER_ALL_PINGPONG_MOTIF

#include <sst/core/params.h>

#include "embergen.h"
#include "embermpigen.h"
#include "embersendev.h"
#include "emberrecvev.h"

namespace SST {
namespace Ember {

class EmberAllPingPongGenerator : public EmberMessagePassingGenerator {

public:
	EmberAllPingPongGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
	void finish(const SST::Output* output) { }

private:
	uint32_t iterations;
	uint32_t commWithRank;
	uint32_t messageSize;
	uint32_t myRank;
	uint32_t halfWorld;
	uint32_t computeTime;

};

}
}

#endif
