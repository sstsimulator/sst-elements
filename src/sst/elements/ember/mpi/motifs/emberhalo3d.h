// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_HALO_3D_BLOCKING
#define _H_EMBER_HALO_3D_BLOCKING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo3DGenerator : public EmberMessagePassingGenerator {

public:
	EmberHalo3DGenerator(SST::Component* owner, Params& params);
	~EmberHalo3DGenerator() {}
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ );

private:
	uint32_t m_loopIndex;

	bool performReduction;

	uint32_t iterations;

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
