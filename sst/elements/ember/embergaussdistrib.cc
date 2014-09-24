
#include <sst_config.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "emberdistrib.h"
#include "embergaussdistrib.h"

using namespace SST::Ember;
using namespace SST::RNG;

EmberGaussianDistribution::EmberGaussianDistribution(Component* owner, Params& params) :
	EmberComputeDistribution(owner, params) {

	const double mean = (double) params.find_floating("mean", 1.0);
	const double stddev = (double) params.find_floating("stddev", 0.25);

	distrib = new SSTGaussianDistribution(mean, stddev);
}

EmberGaussianDistribution::~EmberGaussianDistribution() {
	delete distrib;
}

double EmberGaussianDistribution::sample(const uint64_t now) {
	return distrib->getNextDouble();
}
