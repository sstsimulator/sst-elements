
#ifndef _H_EMBER_BARRIER_MOTIF
#define _H_EMBER_BARRIER_MOTIF

#include <sst/core/params.h>
#include "embergen.h"
#include "emberbarrierev.h"

namespace SST {
namespace Ember {

class EmberBarrierGenerator : public EmberGenerator {

public:
	EmberBarrierGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
	void finish(const SST::Output* output) { }
private:
	uint32_t iterations;

};

}
}

#endif
