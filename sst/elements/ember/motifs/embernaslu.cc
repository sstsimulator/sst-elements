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


#include <sst_config.h>

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/math/sqrt.h>

#include "embernaslu.h"

using namespace SST::Ember;

EmberNASLUGenerator::EmberNASLUGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nsCompute = (uint64_t) params.find_integer("computetime", 1000);

	px = (uint32_t) params.find_integer("pex", 0);
	py = (uint32_t) params.find_integer("pey", 0);

	iterations = (uint32_t) params.find_integer("iterations", 1);

	nx  = (uint32_t) params.find_integer("nx", 50);
	ny  = (uint32_t) params.find_integer("ny", 50);
	nz  = (uint32_t) params.find_integer("nz", 50);
	nzblock = (uint32_t) params.find_integer("nzblock", 1);

	// Check K-blocking factor is acceptable for dividing the Nz dimension
	assert(nz % nzblock == 0);
}

void EmberNASLUGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
	size = worldSize;

	// Check that we are using all the processors or else lock up will happen :(.
	if( (px * py) != worldSize ) {
		output->fatal(CALL_INFO, -1, "Error: NAS-LU motif checked processor decomposition: %" PRIu32 "x%" PRIu32 " != MPI World %" PRIu32 "\n",
			px, py, worldSize);
	}

	int32_t myX = 0;
	int32_t myY = 0;

	// Get our position in a 2D processor array
	getPosition(rank, px, py, &myX, &myY);

	x_up   = (myX != (px - 1)) ? rank + 1 : -1;
	x_down = (myX != 0) ? rank - 1 : -1;

	y_up   = (myY != (py - 1)) ? rank + px : -1;
	y_down = (myY != 0) ? rank - px : -1;

	if(0 == pRank) {
		output->verbose(CALL_INFO, 1, 0, " NAS-LU Motif\n");
		output->verbose(CALL_INFO, 1, 0, " nx = %" PRIu32 ", ny = %" PRIu32 ", nz = %" PRIu32 ", nzblock=%" PRIu32 ", (nx/nzblock)=%" PRIu32 "\n",
			nx, ny, nz, nzblock, (nz / nzblock));
	}

	output->verbose(CALL_INFO, 1, 0, " Rank: %" PRIu32 " is located at coordinations of (%" PRId32 ", %" PRId32 ") in the 2D decomposition, X+: %" PRId32 ",X-:%" PRId32 ",Y+:%" PRId32 ",Y-:%" PRId32 "\n",
		rank, myX, myY, x_up, x_down, y_up, y_down);
}

void EmberNASLUGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		// Sweep from (0, 0) outwards towards (Px, Py)
		for(uint32_t i = 0; i < nz; i+= nzblock) {
			if(x_down >= 0) {
				evQ->push( new EmberRecvEvent(x_down, (nx * nzblock), 1000, (Communicator) 0) );
			}

			if(y_down >= 0) {
				evQ->push( new EmberRecvEvent(y_down, (ny * nzblock), 1000, (Communicator) 0) );
			}

			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_up >= 0) {
      	                         evQ->push( new EmberSendEvent(x_up, (nx * nzblock), 1000, (Communicator) 0) );
       		         }

                        if(y_up >= 0) {
                        	evQ->push( new EmberSendEvent(y_up, (ny * nzblock), 1000, (Communicator) 0) );
               		}
		}

		// Sweep from (Px,Py) outwards towards (0,0)
		for(uint32_t i = 0; i < nz; i+= nzblock) {
			if(x_up >= 0) {
				evQ->push( new EmberRecvEvent(x_up, (nx * nzblock), 3000, (Communicator) 0) );
			}

			if(y_up >= 0) {
				evQ->push( new EmberRecvEvent(y_up, (ny * nzblock), 3000, (Communicator) 0) );
			}

			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_down >= 0) {
 	                       evQ->push( new EmberSendEvent(x_down, (nx * nzblock), 3000, (Communicator) 0) );
	                }

	                if(y_down >= 0) {
	     	           evQ->push( new EmberSendEvent(y_down, (ny * nzblock), 3000, (Communicator) 0) );
	                }
		}
	} else {
		// We are done, tell Ember to finalize the MPI and stop the processing on this motif
                EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
                evQ->push(finalize);
	}
}
