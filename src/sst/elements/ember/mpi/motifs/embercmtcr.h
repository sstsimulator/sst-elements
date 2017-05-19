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


#ifndef _H_EMBER_CMT_CR
#define _H_EMBER_CMT_CR

#include <sst/core/rng/gaussian.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCMTCRGenerator : public EmberMessagePassingGenerator {

public:
	EmberCMTCRGenerator(SST::Component* owner, Params& params);
//	~EmberCMT3DGenerator();
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ);

private:
// User parameters - application
	uint32_t iterations;	// Total no. of timesteps being simulated
	uint32_t eltSize;		// Size of element (5-20)
	uint32_t variables;     // No. of physical quantities

// User parameters - machine
	int32_t px;				// Machine size (no. of nodes in 3d dimensions)
	int32_t py;			
	int32_t pz;			
    uint32_t threads;

// User parameters - mpi rank	
	uint32_t mx;			// Local distribution of the elements on a MPI rank
	uint32_t my;			
	uint32_t mz;			
	uint32_t nelt;			// Total no. of elements per process (100-10,000)
//	uint64_t nsCompute;		// computation time (or delay)

// User parameters - processor
	uint64_t procFlops;		// no. of FLOPS/cycle for the processor
	uint64_t procFreq;		// operating frequency of the processor							
	double m_mean;
	double m_stddev;

// Model parameters
	uint32_t m_loopIndex;   // Loop over 'iterations'
    uint32_t m_phyIndex;    // Loop over 'variables'
	int32_t myX;			// Local (x,y,z) coordinates and rank
	int32_t myY;
	int32_t myZ;
	uint64_t myID;          // my MPI rank
	uint64_t xferSize;      // Average transfer size
	uint32_t stages;        // No. of Stages in cyrstal router reduction
	SSTGaussianDistribution* m_random;

};

}
}

#endif
