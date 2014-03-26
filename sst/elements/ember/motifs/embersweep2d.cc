
#include <sst_config.h>

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/math/sqrt.h>

#include "embersweep2d.h"

using namespace SST::Ember;

EmberSweep2DGenerator::EmberSweep2DGenerator(SST::Component* owner, Params& params) :
	EmberGenerator(owner, params) {

	outer_iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	inner_iterations = (uint32_t) params.find_integer("generator.inner_iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 1000);
	messageSize = (uint32_t) params.find_integer("generator.messagesize", 128);

}

void EmberSweep2DGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	size = worldSize;
}

void EmberSweep2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < outer_iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);
	}
}
