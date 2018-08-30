// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION

#include "emberdistrib.h"
#include <sst/core/elementinfo.h>
#include <sst/core/rng/gaussian.h>

namespace SST {
namespace Ember {

class EmberGaussianDistribution : public EmberComputeDistribution {
public:
    SST_ELI_REGISTER_MODULE(
        EmberGaussianDistribution, 
        "ember",
        "GaussianDistrib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Gaussian distributed compute noise model",
        "SST::Ember::EmberComputeDistribution"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "mean",         "Sets the mean value of the Gaussian distribution", "1.0" },
        {   "stddev",       "Sets the standard deviation of the Gaussian distribution", "0.25" },
    )    

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
