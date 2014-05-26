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

#include "embersweep2d.h"

using namespace SST::Ember;

EmberSweep2DGenerator::EmberSweep2DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nsCompute = (uint64_t) params.find_integer("computetime", 1000);
	iterations = (uint32_t) params.find_integer("iterations", 1);

	nx  = (uint32_t) params.find_integer("nx", 50);
	ny  = (uint32_t) params.find_integer("ny", 50);
	y_block = (uint32_t) params.find_integer("y_block", 1);

	// Check K-blocking factor is acceptable for dividing the Nz dimension
	assert(ny % y_block == 0);
}

void EmberSweep2DGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = (int32_t) pRank;
	size = (int32_t) worldSize;

	if(0 == pRank) {
		output->verbose(CALL_INFO, 1, 0, " 2D Sweep Motif\n");
		output->verbose(CALL_INFO, 1, 0, " nx = %" PRIu32 ", ny = %" PRIu32 ", y_block=%" PRIu32 ", (ny/yblock)=%" PRIu32 "\n",
			nx, ny, y_block, (ny/y_block));
	}

	x_up   = (rank < (size - 1)) ? rank + 1 : -1;
	x_down = (rank > 0) ? rank - 1 : -1;

	output->verbose(CALL_INFO, 1, 0, "Rank: %" PRId32 ", X+:%" PRId32 ",X-:%" PRId32 "\n", rank, x_up, x_down);
}

void EmberSweep2DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		// Sweep from (0, 0) outwards towards (Px, Py)
		for(uint32_t i = 0; i < ny; i+= y_block) {
			if(x_down >= 0) {
				evQ->push( new EmberRecvEvent(x_down, (nx * y_block), 1000, (Communicator) 0) );
			}

			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_up >= 0) {
	                        evQ->push( new EmberSendEvent(x_up, (nx * y_block), 1000, (Communicator) 0) );
    		        }
		}

		// Sweep from (Px, 0) outwards towards (0, Py)
		for(uint32_t i = 0; i < ny; i+= y_block) {
			if(x_up >= 0) {
				evQ->push( new EmberRecvEvent(x_up, (nx * y_block), 2000, (Communicator) 0) );
			}


			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_down >= 0) {
      	                         evQ->push( new EmberSendEvent(x_down, (nx * y_block), 2000, (Communicator) 0) );
                        }
		}

		// Sweep from (Px,Py) outwards towards (0,0)
		for(uint32_t i = 0; i < ny; i+= y_block) {
			if(x_up >= 0) {
				evQ->push( new EmberRecvEvent(x_up, (nx * y_block), 3000, (Communicator) 0) );
			}

			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_down >= 0) {
                               evQ->push( new EmberSendEvent(x_down, (nx * y_block), 3000, (Communicator) 0) );
                        }
		}

		// Sweep from (0, Py) outwards towards (Px, 0)
		for(uint32_t i = 0; i < ny; i+= y_block) {
			if(x_down >= 0) {
				evQ->push( new EmberRecvEvent(x_down, (nx * y_block), 4000, (Communicator) 0) );
			}

			evQ->push(new EmberComputeEvent(nsCompute));

			if(x_up >= 0) {
       	                        evQ->push( new EmberSendEvent(x_up, (nx * y_block), 4000, (Communicator) 0) );
                        }
		}
	} else {
		// We are done, tell Ember to finalize the MPI and stop the processing on this motif
                EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
                evQ->push(finalize);
	}
}
