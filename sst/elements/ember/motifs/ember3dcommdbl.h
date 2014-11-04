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


#ifndef _H_EMBER_3D_COMM_DOUBLING
#define _H_EMBER_3D_COMM_DOUBLING

#include "embermpigen.h"

namespace SST {
namespace Ember {

class Ember3DCommDoublingGenerator : public EmberMessagePassingGenerator {

public:
	Ember3DCommDoublingGenerator(SST::Component* owner, Params& params);
	~Ember3DCommDoublingGenerator();
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
        void finish(const SST::Output* output);
	int32_t power3(const uint32_t expon);

private:
	uint32_t rank;

	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t computeBetweenSteps;
	uint32_t items_per_node;
	uint32_t iterations;
	MessageRequest* requests;
	uint32_t next_request;

	uint32_t basePhase;
	uint32_t itemsThisPhase;

};

}
}

#endif
