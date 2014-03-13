
#include "emberfini.h"

using namespace SST::Ember;

EmberFiniGenerator::EmberFiniGenerator(SST::Component* owner, Params& params) :
	EmberGenerator(owner, params) {
}

void EmberFiniGenerator::configureEnvironment(uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
    size = worldSize;
}

void EmberFiniGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

	EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
	evQ->push(finalize);
}
