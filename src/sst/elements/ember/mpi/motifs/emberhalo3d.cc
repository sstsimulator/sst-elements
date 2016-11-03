// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

#include "emberhalo3d.h"

using namespace SST::Ember;

EmberHalo3DGenerator::EmberHalo3DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "Halo3D"), 
	m_loopIndex(0)
{
	nx  = (uint32_t) params.find("arg.nx", 100);
	ny  = (uint32_t) params.find("arg.ny", 100);
	nz  = (uint32_t) params.find("arg.nz", 100);

	peX = (uint32_t) params.find("arg.pex", 0);
	peY = (uint32_t) params.find("arg.pey", 0);
	peZ = (uint32_t) params.find("arg.pez", 0);

	items_per_cell = (uint32_t) params.find("arg.fields_per_cell", 1);
	performReduction = (params.find("arg.doreduce", 1) == 1);
	sizeof_cell = (uint32_t) params.find("arg.datatype_width", 8);

	uint64_t pe_flops = (uint64_t) params.find("arg.peflops", 10000000000);
	uint64_t flops_per_cell = (uint64_t) params.find("arg.flopspercell", 26);

	const uint64_t total_grid_points = (uint64_t) (nx * ny * nz);
	const uint64_t total_flops       = total_grid_points * ((uint64_t) items_per_cell) * ((uint64_t) flops_per_cell);

	// Converts FLOP/s into nano seconds of compute
	const double compute_seconds = ( (double) total_flops / ( (double) pe_flops / 1000000000.0 ) );
	nsCompute  = (uint64_t) params.find("arg.computetime", (uint64_t) compute_seconds);
	nsCopyTime = (uint32_t) params.find("arg.copytime", 0);

	iterations = (uint32_t) params.find("arg.iterations", 1);

	x_down = -1;
	x_up   = -1;
	y_down = -1;
	y_up   = -1;
	z_down = -1;
	z_up   = -1;

	configure();
}


void EmberHalo3DGenerator::configure()
{
	unsigned worldSize = size();

	if(peX == 0 || peY == 0 || peZ == 0) {
		peX = worldSize;
                peY = 1;
                peZ = 1;

                uint32_t meanExisting = (uint32_t) ( (peX + peY + peZ) / 3);
       	        uint32_t varExisting  = (uint32_t) (  ( (peX * peX) + (peY * peY) + (peZ * peZ) ) / 3 ) - (meanExisting * meanExisting);

                for(uint32_t i = 0; i < worldSize; i++) {
                        for(uint32_t j = 0; j < worldSize; j++) {
                                for(uint32_t k = 0; k < worldSize; k++) {
                                        if( (i*j*k) == worldSize ) {
                                                // We have a match but is it better than the test we have?
                                                const uint32_t meanNew      = (uint32_t) ( ( i + j + k ) / 3 );
                                                const uint32_t varNew       = ( ( (i * i) + (j * j) + (k * k) ) / 3 ) - ( meanNew * meanNew );

                                                if(varNew <= varExisting) {
                                                        if(0 == rank()) {
                                                                verbose(CALL_INFO, 2, 0, "Found an improved decomposition solution: %" PRIu32 " x %" PRIu32 " x %" PRIu32 "\n",
                                                                        i, j, k);
                                                        }

                                                        // Update our decomposition
                                                        peX = i;
                                                        peY = j;
                                                        peZ = k;

                                       	                 // Update the statistics which summarize this selection
                      	                                meanExisting = (uint32_t) ( (peX + peY + peZ) / 3);
                       	                                varExisting  = (uint32_t) (  ( (peX * peX) + (peY * peY) + (peZ * peZ) ) / 3 ) - (meanExisting * meanExisting);
       	                                       	}
        	                         }
                        	}
                	}
        	}

	}

    if(0 == rank()) {
		output("Halo3D processor decomposition solution: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "\n", peX, peY, peZ);
		output("Halo3D problem size: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "\n", nx, ny, nz);
		output("Halo3D compute time: %" PRIu32 " ns\n", nsCompute);
		output("Halo3D copy time: %" PRIu32 " ns\n", nsCopyTime);
		output("Halo3D iterations: %" PRIu32 "\n", iterations);
		output("Halo3D iterms/cell: %" PRIu32 "\n", items_per_cell);
		output("Halo3D do reduction: %" PRIu32 "\n", performReduction);
	}

	assert( peX * peY * peZ == worldSize );

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", using decomposition: %" PRIu32 "x%" PRIu32 "x%" PRIu32 ".\n",
		rank(), peX, peY, peZ);

	int32_t my_Z = 0;
	int32_t my_Y = 0;
	int32_t my_X = 0;

	getPosition(rank(), peX, peY, peZ, &my_X, &my_Y, &my_Z);

	//output("My rank is: %" PRIu32 "\n", rank()); // NetworkSim

	y_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z);
	y_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z);
	x_up    = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z);
	x_down  = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z);
	z_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z + 1);
	z_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z - 1);

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", World=%" PRId32 ", X=%" PRId32 ", Y=%" PRId32 ", Z=%" PRId32 ", Px=%" PRId32 ", Py=%" PRId32 ", Pz=%" PRId32 "\n", 
		rank(), worldSize, my_X, my_Y, my_Z, peX, peY, peZ);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank(), x_up, x_down);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank(), y_up, y_down);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank(), z_up, z_down);

//	assert( (x_up < worldSize) && (y_up < worldSize) && (z_up < worldSize) );
}

bool EmberHalo3DGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    verbose(CALL_INFO, 1, 0, "loop=%d\n", m_loopIndex );

		enQ_compute( evQ, nsCompute);

		std::vector<MessageRequest*> requests;

		if(x_down > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, x_down, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, req);
		}

		if(x_up > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, x_up, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, req);
		}

		if(x_down > -1) {
			enQ_send( evQ ,x_down, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld);
		}

		if(x_up > -1) {
			enQ_send( evQ ,x_up, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld);
		}

		for(uint32_t i = 0; i < requests.size(); ++i) {
			enQ_wait( evQ, requests[i]);
		}

		requests.clear();

		if(nsCopyTime > 0) {
			enQ_compute( evQ, nsCopyTime);
		}

		// -- ////////////////////////////////////////////////////////////////////////////

		if(y_down > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, y_down, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, req);
		}

		if(y_up > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, y_up, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, req);
		}

		if(y_down > -1) {
			enQ_send( evQ ,y_down, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld);
		}

		if(y_up > -1) {
			enQ_send( evQ ,y_up, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld);
		}

		for(uint32_t i = 0; i < requests.size(); ++i) {
			enQ_wait( evQ, requests[i]);
		}

		requests.clear();

		if(nsCopyTime > 0) {
			enQ_compute( evQ, nsCopyTime);
		}

		// -- ////////////////////////////////////////////////////////////////////////////

		if(z_down > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, z_down, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, req);
		}

		if(z_up > -1) {
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);

			enQ_irecv( evQ, z_up, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, req);
		}

		if(z_down > -1) {
			enQ_send( evQ ,z_down, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld);
		}

		if(z_up > -1) {
			enQ_send( evQ ,z_up, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld);
		}

		for(uint32_t i = 0; i < requests.size(); ++i) {
			enQ_wait( evQ, requests[i]);
		}

		requests.clear();

		if(nsCopyTime > 0) {
			enQ_compute( evQ, nsCopyTime );
		}

		if(performReduction) {
			enQ_allreduce( evQ, NULL, NULL, 1, DOUBLE, SUM, GroupWorld);
		}

    if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }

}
