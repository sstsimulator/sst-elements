// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "emberdistrib.h"
#include "embergaussdistrib.h"

using namespace SST::Ember;
using namespace SST::RNG;

EmberGaussianDistribution::EmberGaussianDistribution(Params& params) :
	EmberComputeDistribution(params) {

	const double mean = (double) params.find("mean", 1.0);
	const double stddev = (double) params.find("stddev", 0.25);

	distrib = new SSTGaussianDistribution(mean, stddev);
}

EmberGaussianDistribution::~EmberGaussianDistribution() {
	delete distrib;
}

double EmberGaussianDistribution::sample(const uint64_t now) {
	return distrib->getNextDouble();
}
