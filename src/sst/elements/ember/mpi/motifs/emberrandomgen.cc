
#include <sst_config.h>

#include <sst/core/rng/xorshift.h>
#include "emberrandomgen.h"

using namespace SST::Ember;
using namespace SST::RNG;

EmberRandomTrafficGenerator::EmberRandomTrafficGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	msgSize = (uint32_t) params.find_integer("arg.messagesize", 1);
	maxIterations = (uint32_t) params.find_integer("arg.iterations", 1);
	iteration = 0;
}

bool EmberRandomTrafficGenerator::generate( std::queue<EmberEvent*>& evQ ) {

	if(iteration == maxIterations) {
		return true;
	} else {
		XORShiftRNG* rng = new XORShiftRNG(size() + iteration);

		const uint32_t worldSize = size();
		uint32_t* rankArray = (uint32_t*) malloc(sizeof(uint32_t) * worldSize);

		// Set up initial passing (everyone passes to themselves)
		for(uint32_t i = 0; i < worldSize; i++) {
			rankArray[i] = i;
		}

		for(uint32_t i = 0; i < worldSize; i++) {
			uint32_t swapPartner = rng->generateNextUInt32() % worldSize;

			uint32_t oldValue = rankArray[i];
			rankArray[i] = rankArray[swapPartner];
			rankArray[swapPartner] = oldValue;
		}

		void* allocBuffer = memAlloc( msgSize * 8 );
		int nextMRIndex = 0;

		MessageRequest* msgRequests = (MessageRequest*) malloc(sizeof(MessageRequest) * 2);

		if(rankArray[rank()] != rank()) {
			enQ_isend( evQ, allocBuffer, msgSize, DOUBLE, rankArray[rank()],
				0, GroupWorld, &msgRequests[nextMRIndex++]);
		}

		void* recvBuffer = memAlloc( msgSize * 8 );

		for(uint32_t i = 0; i < worldSize; i++) {
			if(rank() == rankArray[i] && (i != rank())) {
				enQ_irecv( evQ, recvBuffer, msgSize, DOUBLE, i, 0,
					GroupWorld, &msgRequests[nextMRIndex++]);
				break;
			}
		}

		enQ_waitall( evQ, nextMRIndex, msgRequests, NULL );

		iteration++;
		delete rng;

		return false;
	}
}


