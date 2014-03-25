
#ifndef _H_EMBER_RING
#define _H_EMBER_RING

#include <sst/core/params.h>
#include "embergen.h"

namespace SST {
namespace Ember {

class EmberRingGenerator : public EmberGenerator {

public:
	EmberRingGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);

private:
    uint32_t size;
	uint32_t rank;
	uint32_t messageSize;
	uint32_t iterations;

};

}
}

#endif
