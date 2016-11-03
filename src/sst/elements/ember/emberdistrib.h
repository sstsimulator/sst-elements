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
