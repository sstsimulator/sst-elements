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


#ifndef _H_EMBER_CMT_2D
#define _H_EMBER_CMT_2D

#include <sst/core/rng/gaussian.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCMT2DGenerator : public EmberMessagePassingGenerator {

public:
	EmberCMT2DGenerator(SST::Component* owner, Params& params);
//	~EmberCMT2DGenerator();
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ);

private:
// User parameters - application
	uint32_t iterations;	// total no. of timesteps required
	uint32_t eltSize;		// size of element or stencil in 3-dim (5-20)
    uint32_t variables;     // No. of physical quantities
    
// User parameters - machine
	int32_t px;				// Machine size (no. of nodes in mesh)
	int32_t py;			
	uint32_t threads;			

// User parameters - mpi rank
	uint32_t mx;			// local distribution of the elements on a proc
	uint32_t my;			
	uint32_t mz;			
	uint32_t nelt;			// Total no. of elements per process (100-10,000)

// User parameters - processor
	uint64_t procFlops;		// no. of FLOPS/cycle for the processor
	uint64_t procFreq;		// operating frequency of the processor							
	double m_mean;
	double m_stddev;

// Model parameters
	uint32_t m_loopIndex;   // Loop over 'iterations'
    uint32_t m_phyIndex;    // Loop over 'variables'
	SSTGaussianDistribution* m_random;
	int32_t myX;			// Local (x,y,z) coordinates
	int32_t myY;
    uint64_t myID;
	uint64_t x_xferSize;	// Amount of data transferred in x,y direction
	uint64_t y_xferSize;
	int32_t x_pos;			// x & y neighbors
	int32_t x_neg;
	int32_t y_pos;
	int32_t y_neg;
	bool sendx_pos;			// Set to true if a node transfers data in +/-x, +/-y
	bool sendx_neg;
	bool sendy_pos;
	bool sendy_neg;


};

}
}

#endif
