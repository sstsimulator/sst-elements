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
#include "emberpingpong.h"

using namespace SST::Ember;

EmberPingPongGenerator::EmberPingPongGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	messageSize = (uint32_t) params.find_integer("messagesize", 1024);
	iterations = (uint32_t) params.find_integer("iterations", 1);
}

void EmberPingPongGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
}

void EmberPingPongGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

	if(phase < iterations) {
		if(0 == rank) {
			EmberSendEvent* zeroSend = new EmberSendEvent((uint32_t) 1, messageSize, 0, (Communicator) 0);
			EmberRecvEvent* zeroRecv = new EmberRecvEvent((uint32_t) 1, messageSize, 0, (Communicator) 0);

			evQ->push(zeroSend);
			evQ->push(zeroRecv);
		} else if (1 == rank) {
			EmberSendEvent* oneSend = new EmberSendEvent((uint32_t) 0, messageSize, 0, (Communicator) 0);
			EmberRecvEvent* oneRecv = new EmberRecvEvent((uint32_t) 0, messageSize, 0, (Communicator) 0);

			evQ->push(oneRecv);
			evQ->push(oneSend);
		} else {
			EmberFinalizeEvent* endSimFinalize = new EmberFinalizeEvent();
			evQ->push(endSimFinalize);
		}
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
