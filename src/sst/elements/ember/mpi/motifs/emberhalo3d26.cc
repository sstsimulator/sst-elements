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

#include <sst_config.h>

#include "emberhalo3d26.h"

using namespace SST::Ember;

EmberHalo3D26Generator::EmberHalo3D26Generator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "Halo3D26"),
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

	uint64_t flops_per_cell = (uint64_t) params.find("arg.flopspercell", 26);

	const uint64_t total_grid_points = (uint64_t) (nx * ny * nz);

	uint64_t nsCompute = 0;
	if ( params.contains("arg.computetime") ) {
		nsCompute  = (uint64_t) params.find<uint64_t>("arg.computetime");
		compute_the_time = nsCompute;
	} else {
		uint64_t total_flops       = total_grid_points * ((uint64_t) items_per_cell) * ((uint64_t) flops_per_cell);
		double pe_flops = params.find<double>("arg.peflops", 1000000000);

		const double NANO_SECONDS = 1000000000;
		compute_the_time =   ( ( (double) total_flops / pe_flops ) * NANO_SECONDS );

        if(0 == rank()) {
			output("Halo3D: total_flops: %" PRIu64 "\n", total_flops);
			output("Halo3D: peFlops:     %lf\n", pe_flops);
		}
	}

	nsCopyTime = (uint32_t) params.find("arg.copytime", 0);
	iterations = (uint32_t) params.find("arg.iterations", 1);

	xface_down = -1;
        xface_up = -1;

        yface_down = -1;
        yface_up = -1;

        zface_down = -1;
        zface_up = -1;

	line_a = -1;
        line_b = -1;
        line_c = -1;
        line_d = -1;
        line_f = -1;
	line_e = -1;
        line_g = -1;
        line_h = -1;
        line_i = -1;
        line_j = -1;
        line_k = -1;
	line_l = -1;

        corner_a = -1;
        corner_b = -1;
        corner_c = -1;
        corner_d = -1;
        corner_e = -1;
        corner_f = -1;
        corner_g = -1;
	corner_h = -1;

	if(peX == 0 || peY == 0 || peZ == 0) {
		peX = size();
                peY = 1;
                peZ = 1;

                uint32_t meanExisting = (uint32_t) ( (peX + peY + peZ) / 3);
       	        uint32_t varExisting  = (uint32_t) (  ( (peX * peX) + (peY * peY) + (peZ * peZ) ) / 3 ) - (meanExisting * meanExisting);

                for(int32_t i = 0; i < size(); i++) {
                        for(int32_t j = 0; j < size(); j++) {
                                for(int32_t k = 0; k < size(); k++) {
                                        if( (i*j*k) == size() ) {
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
		output("Halo3D compute time: %" PRIu64 " ns\n", compute_the_time );
		output("Halo3D copy time:    %" PRIu32 " ns\n", nsCopyTime);
		output("Halo3D iterations:   %" PRIu32 "\n", iterations);
		output("Halo3D items/cell:   %" PRIu32 "\n", items_per_cell);
		output("Halo3D do reduction: %" PRIu32 "\n", performReduction);
	}

	assert( peX * peY * peZ == (unsigned) size() );

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", using decomposition: %" PRIu32 "x%" PRIu32 "x%" PRIu32 ".\n",
		rank(), peX, peY, peZ);

	int32_t my_Z = 0;
	int32_t my_Y = 0;
	int32_t my_X = 0;

	getPosition(rank(), peX, peY, peZ, &my_X, &my_Y, &my_Z);

	xface_down = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z);
        xface_up = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z);

        yface_down = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z);
        yface_up = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z);

        zface_down = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z - 1);
        zface_up = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z + 1);

	line_a = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y - 1, my_Z);
        line_b = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z - 1);
        line_c = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y - 1, my_Z);
        line_d = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z + 1);
        line_e = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z + 1);
        line_f = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z + 1);
        line_g = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z - 1);
        line_h = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z - 1);
        line_i = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y + 1, my_Z);
        line_j = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z + 1);
        line_k = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y + 1, my_Z);
        line_l = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z - 1);

        corner_a = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y - 1, my_Z + 1);
        corner_b = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y - 1, my_Z + 1);
        corner_c = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y - 1, my_Z - 1);
        corner_d = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y - 1, my_Z - 1);
        corner_e = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y + 1, my_Z + 1);
        corner_f = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y + 1, my_Z + 1);
        corner_g = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y + 1, my_Z - 1);
	corner_h = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y + 1, my_Z - 1);

	// Count up the requests which could be posted (this is a high water mark)
	char requestLength = 0;

	requestLength =  (xface_down > -1) ? 1 : 0;
	requestLength += (xface_up > -1) ? 1 : 0;
	requestLength += (yface_down > -1) ? 1 : 0;
	requestLength += (yface_up > -1) ? 1 : 0;
	requestLength += (zface_up > -1) ? 1 : 0;
	requestLength += (zface_down > -1) ? 1 : 0;

	requestLength += (line_a > -1) ? 1 : 0;
	requestLength += (line_b > -1) ? 1 : 0;
	requestLength += (line_c > -1) ? 1 : 0;
	requestLength += (line_d > -1) ? 1 : 0;
	requestLength += (line_e > -1) ? 1 : 0;
	requestLength += (line_f > -1) ? 1 : 0;
	requestLength += (line_g > -1) ? 1 : 0;
	requestLength += (line_h > -1) ? 1 : 0;
	requestLength += (line_i > -1) ? 1 : 0;
	requestLength += (line_j > -1) ? 1 : 0;
	requestLength += (line_k > -1) ? 1 : 0;
	requestLength += (line_l > -1) ? 1 : 0;

	requestLength += (corner_a > -1) ? 1 : 0;
	requestLength += (corner_b > -1) ? 1 : 0;
	requestLength += (corner_c > -1) ? 1 : 0;
	requestLength += (corner_d > -1) ? 1 : 0;
	requestLength += (corner_e > -1) ? 1 : 0;
	requestLength += (corner_f > -1) ? 1 : 0;
	requestLength += (corner_g > -1) ? 1 : 0;
	requestLength += (corner_h > -1) ? 1 : 0;

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", World=%" PRId32 ", X=%" PRId32 ", Y=%" PRId32 ", Z=%" PRId32 ", Px=%" PRId32 ", Py=%" PRId32 ", Pz=%" PRId32 "\n", 
		rank(), size(), my_X, my_Y, my_Z, peX, peY, peZ);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Total communication partners: %d\n", rank(), (int) requestLength);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank(), xface_up, xface_down);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank(), yface_up, yface_down);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank(), zface_up, zface_down);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LA: %" PRId32 ", LB: %" PRId32 ", LC: %" PRId32 ", LD: %" PRId32 "\n", rank(), line_a, line_b, line_c, line_d);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LE: %" PRId32 ", LF: %" PRId32 ", LG: %" PRId32 ", LH: %" PRId32 "\n", rank(), line_e, line_f, line_g, line_h);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LI: %" PRId32 ", LJ: %" PRId32 ", LK: %" PRId32 ", LL: %" PRId32 "\n", rank(), line_i, line_j, line_k, line_l);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", CA: %" PRId32 ", CB: %" PRId32 ", CC: %" PRId32 ", CD: %" PRId32 "\n", rank(),
		corner_a, corner_b, corner_c, corner_d);
	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", CE: %" PRId32 ", CF: %" PRId32 ", CG: %" PRId32 ", CH: %" PRId32 "\n", rank(),
		corner_e, corner_f, corner_g, corner_h);

	verbose(CALL_INFO, 4, 0, "Allocating request entries...\n");
	requests.resize( requestLength * 2 );
}

bool EmberHalo3D26Generator::generate( std::queue<EmberEvent*>& evQ) {
	verbose(CALL_INFO, 1, 0, "Iteration on rank %" PRId32 "\n", rank());

		enQ_compute( evQ, compute_the_time );

		int nextRequest = 0;

		if(xface_down > -1) {
			enQ_irecv( evQ, xface_down, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(xface_up > -1) {
			enQ_irecv( evQ, xface_up, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(yface_down > -1) {
			enQ_irecv( evQ, yface_down, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(yface_up > -1) {
			enQ_irecv( evQ, yface_up, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(zface_down > -1) {
			enQ_irecv( evQ, zface_down, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(zface_up > -1) {
			enQ_irecv( evQ, zface_up, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_a > -1) {
			enQ_irecv( evQ, line_a, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_b > -1) {
			enQ_irecv( evQ, line_b, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_c > -1) {
			enQ_irecv( evQ, line_c, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_d > -1) {
			enQ_irecv( evQ, line_d, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_e > -1) {
			enQ_irecv( evQ, line_e, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_f > -1) {
			enQ_irecv( evQ, line_f, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_g > -1) {
			enQ_irecv( evQ, line_g, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_h > -1) {
			enQ_irecv( evQ, line_h, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_i > -1) {
			enQ_irecv( evQ, line_i, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_j > -1) {
			enQ_irecv( evQ, line_j, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_k > -1) { 
			enQ_irecv( evQ, line_k, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_l > -1) {
			enQ_irecv( evQ, line_l, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_a > -1) {
			enQ_irecv( evQ, corner_a, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_b > -1) {
			enQ_irecv( evQ, corner_b, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_c > -1) {
			enQ_irecv( evQ, corner_c, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_d > -1) {
			enQ_irecv( evQ, corner_d, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_e > -1) {
			enQ_irecv( evQ, corner_e, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_f > -1) {
			enQ_irecv( evQ, corner_f, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_g > -1) {
			enQ_irecv( evQ, corner_g, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_h > -1) {
			enQ_irecv( evQ, corner_h, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}


		// Enqueue the sends
		if(xface_down > -1) {
			enQ_isend( evQ, xface_down, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(xface_up > -1) {
			enQ_isend( evQ, xface_up, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(yface_down > -1) {
			enQ_isend( evQ, yface_down, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(yface_up > -1) {
			enQ_isend( evQ, yface_up, items_per_cell * sizeof_cell * nx * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(zface_down > -1) {
			enQ_isend( evQ, zface_down, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(zface_up > -1) { 
			enQ_isend( evQ, zface_up, items_per_cell * sizeof_cell * ny * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_a > -1) {
			enQ_isend( evQ, line_a, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_b > -1) {
			enQ_isend( evQ, line_b, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_c > -1) {
			enQ_isend( evQ, line_c, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_d > -1) {
			enQ_isend( evQ, line_d, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_e > -1) { 
			enQ_isend( evQ, line_e, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_f > -1) {
			enQ_isend( evQ, line_f, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_g > -1) {
			enQ_isend( evQ, line_g, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_h > -1) {
			enQ_isend( evQ, line_h, items_per_cell * sizeof_cell * ny, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_i > -1) {
			enQ_isend( evQ, line_i, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_j > -1) {
			enQ_isend( evQ, line_j, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_k > -1) {
			enQ_isend( evQ, line_k, items_per_cell * sizeof_cell * nz, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(line_l > -1) {
			enQ_isend( evQ, line_l, items_per_cell * sizeof_cell * nx, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_a > -1) {
			enQ_isend( evQ, corner_a, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_b > -1) {
			enQ_isend( evQ, corner_b, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_c > -1) { 
			enQ_isend( evQ, corner_c, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_d > -1) {
			enQ_isend( evQ, corner_d, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_e > -1) {
			enQ_isend( evQ, corner_e, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_f > -1) {
			enQ_isend( evQ, corner_f, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_g > -1) {
			enQ_isend( evQ, corner_g, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		if(corner_h > -1) { 
			enQ_isend( evQ, corner_h, items_per_cell * sizeof_cell * 1, 0, GroupWorld, &requests[nextRequest]);
			nextRequest++;
		}

		// Enqueue a wait all for all the communications we have set up
		enQ_waitall( evQ, nextRequest, &requests[0], NULL );

		verbose(CALL_INFO, 1, 0, "Iteration on rank %" PRId32 " completed generation, %d events in queue\n",
			rank(), (int)evQ.size());

    if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }

}
