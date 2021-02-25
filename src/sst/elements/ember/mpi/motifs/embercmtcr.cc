// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

#include "embercmtcr.h"

/*
	CMT communication motif for Crystal Router algorithm using a 3d decom-
	position of elements over any network topology.

	todo: replace with isend and irecv
*/

using namespace SST::Ember;

EmberCMTCRGenerator::EmberCMTCRGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "CMTCR"),
	m_loopIndex(0)
{
	iterations = (uint32_t) params.find("arg.iterations", 1);
	eltSize = (uint32_t) params.find("arg.elementsize", 5);  //polynomial degree = eltSize-1
	variables = (uint32_t) params.find("arg.variables", 1); //no. of physical quantities

	//Distribution of processors in the machine
	px  = (uint32_t) params.find("arg.px", 8);
	py  = (uint32_t) params.find("arg.py", 8);
	pz  = (uint32_t) params.find("arg.pz", 8);
	threads = (uint32_t) params.find("arg.threads", 1);

	// Local distribution of elements on each MPI rank
	mx = (uint32_t) params.find("arg.mx", 10);
	my = (uint32_t) params.find("arg.my", 10);
	mz = (uint32_t) params.find("arg.mz", 10);
	nelt  = mx * my * mz; //Default : 1000

	// Calculate computation time in nanoseconds
	procFlops = (uint64_t) params.find("arg.processorflops", 4); //4 DP FLOPS/cycle
	procFreq = (uint64_t) params.find("arg.processorfreq", 2);
	const int64_t flopCount		= pow(eltSize, 3) * (nelt) * (2*eltSize-1); // Not assuming FMAs
	const double time = (double)flopCount / ( (double)procFlops * (double)procFreq ); //Time in ns

    m_mean = params.find("arg.nsComputeMean", time);
    m_stddev = params.find("arg.nsComputeStddev", (m_mean*0.05));

	xferSize = eltSize*eltSize*nelt;

	configure();
}



void EmberCMTCRGenerator::configure()
{

	if( (px * py *pz) != (signed)size() ) {
		fatal(CALL_INFO, -1, "Error: CMTCR motif checked processor decomposition: %" \
			PRIu32 "x%" PRIu32 "x%" PRIu32 " != MPI World %" PRIu32 "\n",
			px, py, pz, size());
	}

	// Get our (x,y) position in a 3D decomposition
	myX = -1; myY = -1; myZ = -1;
	myID = rank();
	getPosition(myID, px, py, pz, &myX, &myY, &myZ);

    stages = (uint32_t) log2( size() );

    m_random = new SSTGaussianDistribution( m_mean, m_stddev,
                                new RNG::MarsagliaRNG( 7+myID, getJobId() ) );

	if(rank() == 0) {
		output( "CMTCR (Crystal Router Reduction) Motif \n" \
		    "element_size = %" PRIu32 ", elements_per_proc = %" PRIu32 ", total processes = %" PRIu32 \
			"\ncompute time: mean = %fns ,stddev = %fns \
 			\npx: %" PRIu32 " ,py: %" PRIu32 " ,pz: %" PRIu32 "\n",
			eltSize, nelt, size(), m_mean, m_stddev,
			px, py, pz );
	}

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu64 " is located at coordinates \
		(%" PRId32 ", %" PRId32 ", %" PRId32") in the 3D grid, \n",
		myID, myX, myY, myZ );

}



bool EmberCMTCRGenerator::generate( std::queue<EmberEvent*>& evQ)
{
        if (m_loopIndex == 0) {
            verbose(CALL_INFO, 2, 0, "rank=%" PRIu64 ", size=%d\n", myID, size());
        }

        double nsCompute = m_random->getNextDouble();
    	enQ_compute( evQ, nsCompute );  	// Delay block for compute

        uint32_t i = 0;
        uint64_t MASK = 0;
        uint64_t myDest = 0;

        for (i=1; i<=stages; i++) {
            MASK = (uint64_t) exp2( stages - i );
            myDest = myID ^ MASK;
        //verbose(CALL_INFO, 2, 0, "stage: %" PRIu32 " \trank: %" PRIu64 " \trecv/send from %" PRIu64 "\n",
        //		    myID, i, myDest);

            if (myID < myDest) {
                enQ_send( evQ, myDest, xferSize, 0, GroupWorld);
                enQ_recv( evQ, myDest, xferSize, 0, GroupWorld);
            } else {
                enQ_recv( evQ, myDest, xferSize, 0, GroupWorld);
                enQ_send( evQ, myDest, xferSize, 0, GroupWorld);
            }
        }

        //enQ_barrier( evQ, GroupWorld);

        //verbose(CALL_INFO, 2, 0, "Completed CR simulation\n");
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












