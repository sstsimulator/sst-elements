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


#include <sst_config.h>
#include "emberallreduce.h"

using namespace SST::Ember;

EmberAllreduceGenerator::EmberAllreduceGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("iterations", 1);
	count      = (uint32_t) params.find_integer("count", 1);

}

void EmberAllreduceGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
}

void EmberAllreduceGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

	if(phase < iterations) {
		EmberAllreduceEvent* allred = new EmberAllreduceEvent(count, EMBER_F64, EMBER_SUM, (Communicator) 0);
		evQ->push(allred);
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
