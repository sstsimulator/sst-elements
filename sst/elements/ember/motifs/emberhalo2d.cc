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

#include "emberhalo2d.h"
#include "emberwaitev.h"
#include "emberirecvev.h"
#include "embersendev.h"

using namespace SST::Ember;

EmberHalo2DGenerator::EmberHalo2DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("iterations", 10);
	nsCompute = (uint32_t) params.find_integer("computenano", 10);
	messageSizeX = (uint32_t) params.find_integer("messagesizey", 128);
	messageSizeY = (uint32_t) params.find_integer("messagesizex", 128);

	sizeX = (uint32_t) params.find_integer("sizex", 0);
	sizeY = (uint32_t) params.find_integer("sizey", 0);

	// Set configuration so we do not exchange messages
	procWest  = 0;
	procEast = 0;
	procNorth = 0;
	procSouth = 0;

	sendWest = false;
	sendEast = false;
	sendNorth = false;
	sendSouth = false;

	messageCount = 0;
}

void EmberHalo2DGenerator::finish(const SST::Output* output) {
	output->verbose(CALL_INFO, 2, 0, "Generator finishing, sent: %" PRIu32 " messages.\n", messageCount);
}

void EmberHalo2DGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	//size = worldSize;

	// Do we need to auto-size the 2D processor array?
	if(0 == sizeX || 0 == sizeY) {
		uint32_t localX = SST::Math::square_root(worldSize);

		while(localX >= 1) {
			uint32_t localY = (uint32_t) worldSize / localX;

			if(localY * localX == worldSize) {
				break;
			}

			localX--;
		}

		sizeX = localX;
		sizeY = worldSize / sizeX;
	}

	// Check we are not leaving ranks behind
	assert(worldSize == (sizeX * sizeY));

	output->verbose(CALL_INFO, 2, 0, "Processor grid dimensions, X=%" PRIu32 ", Y=%" PRIu32 "\n",
		sizeX, sizeY);

	if(rank % sizeX > 0) {
		sendWest = true;
		procWest = rank - 1;
	}

	if((rank + 1) % sizeX != 0) {
		sendEast = true;
		procEast = rank + 1;
	}

	if(rank >= sizeX) {
		sendNorth = true;
		procNorth = rank - sizeX;
	}

	if(rank < (worldSize - sizeX)) {
		sendSouth = true;
		procSouth = rank + sizeX;
	}

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", send left=%s %" PRIu32 ", send right= %s %" PRIu32
		", send above=%s %" PRIu32 ", send below=%s %" PRIu32 "\n",
		rank,
		(sendWest  ? "Y" : "N"), procWest,
		(sendEast ? "Y" : "N"), procEast,
		(sendSouth ? "Y" : "N"), procSouth,
		(sendNorth ? "Y" : "N"), procNorth);
}

void EmberHalo2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		output->verbose(CALL_INFO, 2, 0, "Halo 2D motif generating events for phase %" PRIu32 "\n", phase);

		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		// Do the horizontal exchange first
		if(rank % 2 == 0) {
			if(sendEast) {
				EmberRecvEvent*  recvEvEast = new EmberRecvEvent(procEast, messageSizeX, 0, (Communicator) 0);
				EmberSendEvent*  sendEvEast = new EmberSendEvent(procEast, messageSizeX, 0, (Communicator) 0);

				evQ->push(recvEvEast);
				evQ->push(sendEvEast);

				messageCount++;
			}

			if(sendWest) {
				EmberRecvEvent*  recvEvWest = new EmberRecvEvent(procWest, messageSizeX, 0, (Communicator) 0);
				EmberSendEvent*  sendEvWest = new EmberSendEvent(procWest, messageSizeX, 0, (Communicator) 0);

				evQ->push(recvEvWest);
				evQ->push(sendEvWest);

				messageCount++;
			}
		} else {
			if(sendEast) {
				EmberRecvEvent*  recvEvEast = new EmberRecvEvent(procEast, messageSizeX, 0, (Communicator) 0);
				EmberSendEvent*  sendEvEast = new EmberSendEvent(procEast, messageSizeX, 0, (Communicator) 0);

				evQ->push(sendEvEast);
				evQ->push(recvEvEast);

				messageCount++;
			}

			if(sendWest) {
				EmberRecvEvent*  recvEvWest = new EmberRecvEvent(procWest, messageSizeX, 0, (Communicator) 0);
				EmberSendEvent*  sendEvWest = new EmberSendEvent(procWest, messageSizeX, 0, (Communicator) 0);

				evQ->push(sendEvWest);
				evQ->push(recvEvWest);

				messageCount++;
			}
		}

		// Now do the vertical exchanges
		if( (rank / sizeX) % 2 == 0) {
			if(sendNorth) {
				EmberRecvEvent* recvEvNorth = new EmberRecvEvent(procNorth, messageSizeY, 0, (Communicator) 0);
				EmberSendEvent* sendEvNorth = new EmberSendEvent(procNorth, messageSizeY, 0, (Communicator) 0);

				evQ->push(recvEvNorth);
				evQ->push(sendEvNorth);

				messageCount++;
			}

			if(sendSouth) {
				EmberRecvEvent* recvEvSouth = new EmberRecvEvent(procSouth, messageSizeY, 0, (Communicator) 0);
				EmberSendEvent* sendEvSouth = new EmberSendEvent(procSouth, messageSizeY, 0, (Communicator) 0);

				evQ->push(recvEvSouth);
				evQ->push(sendEvSouth);

				messageCount++;
			}
		} else {
			if(sendNorth) {
				EmberRecvEvent* recvEvNorth = new EmberRecvEvent(procNorth, messageSizeY, 0, (Communicator) 0);
				EmberSendEvent* sendEvNorth = new EmberSendEvent(procNorth, messageSizeY, 0, (Communicator) 0);

				evQ->push(sendEvNorth);
				evQ->push(recvEvNorth);

				messageCount++;
			}

			if(sendSouth) {
				EmberRecvEvent* recvEvSouth = new EmberRecvEvent(procSouth, messageSizeY, 0, (Communicator) 0);
				EmberSendEvent* sendEvSouth = new EmberSendEvent(procSouth, messageSizeY, 0, (Communicator) 0);

				evQ->push(sendEvSouth);
				evQ->push(recvEvSouth);

				messageCount++;
			}
		}

		output->verbose(CALL_INFO, 2, 0, "Halo 2D motif completed event generation for phase: %" PRIu32 "\n", phase);
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
