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


#ifndef _H_EMBER_HALO_3D_SINGLE_VAR_BLOCKING
#define _H_EMBER_HALO_3D_SINGLE_VAR_BLOCKING

#include "embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo3DSVGenerator : public EmberMessagePassingGenerator {

public:
	EmberHalo3DSVGenerator(SST::Component* owner, Params& params);
	~EmberHalo3DSVGenerator();
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
        void finish(const SST::Output* output);

private:
	uint32_t performReduction;

	uint32_t iterations;
	uint32_t rank;

	// Share these over all instances of the motif
	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t nsCompute;
	uint32_t nsCopyTime;

	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t items_per_cell;
	uint32_t sizeof_cell;

	int32_t  x_down;
	int32_t  x_up;
	int32_t  y_down;
	int32_t  y_up;
	int32_t  z_down;
	int32_t  z_up;

};

}
}

#endif
