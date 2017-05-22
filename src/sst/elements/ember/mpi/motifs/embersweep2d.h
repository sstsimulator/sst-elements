// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SWEEP_2D
#define _H_EMBER_SWEEP_2D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberSweep2DGenerator : public EmberMessagePassingGenerator {

public:
	EmberSweep2DGenerator(SST::Component* owner, Params& params);
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ );

private:
	uint32_t m_loopIndex;

	int32_t  x_up;
	int32_t  x_down;

	uint32_t nx;
	uint32_t ny;
	uint32_t y_block;

	uint32_t nsCompute;
	uint32_t iterations;
};

}
}

#endif
