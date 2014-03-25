
#ifndef _H_EMBER_HALO_2D
#define _H_EMBER_HALO_2D

#include "embergen.h"

namespace SST {
namespace Ember {

class EmberHalo2DGenerator : public EmberGenerator {

public:
	EmberHalo2DGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);

private:
	uint32_t rank;
	uint32_t sizeX;
	uint32_t sizeY;
	uint32_t nsCompute;
	uint32_t messageSizeX;
	uint32_t messageSizeY;
	uint32_t iterations;

	int32_t  procLeft;
	int32_t  procRight;
	int32_t  procAbove;
	int32_t  procBelow;

};

}
}

#endif
