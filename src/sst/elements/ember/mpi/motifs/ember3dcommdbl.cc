// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "ember3dcommdbl.h"

using namespace SST::Ember;

Ember3DCommDoublingGenerator::Ember3DCommDoublingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "3DCommDoubling"), 
	phase(0)
{
	peX = (uint32_t) params.find("arg.pex", 0);
	peY = (uint32_t) params.find("arg.pey", 0);
	peZ = (uint32_t) params.find("arg.pez", 0);

	basePhase = (uint32_t) params.find("arg.basephase", 0);

	items_per_node = (uint32_t) params.find("arg.items_per_node", 64);
	itemsThisPhase = items_per_node;

	computeBetweenSteps = (uint32_t) params.find("arg.compute_at_step", 2000);

	requests = (MessageRequest*) malloc(sizeof(MessageRequest) * 52);

	configure();
}

void Ember3DCommDoublingGenerator::configure()
{
	if((peX * peY * peZ) != (unsigned) size()) {
		fatal(CALL_INFO, -1, "Processor decomposition of %" PRIu32 "x%" PRIu32 "x%" PRIu32 " != rank count of %" PRIu32 "\n",
			peX, peY, peZ, size());
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

bool Ember3DCommDoublingGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
	if(0 == rank()) {
		verbose(CALL_INFO, 1, 0, "Motif executing phase %" PRIu32 "...\n", phase);
	}
	++phase;

	if(computeBetweenSteps > 0) {
		enQ_compute( evQ, computeBetweenSteps );
	}

	next_request = 0;

	int32_t myX, myY, myZ = 0;
	getPosition(rank(), peX, peY, peZ, &myX, &myY, &myZ);

	const int32_t comm_delta = power3(phase) + basePhase;
	int32_t next_comm_rank = 0;
	itemsThisPhase = (uint32_t) (itemsThisPhase + 1124) * 3;

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY, myZ);

	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY, myZ + comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX, myY, myZ - comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY + comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}


	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX + comm_delta, myY - comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY + comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ + comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	next_comm_rank = convertPositionToRank(peX, peY, peZ, myX - comm_delta, myY - comm_delta, myZ - comm_delta);
	if(next_comm_rank > -1) {
		enQ_isend( evQ, next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;

		enQ_irecv( evQ ,next_comm_rank, itemsThisPhase,
			0, GroupWorld, &requests[next_request]);
		next_request++;
	}

	// If we set up any communication wait for them and continue
	// If not, we exit by notifying engine of a finalize
	if(next_request > 0) {
		enQ_waitall( evQ, next_request, &requests[0], NULL );
		return false;
	} else {
		return true;
	}
}
