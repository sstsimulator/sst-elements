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

#ifndef _H_EMBER_MPI_GENERATOR
#define _H_EMBER_MPI_GENERATOR

#include "embergen.h"

namespace SST {
namespace Ember {

class EmberMessagePassingGenerator : public EmberGenerator {

public:
	EmberMessagePassingGenerator( Component* owner, Params& params);
	bool autoInitialize() { return true; }

	virtual void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize) = 0;
        virtual void generate(const SST::Output* output, const uint32_t phase,
                std::queue<EmberEvent*>* evQ) = 0;
        virtual void finish(const SST::Output* output) = 0;

protected:
	~EmberMessagePassingGenerator();
};

}
}

#endif
