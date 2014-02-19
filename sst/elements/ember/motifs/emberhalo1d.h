
#ifndef _H_EMBER_HALO_1D
#define _H_EMBER_HALO_1D

#include "embergen.h"

namespace SST {
namespace Ember {

class EmberHalo1DGenerator : public EmberGenerator {

public:
	EmberHalo1DGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, const uint32_t rank, std::queue<EmberEvent*>* evQ);

private:
	uint32_t rank;
	uint32_t size;
	uint32_t nsCompute;
	uint32_t messageSize;
	uint32_t iterations;
	uint32_t wrapAround;

};

}
}

#endif
