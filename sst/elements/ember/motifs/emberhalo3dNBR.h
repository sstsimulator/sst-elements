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


#ifndef _H_EMBER_HALO_3DNBR
#define _H_EMBER_HALO_3DNBR

#include "embergen.h"
#include "embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo3DNBRGenerator : public EmberMessagePassingGenerator {

public:
	EmberHalo3DNBRGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
        void finish(const SST::Output* output);

private:
	uint32_t rank;

	uint32_t sizeX;
	uint32_t sizeY;
	uint32_t sizeZ;

	uint32_t nsCompute;
	uint32_t nsCopyTime;
	uint32_t messageSizeX;
	uint32_t messageSizeY;
	uint32_t iterations;
	uint32_t messageCount;

	int32_t  procXFaceUp;
	int32_t  procXFaceDown;
	int32_t  procYFaceUp;
	int32_t  procYFaceDown;
	int32_t  procZFaceUp;
	int32_t  procZFaceDown;

};

}
}

#endif
