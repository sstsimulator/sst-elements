
#ifndef _H_SST_EMBER_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_COMPUTE_DISTRIBUTION

#include <sst/core/module.h>
#include <sst/core/params.h>

namespace SST {
namespace Ember {

class EmberComputeDistribution : public SST::Module {

public:
	EmberComputeDistribution(Component* owner, Params& params);
	~EmberComputeDistribution();
	virtual double sample(uint64_t now) = 0;

};

}
}

#endif
