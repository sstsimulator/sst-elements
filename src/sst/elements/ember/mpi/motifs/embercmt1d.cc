// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <math.h>

#include <sst_config.h>
#include <sst/core/rng/marsaglia.h>

#include "embercmt1d.h"

/*	CMT communication motif for a linear mapping of elements.
	Each process communication with two if its neighbor. There 
	is no wraparound so the first and last rank communicate with 
	only one neighbor.
	
	todo: replace with isend and irecv
*/

using namespace SST::Ember;

EmberCMT1DGenerator::EmberCMT1DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "CMT1D"), 
	m_loopIndex(0)
{
    	iterations = (uint32_t) params.find("arg.iterations", 1);
    	eltSize = (uint32_t) params.find("arg.elementsize", 10); //polynomial degree = eltSize-1
    	variables = (uint32_t) params.find("arg.variables", 1); //no. of physical quantities

        // No. of ranks per process
    	threads = (uint32_t) params.find("arg.threads", 1);
     
    	// Local distribution of elements on each MPI rank
    	nelt = (uint32_t) params.find("arg.nelt", 1000);

    	// Calculate computation time in nanoseconds
    	procFlops = (uint64_t) params.find("arg.processorflops", 4); // Nehalem 4 DP FLOPS/cycle
    	procFreq = (uint64_t) params.find("arg.processorfreq", 2);
    	const int64_t flopCount	= pow(eltSize, 3) * (nelt) * (2*eltSize-1); // Not assuming FMAs
    	const double time = (double)flopCount / ( (double)procFlops * (double)procFreq ); //Time in ns
    	
    	m_mean = params.find("arg.nsComputeMean", time);
        m_stddev = params.find("arg.nsComputeStddev", (m_mean*0.05));

    	xferSize = eltSize*eltSize;
    	
    	configure();
}


void EmberCMT1DGenerator::configure()
{
        myID = rank();

        // Initialize Marsaglia RNG for compute time
        m_random = new SSTGaussianDistribution( m_mean, m_stddev, 
                                    new RNG::MarsagliaRNG( 7+myID, getJobId() ) );

        
    	if(rank() == 0) {
            output("CMT1D (Pairwise Exchange) Motif \n"
                "element_size = %" PRIu32 ", elements_per_proc = %" PRIu32 ", total processes = %" PRIu32 \
    			"\ncompute time: mean = %fns ,stddev = %fns \nxfersize: %" PRIu64 "\n", 
    			eltSize, nelt, size(), m_mean, m_stddev, xferSize);
    	}		
}


bool EmberCMT1DGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
 
        if ( 0 == m_loopIndex ) {
    		verbose(CALL_INFO, 2,0, "rank=%d, size=%d\n", rank(), size());
        }

        double nsCompute = m_random->getNextDouble();
    	enQ_compute( evQ, nsCompute );  	// Delay block for compute

    	if(rank() == 0) {
    		enQ_recv( evQ, 1, xferSize, 0, GroupWorld);
    		enQ_send( evQ, 1, xferSize, 0, GroupWorld);

    	} 
    	else if( (size() - 1) == (signed)rank() ) {
    		enQ_send( evQ, rank() - 1, xferSize, 0, GroupWorld);
    		enQ_recv( evQ, rank() - 1, xferSize, 0, GroupWorld);

    	} 
    	else {
    		enQ_send( evQ, rank() - 1, xferSize, 0, GroupWorld);
    		enQ_recv( evQ, rank() + 1, xferSize, 0, GroupWorld);
    		enQ_send( evQ, rank() + 1, xferSize, 0, GroupWorld);
    		enQ_recv( evQ, rank() - 1, xferSize, 0, GroupWorld);
    	}

        //enQ_barrier( evQ, GroupWorld);

    	if ( ++m_loopIndex == iterations) {
            if ( ++m_phyIndex == variables) {
                return true;
            } else {
                m_loopIndex = 0;
                return false;
            }
        } else {
            return false;
        }



}




