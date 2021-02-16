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


#ifndef _H_SST_EMBER_CONSTANT_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_CONSTANT_COMPUTE_DISTRIBUTION

#include "emberdistrib.h"

namespace SST {
namespace Ember {

class EmberConstDistribution : public EmberComputeDistribution {
public:
   SST_ELI_REGISTER_MODULE(
        EmberConstDistribution,
        "ember",
        "ConstDistrib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Constant compute distribution model",
        "SST::Ember::EmberComputeDistribution"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "constant",     "Sets the constant value to return in the distribution.", "1.0" },
    )

public:
	EmberConstDistribution(Params& params);
	~EmberConstDistribution();
	double sample(uint64_t now);

private:
	double the_value;

};

}
}

#endif
