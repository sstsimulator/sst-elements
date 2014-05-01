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
#include "emberring.h"

using namespace SST::Ember;

EmberRingGenerator::EmberRingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	messageSize = (uint32_t) params.find_integer("messagesize", 1024);
	iterations = (uint32_t) params.find_integer("iterations", 1);
}

void EmberRingGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
    size = worldSize;
	output->verbose(CALL_INFO, 2, 0,"rank=%d size=%d\n",rank,size);
}


void EmberRingGenerator::finish(const SST::Output* output) {
    output->verbose(CALL_INFO, 2, 0, "Generator finishing\n");
}


inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

void EmberRingGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

    uint32_t to = mod(rank + 1, size), from = mod( (signed int) rank - 1, size );
    output->verbose(CALL_INFO, 2, 0,"to=%d from=%d\n",to,from);

	if(phase < iterations) {

		EmberSendEvent* send = new EmberSendEvent((uint32_t) to, messageSize, 0, (Communicator) 0);
		EmberRecvEvent* recv = new EmberRecvEvent((uint32_t) from, messageSize, 0, (Communicator) 0);

		if(0 == rank) {
			evQ->push(send);
			evQ->push(recv);
		} else {
			evQ->push(recv);
			evQ->push(send);
		}

	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
