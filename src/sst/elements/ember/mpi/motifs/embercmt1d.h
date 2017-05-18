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


#ifndef _H_EMBER_CMT_1D
#define _H_EMBER_CMT_1D

#include <sst/core/rng/gaussian.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCMT1DGenerator : public EmberMessagePassingGenerator {

public:
	EmberCMT1DGenerator(SST::Component* owner, Params& params);
//	~EmberCMT1DGenerator();
    void configure();
	bool generate( std::queue<EmberEvent*>& evQ);

private:

// User parameters - application
	uint32_t iterations;	// total no. of timesteps required
	uint32_t eltSize;		// size of 3d element or stencil (5 - 20)
    uint32_t variables;     // No. of physical quantities	

// User parameters - machine
	uint32_t threads;			
	
// User parameters - mpi rank	
	uint32_t nelt;			// no. of elements per process (100 - 10000)

// User parameters - processor
	uint64_t procFlops;		// no. of FLOPS/cycle for the processor
	uint64_t procFreq;		// operating frequency of the processor							
	double m_mean;
	double m_stddev;

// Model parameters
	uint32_t m_loopIndex;   // Loop over 'iterations'
    uint32_t m_phyIndex;    // Loop over 'variables'
	SSTGaussianDistribution* m_random;
    uint64_t myID;
	uint64_t xferSize;		// Amount of data transferred 

};

}
}

#endif
