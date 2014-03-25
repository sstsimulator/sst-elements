
#include <sst_config.h>

#include <sst/core/component.h>
#include <sst/core/params.h>

#include "emberhalo2d.h"

using namespace SST::Ember;

EmberHalo2DGenerator::EmberHalo2DGenerator(SST::Component* owner, Params& params) :
	EmberGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 1000);
	messageSizeX = (uint32_t) params.find_integer("generator.messagesizey", 128);
	messageSizeY = (uint32_t) params.find_integer("generator.messagesizex", 128);

	sizeX = (uint32_t) params.find_integer("generator.sizex", 0);
	sizeY = (uint32_t) params.find_integer("generator.sizey", 0);

	// Set configuration so we do not exchange messages
	procLeft = -1;
	procRight = -1;
	procAbove = -1;
	procBelow = -1;
}

void EmberHalo2DGenerator::configureEnvironment(uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	//size = worldSize;

	// Do we need to auto-size the 2D processor array?
	if(0 == sizeX || 0 == sizeY) {
		uint32_t localX = (worldSize / 2);

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

	if(rank % sizeX == 0) {
		procLeft = (int32_t) rank - 1;
	}

	if((rank + 1) % sizeX == 0) {
		procRight = (int32_t) rank + 1;
	}

	if(rank < sizeX) {
		procBelow = (int32_t) rank - sizeX;
	}

	if(rank > (worldSize - sizeX)) {
		procAbove = (int32_t) rank + sizeX;
	}
}

void EmberHalo2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		if(procLeft > -1) {

		}

		if(procRight > -1) {

		}

		if(procAbove > -1) {

		}

		if(procBelow > -1) {

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
