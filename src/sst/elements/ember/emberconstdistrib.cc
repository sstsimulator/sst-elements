// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include "emberconstdistrib.h"

using namespace SST::Ember;

EmberConstDistribution::EmberConstDistribution(Component* owner, Params& params) :
	EmberComputeDistribution(owner, params) {

	the_value = params.find("constant", 1.0);
}

EmberConstDistribution::~EmberConstDistribution() {

}

double EmberConstDistribution::sample(const uint64_t now) {
	return the_value;
}
