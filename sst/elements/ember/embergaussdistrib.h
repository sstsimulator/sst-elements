
#ifndef _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION

#include "emberdistrib.h"
#include <sst/core/rng/gaussian.h>

namespace SST {
namespace Ember {

class EmberGaussianDistribution : public EmberComputeDistribution {

public:
	EmberGaussianDistribution(Component* owner, Params& params);
	~EmberGaussianDistribution();
	double sample(uint64_t now);

private:
	SSTGaussianDistribution* distrib;

};

}
}

#endif
