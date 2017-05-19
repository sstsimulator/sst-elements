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


#ifndef _H_EMBER_3D_COMM_DOUBLING
#define _H_EMBER_3D_COMM_DOUBLING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class Ember3DCommDoublingGenerator : public EmberMessagePassingGenerator {

public:
	Ember3DCommDoublingGenerator(SST::Component* owner, Params& params);
	~Ember3DCommDoublingGenerator() {}
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ );
	int32_t power3(const uint32_t expon);

private:
	uint32_t phase;
	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t computeBetweenSteps;
	uint32_t items_per_node;
//	uint32_t iterations;
	MessageRequest* requests;
	uint32_t next_request;

	uint32_t basePhase;
	uint32_t itemsThisPhase;

};

}
}

#endif
