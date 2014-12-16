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

#include <stdint.h>
#include <sst/core/math/sqrt.h>

#include "embersweep2d.h"

using namespace SST::Ember;

EmberSweep2DGenerator::EmberSweep2DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params),
	m_loopIndex(0) 
{
	m_name = "Sweep2D";

	nsCompute = (uint64_t) params.find_integer("arg.computetime", 1000);
	iterations = (uint32_t) params.find_integer("arg.iterations", 1);

	nx  = (uint32_t) params.find_integer("arg.nx", 50);
	ny  = (uint32_t) params.find_integer("arg.ny", 50);
	y_block = (uint32_t) params.find_integer("arg.y_block", 1);

	// Check K-blocking factor is acceptable for dividing the Nz dimension
	assert(ny % y_block == 0);
}

void EmberSweep2DGenerator::configure()
{
	if(0 == rank()) {
		m_output->verbose(CALL_INFO, 1, 0, " 2D Sweep Motif\n");
		m_output->verbose(CALL_INFO, 1, 0, " nx = %" PRIu32 ", ny = %" 
			PRIu32 ", y_block=%" PRIu32 ", (ny/yblock)=%" PRIu32 "\n",
			nx, ny, y_block, (ny/y_block));
	}

	x_up   = (rank() < (size() - 1)) ? rank() + 1 : -1;
	x_down = (rank() > 0) ? rank() - 1 : -1;

	m_output->verbose(CALL_INFO, 1, 0, "Rank: %" PRId32 ", X+:%" PRId32 
					",X-:%" PRId32 "\n", rank(), x_up, x_down);
}

bool EmberSweep2DGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    if( 0 == m_loopIndex) {
        GEN_DBG( 1, "rank=%d size=%d\n", rank(),size());
	}
	// Sweep from (0, 0) outwards towards (Px, Py)
	for(uint32_t i = 0; i < ny; i+= y_block) {
		if(x_down >= 0) {
			enQ_recv( evQ, x_down, (nx * y_block), 1000, GroupWorld );
		}

		enQ_compute( evQ, nsCompute);

		if(x_up >= 0) {
			enQ_send( evQ, x_up, (nx * y_block), 1000, GroupWorld );
		}
	}

	// Sweep from (Px, 0) outwards towards (0, Py)
	for(uint32_t i = 0; i < ny; i+= y_block) {
		if(x_up >= 0) {
			enQ_recv( evQ, x_up, (nx * y_block), 2000, GroupWorld );
		}


		enQ_compute( evQ, nsCompute );

		if(x_down >= 0) {
			enQ_send( evQ, x_down, (nx * y_block), 2000, GroupWorld );
		}
	}

	// Sweep from (Px,Py) outwards towards (0,0)
	for(uint32_t i = 0; i < ny; i+= y_block) {
		if(x_up >= 0) {
			enQ_recv( evQ, x_up, (nx * y_block), 3000, GroupWorld );
		}

		enQ_compute( evQ, nsCompute );

		if(x_down >= 0) {
       		enQ_send( evQ, x_down, (nx * y_block), 3000, GroupWorld );
		}
	}

	// Sweep from (0, Py) outwards towards (Px, 0)
	for(uint32_t i = 0; i < ny; i+= y_block) {
		if(x_down >= 0) {
			enQ_recv( evQ, x_down, (nx * y_block), 4000, GroupWorld );
		}

		enQ_compute( evQ, nsCompute );

		if(x_up >= 0) {
			enQ_send( evQ, x_up, (nx * y_block), 4000, GroupWorld );
		}
	}

    if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }
}
