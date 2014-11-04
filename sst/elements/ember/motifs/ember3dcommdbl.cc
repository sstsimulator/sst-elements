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

#include "ember3dcommdbl.h"
#include "emberirecvev.h"
#include "emberisendev.h"
#include "emberwaitallev.h"
#include "emberallredev.h"

using namespace SST::Ember;

Ember3DCommDoublingGenerator::Ember3DCommDoublingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	peX = (uint32_t) params.find_integer("pex", 0);
	peY = (uint32_t) params.find_integer("pey", 0);
	peZ = (uint32_t) params.find_integer("pez", 0);

	basePhase = (uint32_t) params.find_integer("basephase", 0);

	items_per_node = (uint32_t) params.find_integer("items_per_node", 64);
	itemsThisPhase = items_per_node;

	computeBetweenSteps = (uint32_t) params.find_integer("compute_at_step", 2000);

	requests = (MessageRequest*) malloc(sizeof(MessageRequest) * 52);
	next_request = 0;
}

Ember3DCommDoublingGenerator::~Ember3DCommDoublingGenerator() {

}

void Ember3DCommDoublingGenerator::configureEnvironment(const SST::Output* output, uint32_t myRank, uint32_t worldSize) {
	rank = myRank;

	if((peX * peY * peZ) != worldSize) {
		output->fatal(CALL_INFO, -1, "Processor decomposition of %" PRIu32 "x%" PRIu32 "x%" PRIu32 " != rank count of %" PRIu32 "\n",
			peX, peY, peZ, worldSize);
	}

}

int32_t Ember3DCommDoublingGenerator::power3(const uint32_t expon) {
	if(0 == expon) {
		return 1;
	} else {
		int32_t the_pow = 3;
		for(uint32_t i = 0; i < expon; ++i) {
			the_pow *= 3;
		}

		return the_pow;
	}
}

void Ember3DCommDoublingGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(0 == rank) {
		output->verbose(CALL_INFO, 1, 0, "Motif executing phase %" PRIu32 "...\n", phase);
	}

	if(computeBetweenSteps > 0) {
		evQ->push(new EmberComputeEvent(computeBetweenSteps));
	}

	next_request = 0;

	int32_t myX, myY, myZ = 0;
	getPosition(rank, peX, peY, peZ, &myX, &myY, &myZ);

	const int32_t comm_delta = power3(phase) + basePhase;
	int32_t next_comm_rank = 0;
	itemsThisPhase = (uint32_t) (itemsThisPhase + 1124) * 3;

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY, myZ);

	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);

	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY, myZ + comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY, myZ - comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}


	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		EmberISendEvent* send = new EmberISendEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		EmberIRecvEvent* recv = new EmberIRecvEvent(next_comm_rank, itemsThisPhase,
			0, (Communicator) 0, &requests[next_request]);
		next_request++;

		evQ->push(send);
		evQ->push(recv);
	}

	// If we set up any communication wait for them and continue
	// If not, we exit by notifying engine of a finalize
	if(next_request > 0) {
		evQ->push( new EmberWaitallEvent( next_request, &requests[0], false ) );
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
        	evQ->push(finalize);
	}
}

void Ember3DCommDoublingGenerator::finish(const SST::Output* output) {

}
