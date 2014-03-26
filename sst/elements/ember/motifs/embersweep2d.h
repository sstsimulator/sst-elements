
#ifndef _H_EMBER_SWEEP_2D
#define _H_EMBER_SWEEP_2D

#include "embergen.h"

namespace SST {
namespace Ember {

class EmberSweep2DGenerator : public EmberGenerator {

public:
	EmberSweep2DGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);

private:
	uint32_t rank;
	uint32_t size;

	uint32_t nsCompute;
	uint32_t messageSize;
	uint32_t outer_iterations;
	uint32_t inner_iterations;

};

}
}

#endif
