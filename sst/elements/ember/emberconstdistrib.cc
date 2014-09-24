
#include <sst_config.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "emberdistrib.h"
#include "emberconstdistrib.h"

using namespace SST::Ember;

EmberConstDistribution::EmberConstDistribution(Component* owner, Params& params) :
	EmberComputeDistribution(owner, params) {

	the_value = params.find_floating("constant", 1.0);
}

EmberConstDistribution::~EmberConstDistribution() {

}

double EmberConstDistribution::sample(const uint64_t now) {
	return the_value;
}
