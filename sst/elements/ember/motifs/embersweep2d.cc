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

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/math/sqrt.h>

#include "embersweep2d.h"

using namespace SST::Ember;

EmberSweep2DGenerator::EmberSweep2DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	outer_iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	inner_iterations = (uint32_t) params.find_integer("generator.inner_iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 1000);
	messageSize = (uint32_t) params.find_integer("generator.messagesize", 128);

}

void EmberSweep2DGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	size = worldSize;
}

void EmberSweep2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < outer_iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);
	}
}
