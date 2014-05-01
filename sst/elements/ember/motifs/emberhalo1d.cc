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

#include "emberhalo1d.h"

using namespace SST::Ember;

EmberHalo1DGenerator::EmberHalo1DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("iterations", 10);
	nsCompute = (uint32_t) params.find_integer("computenano", 1000);
	messageSize = (uint32_t) params.find_integer("messagesize", 128);
}

void EmberHalo1DGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	size = worldSize;
}

void EmberHalo1DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		if(0 == rank) {
			EmberRecvEvent* recvRight = new EmberRecvEvent(1, messageSize, 0, (Communicator) 0);
			EmberSendEvent* sendRight = new EmberSendEvent(1, messageSize, 0, (Communicator) 0);

			evQ->push(recvRight);
			evQ->push(sendRight);
		} else if( (size - 1) == rank ) {
			EmberSendEvent* sendLeft = new EmberSendEvent(rank - 1, messageSize, 0, (Communicator) 0);
			EmberRecvEvent* recvLeft = new EmberRecvEvent(rank - 1, messageSize, 0, (Communicator) 0);

			evQ->push(sendLeft);
			evQ->push(recvLeft);
		} else {
			EmberSendEvent* sendLeft = new EmberSendEvent(rank - 1, messageSize, 0, (Communicator) 0);
			EmberRecvEvent* recvRight = new EmberRecvEvent(rank + 1, messageSize, 0, (Communicator) 0);
			EmberSendEvent* sendRight = new EmberSendEvent(rank + 1, messageSize, 0, (Communicator) 0);
			EmberRecvEvent* recvLeft =new EmberRecvEvent(rank - 1, messageSize, 0, (Communicator) 0);

			evQ->push(sendLeft);
			evQ->push(recvRight);
			evQ->push(sendRight);
			evQ->push(recvLeft);
		}
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
