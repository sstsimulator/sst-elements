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
#include "emberfini.h"

using namespace SST::Ember;

EmberFiniGenerator::EmberFiniGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {
}

void EmberFiniGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
    size = worldSize;
}

void EmberFiniGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

	EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
	evQ->push(finalize);
}
