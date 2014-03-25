
#include <sst_config.h>

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/math/sqrt.h>

#include "emberhalo2d.h"

using namespace SST::Ember;

EmberHalo2DGenerator::EmberHalo2DGenerator(SST::Component* owner, Params& params) :
	EmberGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 1000);
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
		(sendLeft ? "Y" : "N"), procLeft,
		(sendRight ? "Y" : "N"), procRight,
		(sendBelow ? "Y" : "N"), procBelow,
		(sendAbove ? "Y" : "N"), procAbove);
}

void EmberHalo2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		if(xBeforeY) {
			if(procLeft > -1) {

			}

			if(procRight > -1) {

			}

			if(procAbove > -1) {

			}

			if(procBelow > -1) {

			}

		} else {
			if(procAbove > -1) {

			}

			if(procBelow > -1) {

			}

			if(procLeft > -1) {

			}

			if(procRight > -1) {

			}
		}


/*		if(0 == rank) {
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
*/
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
