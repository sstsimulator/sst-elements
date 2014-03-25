
#ifndef _H_EMBER_FINI
#define _H_EMBER_FINI

#include <sst/core/params.h>
#include "embergen.h"

namespace SST {
namespace Ember {

class EmberFiniGenerator : public EmberGenerator {

public:
	EmberFiniGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);

private:
    uint32_t size;
	uint32_t rank;
};

}
}

#endif
