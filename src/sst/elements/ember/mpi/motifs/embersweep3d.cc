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

#include "embersweep3d.h"

using namespace SST::Ember;

EmberSweep3DGenerator::EmberSweep3DGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Sweep3D"),
    m_loopIndex(0),
    m_InnerLoopIndex(0)
{
	px = (int32_t) params.find<int32_t>("arg.pex", 0);
	py = (int32_t) params.find<int32_t>("arg.pey", 0);

	iterations = (uint32_t) params.find<uint32_t>("arg.iterations", 1);

	nx  = (uint32_t) params.find<uint32_t>("arg.nx", 50);
	ny  = (uint32_t) params.find<uint32_t>("arg.ny", 50);
	nz  = (uint32_t) params.find<uint32_t>("arg.nz", 50);
	kba = (uint32_t) params.find<uint32_t>("arg.kba", 1);

	data_width = (uint32_t) params.find<uint32_t>("arg.datatype_width", 8);
	fields_per_cell = (uint32_t) params.find<uint32_t>("arg.fields_per_cell", 8);

	double flops_per_cell = params.find<double>("arg.flops_per_cell", 275.0);
	double node_flops = params.find<double>("arg.nodeflops", 1000000000);

	const double mesh_slice_size = (double) (nx * ny);
	double compute_time_d = mesh_slice_size * flops_per_cell;

	// Check K-blocking factor is acceptable for dividing the Nz dimension
	assert(nz % kba == 0);

	const double NANO_SECONDS = 1000000000;

	nsCompute = (uint64_t) params.find("arg.computetime",
		(compute_time_d / node_flops) * NANO_SECONDS);

	assert(nsCompute != 0);
}

void EmberSweep3DGenerator::configure()
{
	// Check that we are using all the processors or else lock up will happen :(.
	if( (px * py) != (signed) size() ) {
		fatal(CALL_INFO, -1, "Error: Sweep 3D motif checked "
			"processor decomposition: %" PRIu32 "x%" PRIu32 " != MPI World %"
			PRIu32 "\n", px, py, size());
	}

	int32_t myX = 0;
	int32_t myY = 0;

	// Get our position in a 2D processor array
	getPosition(rank(), px, py, &myX, &myY);

	x_up   = (myX != (px - 1)) ? rank() + 1 : -1;
	x_down = (myX != 0) ? rank() - 1 : -1;

	y_up   = (myY != (py - 1)) ? rank() + px : -1;
	y_down = (myY != 0) ? rank() - px : -1;

	/**if(0 == rank()) {
		output( "%s nx = %" PRIu32 ", ny = %" PRIu32 ", nz = %" PRIu32
			", kba=%" PRIu32 ", (nx/kba)=%" PRIu32 "\n",
			m_name.c_str(), nx, ny, nz, kba, (nz / kba));
	}


	output( "%s Rank: %" PRIu32 " is located at coordinations of (%" PRId32
			", %" PRId32 ") in the 2D decomposition, X+: %" PRId32 ",X-:%"
			PRId32 ",Y+:%" PRId32 ",Y-:%" PRId32 "\n",
			m_name.c_str(), rank(), myX, myY, x_up, x_down, y_up, y_down);
	*/
}

bool EmberSweep3DGenerator::generate( std::queue<EmberEvent*>& evQ) {

	if( 0 == m_loopIndex && 0 == m_InnerLoopIndex ) {
		configure();
		verbose(CALL_INFO, 2, MOTIF_MASK, "rank=%d size=%d\n", rank(), size());
	}

	//for(uint32_t repeat = 0; repeat < 2; ++repeat) {

	switch(m_InnerLoopIndex) {

	case 0:
		// Sweep from (0, 0) outwards towards (Px, Py)
		for(uint32_t i = 0; i < nz; i+= kba) {
			if(x_down >= 0) {
				enQ_recv(evQ, x_down, (nx * kba * data_width * fields_per_cell), 1000, GroupWorld );
			}

			if(y_down >= 0) {
				enQ_recv(evQ, y_down, (ny * kba * data_width * fields_per_cell), 1000, GroupWorld );
			}

			enQ_compute( evQ, nsCompute * kba * fields_per_cell );

			if(x_up >= 0) {
            	enQ_send( evQ, x_up, (nx * kba * data_width * fields_per_cell), 1000, GroupWorld );
			}

	        if(y_up >= 0) {
       		    enQ_send( evQ, y_up, (ny * kba * data_width * fields_per_cell), 1000, GroupWorld );
			}
		}

		break;

	case 1:
		// Sweep from (Px, 0) outwards towards (0, Py)
		for(uint32_t i = 0; i < nz; i+= kba) {
			if(x_up >= 0) {
				enQ_recv( evQ, x_up, (nx * kba * data_width * fields_per_cell), 2000, GroupWorld );
			}

			if(y_down >= 0) {
				enQ_recv( evQ, y_down, (ny * kba * data_width * fields_per_cell), 2000, GroupWorld );
			}

			enQ_compute( evQ, nsCompute * kba * fields_per_cell );

			if(x_down >= 0) {
				enQ_send( evQ, x_down, (nx * kba * data_width * fields_per_cell), 2000, GroupWorld );
			}

			if(y_up >= 0) {
				enQ_send( evQ, y_up, (ny * kba * data_width * fields_per_cell), 2000, GroupWorld );
			}
		}

		break;

	case 2:
		// Sweep from (Px,Py) outwards towards (0,0)
		for(uint32_t i = 0; i < nz; i+= kba) {
			if(x_up >= 0) {
				enQ_recv( evQ, x_up, (nx * kba * data_width * fields_per_cell), 3000, GroupWorld );
			}

			if(y_up >= 0) {
				enQ_recv( evQ, y_up, (ny * kba * data_width * fields_per_cell), 3000, GroupWorld );
			}

			enQ_compute( evQ, nsCompute * kba * fields_per_cell );

			if(x_down >= 0) {
				enQ_send( evQ, x_down, (nx * kba * data_width * fields_per_cell), 3000, GroupWorld );
	    	}

			if(y_down >= 0) {
				enQ_send( evQ, y_down, (ny * kba * data_width * fields_per_cell), 3000, GroupWorld );
        	}
		}

		break;

	case 3:
		// Sweep from (0, Py) outwards towards (Px, 0)
		for(uint32_t i = 0; i < nz; i+= kba) {
			if(x_down >= 0) {
				enQ_recv( evQ, x_down, (nx * kba * data_width * fields_per_cell), 4000, GroupWorld );
			}

			if(y_up >= 0) {
				enQ_recv( evQ, y_up, (ny * kba * data_width * fields_per_cell), 4000, GroupWorld );
			}

			enQ_compute( evQ, nsCompute * kba * fields_per_cell );

			if(x_up >= 0) {
				enQ_send( evQ, x_up, (nx * kba * data_width * fields_per_cell), 4000, GroupWorld );
        	}

        	if(y_down >= 0) {
            	enQ_send( evQ, y_down, (ny * kba * data_width * fields_per_cell), 4000, GroupWorld );
			}
        }

        break;
	}
	//}

	if( ++m_InnerLoopIndex == 4 ) {
    	if ( ++m_loopIndex == (iterations * 2) ) {
        	return true;
    	} else {
    		m_InnerLoopIndex = 0;
        	return false;
    	}
    } else {
    	return false;
    }
}
