
#include "emberbarrier.h"

using namespace SST::Ember;

EmberBarrierGenerator::EmberBarrierGenerator(SST::Component* owner, Params& params) :
	EmberGenerator(owner, params) {

	iterations = (uint32_t) params.find_integer("iterations", 1);
}

void EmberBarrierGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
}

void EmberBarrierGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

	if(phase < iterations) {
		EmberBarrierEvent* barrier = new EmberBarrierEvent((Communicator) 0);
		evQ->push(barrier);
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
