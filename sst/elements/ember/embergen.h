
#ifndef _H_EMBER_GENERATOR
#define _H_EMBER_GENERATOR

#include <sst_config.h>
#include <sst/core/output.h>
#include <sst/core/module.h>

#include <queue>

#include "emberevent.h"
#include "embersendev.h"
#include "emberrecvev.h"
#include "emberinitev.h"
#include "embercomputeev.h"
#include "emberfinalizeev.h"

namespace SST {
namespace Ember {

class EmberGenerator : public Module {

public:
	EmberGenerator();
	virtual void generate(const SST::Output* output, const uint32_t phase, const uint32_t rank, std::queue<EmberEvent*>* evQ) = 0;

protected:
	~EmberGenerator();

};

}
}

#endif
