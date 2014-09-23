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


#ifndef _H_EMBER_GENERATOR
#define _H_EMBER_GENERATOR

#include <sst/core/output.h>
#include <sst/core/module.h>
#include <sst/core/params.h>

#include <queue>

#include "emberevent.h"
#include "embersendev.h"
#include "emberrecvev.h"
#include "emberinitev.h"
#include "embercomputeev.h"
#include "emberfinalizeev.h"
#include "embermap.h"

namespace SST {
namespace Ember {

class EmberGenerator : public Module {

public:
	EmberGenerator( Component* owner, Params& params );
	virtual void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize) = 0;
	virtual void generate(const SST::Output* output, const uint32_t phase,
		std::queue<EmberEvent*>* evQ) = 0;
	virtual void finish(const SST::Output* output) = 0;
	virtual bool autoInitialize() { return false; }
	virtual void setRankMap(EmberRankMap* mapper);

	void getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ);
        void getPosition(const int32_t rank, const int32_t px, const int32_t py,
                int32_t* myX, int32_t* myY);
	int32_t convertPositionToRank(const int32_t peX, const int32_t peY, const int32_t peZ,
        	const int32_t posX, const int32_t posY, const int32_t posZ);

	~EmberGenerator();

private:
	EmberRankMap* rankMap;

};

}
}

#endif
