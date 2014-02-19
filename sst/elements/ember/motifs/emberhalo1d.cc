
#include <sst_config.h>

#include <sst/core/component.h>
#include <sst/core/params.h>

#include "emberhalo1d.h"

using namespace SST::Ember;

EmberHalo1DGenerator::EmberHalo1DGenerator(SST::Component* owner, Params& params) {
	iterations = (uint32_t) params.find_integer("generator.iterations", 10);
	nsCompute = (uint32_t) params.find_integer("generator.computenano", 1000);
	messageSize = (uint32_t) params.find_integer("generator.messagesize", 128);
}

void EmberHalo1DGenerator::generate(const SST::Output* output, const uint32_t phase, const uint32_t rank, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);
	} else {
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
		evQ->push(finalize);
	}
}
