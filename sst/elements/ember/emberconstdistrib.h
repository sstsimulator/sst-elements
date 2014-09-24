
#ifndef _H_SST_EMBER_CONSTANT_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_CONSTANT_COMPUTE_DISTRIBUTION

#include "emberdistrib.h"

namespace SST {
namespace Ember {

class EmberConstDistribution : public EmberComputeDistribution {

public:
	EmberConstDistribution(Component* owner, Params& params);
	~EmberConstDistribution();
	double sample(uint64_t now);

private:
	double the_value;

};

}
}

#endif
