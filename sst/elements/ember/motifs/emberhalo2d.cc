
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
	EmberGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 10);
	messageSizeX = (uint32_t) params.find_integer("generator.messagesizey", 128);
	messageSizeY = (uint32_t) params.find_integer("generator.messagesizex", 128);

	uint32_t ordering = (uint32_t) params.find_integer("generator.messageorder", 0);
	if(0 == ordering) {
		xBeforeY = true;
	} else {
		xBeforeY = false;
	}

	sizeX = (uint32_t) params.find_integer("generator.sizex", 0);
	sizeY = (uint32_t) params.find_integer("generator.sizey", 0);

	// Set configuration so we do not exchange messages
	procLeft  = 0;
	procRight = 0;
	procAbove = 0;
	procBelow = 0;

	sendLeft = false;
	sendRight = false;
	sendAbove = false;
	sendBelow = false;
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
		sendLeft = true;
		procLeft = rank - 1;
	}

	if((rank + 1) % sizeX != 0) {
		sendRight = true;
		procRight = rank + 1;
	}

	if(rank > sizeX) {
		sendAbove = true;
		procAbove = rank - sizeX;
	}

	if(rank < (worldSize - sizeX)) {
		sendBelow = true;
		procBelow = rank + sizeX;
	}

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", send left=%s %" PRIu32 ", send right= %s %" PRIu32
		", send above=%s %" PRIu32 ", send below=%s %" PRIu32 "\n",
		rank,
		(sendLeft  ? "Y" : "N"), procLeft,
		(sendRight ? "Y" : "N"), procRight,
		(sendBelow ? "Y" : "N"), procBelow,
		(sendAbove ? "Y" : "N"), procAbove);
}

void EmberHalo2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		output->verbose(CALL_INFO, 2, 0, "Halo 2D motif generating events for phase %" PRIu32 "\n", phase);

		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		if(sendLeft) {
			MessageRequest*  leftReq    = new MessageRequest();
			EmberIRecvEvent* recvEvLeft = new EmberIRecvEvent(procLeft, sizeX, 0, (Communicator) 0, leftReq);
			EmberSendEvent*  sendEvLeft = new EmberSendEvent(procLeft, sizeX, 0, (Communicator) 0);
			EmberWaitEvent*  waitEvLeft = new EmberWaitEvent(leftReq);

			evQ->push(recvEvLeft);
			evQ->push(sendEvLeft);
			evQ->push(waitEvLeft);
		}

		if(sendRight) {
			MessageRequest*  rightReq    = new MessageRequest();
			EmberIRecvEvent* recvEvRight = new EmberIRecvEvent(procRight, sizeX, 0, (Communicator) 0, rightReq);
			EmberSendEvent*  sendEvRight = new EmberSendEvent(procRight, sizeX, 0, (Communicator) 0);
			EmberWaitEvent*  waitEvRight = new EmberWaitEvent(rightReq);

			evQ->push(recvEvRight);
			evQ->push(sendEvRight);
			evQ->push(waitEvRight);
		}

		if(sendAbove) {
			MessageRequest*  aboveReq    = new MessageRequest();
			EmberIRecvEvent* recvEvAbove = new EmberIRecvEvent(procAbove, sizeY, 0, (Communicator) 0, aboveReq);
			EmberSendEvent*  sendEvAbove = new EmberSendEvent(procAbove, sizeY, 0, (Communicator) 0);
			EmberWaitEvent*  waitEvAbove = new EmberWaitEvent(aboveReq);

			evQ->push(recvEvAbove);
			evQ->push(sendEvAbove);
			evQ->push(waitEvAbove);
		}

		if(sendBelow) {
			MessageRequest*  belowReq    = new MessageRequest();
			EmberIRecvEvent* recvEvBelow = new EmberIRecvEvent(procBelow, sizeY, 0, (Communicator) 0, belowReq);
			EmberSendEvent*  sendEvBelow = new EmberSendEvent(procBelow, sizeY, 0, (Communicator) 0);
			EmberWaitEvent*  waitEvBelow = new EmberWaitEvent(belowReq);

			evQ->push(recvEvBelow);
			evQ->push(sendEvBelow);
			evQ->push(waitEvBelow);
		}

		output->verbose(CALL_INFO, 2, 0, "Halo 2D motif completed event generation for phase: %" PRIu32 "\n", phase);
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
