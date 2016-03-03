// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_RANDOM_GEN
#define _H_EMBER_RANDOM_GEN

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberRandomTrafficGenerator : public EmberMessagePassingGenerator {

public:
	EmberRandomTrafficGenerator(SST::Component* owner, Params& params);
    	bool generate( std::queue<EmberEvent*>& evQ);

protected:
	uint32_t maxIterations;
	uint32_t iteration;
	uint32_t msgSize;

};

}
}

#endif
