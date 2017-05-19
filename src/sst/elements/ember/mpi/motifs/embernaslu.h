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


#ifndef _H_EMBER_NAS_LU
#define _H_EMBER_NAS_LU

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberNASLUGenerator : public EmberMessagePassingGenerator {

public:
	EmberNASLUGenerator(SST::Component* owner, Params& params);
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ );

private:
	uint32_t m_loopIndex;

	int32_t px;
	int32_t py;
	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t nzblock;

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
