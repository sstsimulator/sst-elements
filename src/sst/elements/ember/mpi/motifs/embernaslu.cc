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


#include <sst_config.h>

#include <stdint.h>
#include <sst/core/math/sqrt.h>

#include "embernaslu.h"

using namespace SST::Ember;

EmberNASLUGenerator::EmberNASLUGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "NASLU"),
	m_loopIndex(0)
{
	nsCompute = (uint64_t) params.find("arg.computetime", 1000);

	px = (int32_t) params.find("arg.pex", 0);
	py = (int32_t) params.find("arg.pey", 0);

	iterations = (uint32_t) params.find("arg.iterations", 1);

	nx  = (uint32_t) params.find("arg.nx", 50);
	ny  = (uint32_t) params.find("arg.ny", 50);
	nz  = (uint32_t) params.find("arg.nz", 50);
	nzblock = (uint32_t) params.find("arg.nzblock", 1);

	// Check K-blocking factor is acceptable for dividing the Nz dimension
	assert(nz % nzblock == 0);

	configure();
}

void EmberNASLUGenerator::configure()
{
	// Check that we are using all the processors or else lock up will happen :(.
	if( (px * py) != (signed)size() ) {
		fatal(CALL_INFO, -1, "Error: NAS-LU motif checked processor decomposition: %" PRIu32 "x%" PRIu32 " != MPI World %" PRIu32 "\n",
			px, py, size());
	}

	int32_t myX = 0;
	int32_t myY = 0;

	// Get our position in a 2D processor array
	getPosition(rank(), px, py, &myX, &myY);

	x_up   = (myX != (px - 1)) ? rank() + 1 : -1;
	x_down = (myX != 0) ? rank() - 1 : -1;

	y_up   = (myY != (py - 1)) ? rank() + px : -1;
	y_down = (myY != 0) ? rank() - px : -1;

	if(0 == rank()) {
		verbose(CALL_INFO, 1, 0, " NAS-LU Motif\n");
		verbose(CALL_INFO, 1, 0, " nx = %" PRIu32 ", ny = %" PRIu32 ", nz = %" PRIu32 ", nzblock=%" PRIu32 ", (nx/nzblock)=%" PRIu32 "\n",
			nx, ny, nz, nzblock, (nz / nzblock));
	}

	verbose(CALL_INFO, 1, 0, " Rank: %" PRIu32 " is located at coordinations of (%" PRId32 ", %" PRId32 ") in the 2D decomposition, X+: %" PRId32 ",X-:%" PRId32 ",Y+:%" PRId32 ",Y-:%" PRId32 "\n",
		rank(), myX, myY, x_up, x_down, y_up, y_down);
}

bool EmberNASLUGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    if( 0 == m_loopIndex) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(),size());
    }

	// Sweep from (0, 0) outwards towards (Px, Py)
	for(uint32_t i = 0; i < nz; i+= nzblock) {
		if(x_down >= 0) {
			enQ_recv( evQ, x_down, (nx * nzblock), 1000, GroupWorld );
		}

		if(y_down >= 0) {
			enQ_recv( evQ, y_down, (ny * nzblock), 1000, GroupWorld );
		}

		enQ_compute( evQ, nsCompute );

		if(x_up >= 0) {
			enQ_send( evQ, x_up, (nx * nzblock), 1000, GroupWorld );
		}

		if(y_up >= 0) {
			enQ_send( evQ, y_up, (ny * nzblock), 1000, GroupWorld );
   		}
	}

	// Sweep from (Px,Py) outwards towards (0,0)
	for(uint32_t i = 0; i < nz; i+= nzblock) {
		if(x_up >= 0) {
			enQ_recv( evQ, x_up, (nx * nzblock), 3000, GroupWorld );
		}

		if(y_up >= 0) {
			enQ_recv( evQ, y_up, (ny * nzblock), 3000, GroupWorld );
		}

		enQ_compute( evQ, nsCompute );

		if(x_down >= 0) {
			enQ_send( evQ, x_down, (nx * nzblock), 3000, GroupWorld );
		}

		if(y_down >= 0) {
			enQ_send( evQ, y_down, (ny * nzblock), 3000, GroupWorld );
		}
	}

	if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }
}
