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

#include "embercmt3d.h"

/*
	CMT communication motif for a 3d decomposition of elements over any
	network topology. Each process communicates with its +-x, +-y & +-z
	neighbors. Note: The motif works only when all the processors in the
	grid are used.

	todo: replace with isend and irecv
*/

using namespace SST::Ember;

EmberCMT3DGenerator::EmberCMT3DGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "CMT3D"),
        m_loopIndex(0),
        x_pos(-1), x_neg(-1),
        y_pos(-1), y_neg(-1),
        z_pos(-1), z_neg(-1),
        sendx_pos(false), sendx_neg(false),
        sendy_pos(false), sendy_neg(false),
        sendz_pos(false), sendz_neg(false)
{
    	iterations = (uint32_t) params.find("arg.iterations", 1);
    	eltSize = (uint32_t) params.find("arg.elementsize", 5); //polynomial degree = eltSize-1
    	variables = (uint32_t) params.find("arg.variables", 1); //no. of physical quantities

    	//Distribution of MPI ranks
    	px  = (uint32_t) params.find("arg.px", 8);
    	py  = (uint32_t) params.find("arg.py", 8);
    	pz  = (uint32_t) params.find("arg.pz", 8);
    	threads = (uint32_t) params.find("arg.threads", 1);

    	//Local distribution of elements on each MPI rank
    	mx = (uint32_t) params.find("arg.mx", 10);
    	my = (uint32_t) params.find("arg.my", 10);
    	mz = (uint32_t) params.find("arg.mz", 10);
    	nelt  = mx * my * mz; //Default : 1000

    	// Calculate computation time in nanoseconds
    	procFlops = (uint64_t) params.find("arg.processorflops", 4); // Nehalem 4 DP FLOPS/cycle
    	procFreq = (uint64_t) params.find("arg.processorfreq", 2);
    	const int64_t flopCount	= pow(eltSize, 3) * (nelt) * (2*eltSize-1); // Not assuming FMAs
    	const double time = (double)flopCount / ( (double)procFlops * (double)procFreq ); //Time in ns

        m_mean = params.find("arg.nsComputeMean", time);
        m_stddev = params.find("arg.nsComputeStddev", (m_mean*0.05));

    	x_xferSize = eltSize*eltSize*my*mz;
    	y_xferSize = eltSize*eltSize*mx*mz;
    	z_xferSize = eltSize*eltSize*mx*my;

        configure();
}


void EmberCMT3DGenerator::configure()
{

    	// Check that we are using all the processors or else lock up will happen :(.
//    	if( (px * py *pz *threads) != (signed)size() ) {
    	if( (px * py *pz *threads) != (signed)size() ) {
    		fatal(CALL_INFO, -1, "Error: CMT3D motif checked processor decomposition: %" \
    			PRId32 "x%" PRId32 "x%" PRId32 "x%" PRIu32 " != MPI World %" PRIu32 "\n",
    			px, py, pz, threads, size());
    	}

    	if(rank() == 0) {
    		output( "CMT3D (Pairwise Exchange) Motif \n" \
    		    "nx1 (elt_size) = %" PRIu32 ", nelt (elts/proc) = %" PRIu32 ", np (total processes) = %" PRIu32 \
    		    ", elements (total) = %" PRIu32 \
     			"\npx: %" PRIu32 " ,py: %" PRIu32 " ,pz: %" PRIu32 " ,threads/proc: %" PRIu32 \
     			"\nmx: %" PRIu32 " ,my: %" PRIu32 " ,mz: %" PRIu32 \
    			"\ncompute time: mean = %fns ,stddev = %fns \
    			\nx_xfer: %" PRIu64 " ,y_xfer: %" PRIu64 " ,z_xfer: %" PRIu64 "\n",
    			eltSize, nelt, size(), nelt*size(),
    			px, py, pz, threads,
    			mx, my, mz,
                m_mean, m_stddev,
    			x_xferSize, y_xferSize, z_xferSize );
    	}

    	// Get our (x,y,z) position and neighboring ranks in a 3D decomposition
    	myX=-1; myY=-1; myZ=-1;
    	myID = rank();
    	getPosition(myID, px, py, pz, &myX, &myY, &myZ);

        // Initialize Marsaglia RNG for compute time
        m_random = new SSTGaussianDistribution( m_mean, m_stddev,
                                    new RNG::MarsagliaRNG( 7+myID, getJobId() ) );

    	// Set which direction to transfer and the neighbors in that direction
    	if( myX > 0 ) {
    		sendx_neg = true;
    		x_neg	= convertPositionToRank(px, py, pz, myX-1, myY, myZ);
    	}

    	if( myX < px-1 ) {
    		sendx_pos = true;
    		x_pos	= convertPositionToRank(px, py, pz, myX+1, myY, myZ);
    	}

    	if( myY > 0 ) {
    		sendy_neg = true;
    		y_neg 	= convertPositionToRank(px, py, pz, myX, myY-1, myZ);
    	}

    	if( myY < py-1 ) {
    		sendy_pos = true;
    		y_pos	= convertPositionToRank(px, py, pz, myX, myY+1, myZ);
    	}

    	if( myZ > 0 ) {
    		sendz_neg = true;
    		z_neg 	= convertPositionToRank(px, py, pz, myX, myY, myZ-1);
    	}

    	if( myZ < pz-1 ) {
    		sendz_pos = true;
    		z_pos	= convertPositionToRank(px, py, pz, myX, myY, myZ+1);
    	}

    	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu64 " is located at coordinates \
    		(%" PRId32 ", %" PRId32 ", %" PRId32") in the 3D grid,\
    		X+:%s %" PRId32 ", X-:%s %" PRId32 ", \
    		Y+:%s %" PRId32 ", Y-:%s %" PRId32 ", \
    		Z+:%s %" PRId32 ", Z-:%s %" PRId32 "\n",
    		myID,
    		myX, myY, myZ,
    		(sendx_pos ? "Y" : "N"), x_pos,
    		(sendx_neg ? "Y" : "N"), x_neg,
    		(sendy_pos ? "Y" : "N"), y_pos,
    		(sendy_neg ? "Y" : "N"), y_neg,
    		(sendz_pos ? "Y" : "N"), z_pos,
    		(sendz_neg ? "Y" : "N"), z_neg);

}



bool EmberCMT3DGenerator::generate( std::queue<EmberEvent*>& evQ)
{
        if (m_loopIndex == 0) {
    		verbose(CALL_INFO, 2,0, "rank=%d, size=%d\n", rank(), size());
        }

        double nsCompute = m_random->getNextDouble();
    	enQ_compute( evQ, nsCompute );  	// Delay block for compute

    	// +x/-x transfers
    	if ( myX % 2 == 0){
    		if (sendx_pos) {
    			enQ_recv( evQ, x_pos, x_xferSize, 0, GroupWorld );
    			enQ_send( evQ, x_pos, x_xferSize, 0, GroupWorld );
    		}
    		if (sendx_neg) {
    			enQ_recv( evQ, x_neg, x_xferSize, 0, GroupWorld );
    			enQ_send( evQ, x_neg, x_xferSize, 0, GroupWorld );
    		}
    	}
    	else {
    		if (sendx_pos) {
    			enQ_send( evQ, x_pos, x_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, x_pos, x_xferSize, 0, GroupWorld );
    		}
    		if (sendx_neg) {
    			enQ_send( evQ, x_neg, x_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, x_neg, x_xferSize, 0, GroupWorld );
    		}
    	}

    	// +y/-y transfers
    	if ( myY % 2 == 0){
    		if (sendy_pos) {
    			enQ_recv( evQ, y_pos, y_xferSize, 0, GroupWorld );
    			enQ_send( evQ, y_pos, y_xferSize, 0, GroupWorld );
    		}
    		if (sendy_neg) {
    			enQ_recv( evQ, y_neg, y_xferSize, 0, GroupWorld );
    			enQ_send( evQ, y_neg, y_xferSize, 0, GroupWorld );
    		}
    	}
    	else {
    		if (sendy_pos) {
    			enQ_send( evQ, y_pos, y_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, y_pos, y_xferSize, 0, GroupWorld );
    		}
    		if (sendy_neg) {
    			enQ_send( evQ, y_neg, y_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, y_neg, y_xferSize, 0, GroupWorld );
    		}
    	}

    	// +z/-z transfers
    	if ( myZ % 2 == 0){
    		if (sendz_pos) {
    			enQ_recv( evQ, z_pos, z_xferSize, 0, GroupWorld );
    			enQ_send( evQ, z_pos, z_xferSize, 0, GroupWorld );
    		}
    		if (sendz_neg) {
    			enQ_recv( evQ, z_neg, z_xferSize, 0, GroupWorld );
    			enQ_send( evQ, z_neg, z_xferSize, 0, GroupWorld );
    		}
    	}
    	else {
    		if (sendz_pos) {
    			enQ_send( evQ, z_pos, z_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, z_pos, z_xferSize, 0, GroupWorld );
    		}
    		if (sendz_neg) {
    			enQ_send( evQ, z_neg, z_xferSize, 0, GroupWorld );
    			enQ_recv( evQ, z_neg, z_xferSize, 0, GroupWorld );
    		}
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












