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


#ifndef _H_EMBER_NULL
#define _H_EMBER_NULL

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberNullGenerator : public EmberMessagePassingGenerator {

public:
	EmberNullGenerator(SST::Component* owner, Params& params) :
		EmberMessagePassingGenerator(owner, params) {
		m_name = "Null";
	}

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
		return true;
	}
};

}
}

#endif
