// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SWEEP_3D
#define _H_EMBER_SWEEP_3D

#include "embergen.h"
#include "embermpigen.h"

namespace SST {
namespace Ember {

class EmberSweep3DGenerator : public EmberMessagePassingGenerator {

public:
	EmberSweep3DGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
        void finish(const SST::Output* output) { }

private:
	int32_t px;
	int32_t py;
	uint32_t rank;
	uint32_t size;
	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t kba;

	int32_t  x_up;
	int32_t  x_down;
	int32_t  y_up;
	int32_t  y_down;

	uint32_t nsCompute;
	uint32_t iterations;

};

}
}

#endif
