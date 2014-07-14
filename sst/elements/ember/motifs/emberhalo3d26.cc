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

#include "emberhalo3d26.h"
#include "emberirecvev.h"
#include "emberisendev.h"
#include "embersendev.h"
#include "emberwaitev.h"
#include "emberwaitallev.h"
#include "emberallredev.h"

using namespace SST::Ember;

EmberHalo3D26Generator::EmberHalo3D26Generator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nx  = (uint32_t) params.find_integer("nx", 100);
	ny  = (uint32_t) params.find_integer("ny", 100);
	nz  = (uint32_t) params.find_integer("nz", 100);

	peX = (uint32_t) params.find_integer("pex", 0);
	peY = (uint32_t) params.find_integer("pey", 0);
	peZ = (uint32_t) params.find_integer("pez", 0);

	items_per_cell = (uint32_t) params.find_integer("fields_per_cell", 1);
	performReduction = (params.find_integer("doreduce", 1) == 1);
	sizeof_cell = (uint32_t) params.find_integer("datatype_width", 8);

	uint64_t pe_flops = (uint64_t) params.find_integer("peflops", 10000000000);
	uint64_t flops_per_cell = (uint64_t) params.find_integer("flopspercell", 26);

	const uint64_t total_grid_points = (uint64_t) (nx * ny * nz);
	const uint64_t total_flops       = total_grid_points * ((uint64_t) items_per_cell) * ((uint64_t) flops_per_cell);

	// Converts FLOP/s into nano seconds of compute
	const double compute_seconds = ( (double) total_flops / ( (double) pe_flops / 1000000000.0 ) );
	nsCompute  = (uint64_t) params.find_integer("computetime", (uint64_t) compute_seconds);
	nsCopyTime = (uint32_t) params.find_integer("copytime", 0);

	iterations = (uint32_t) params.find_integer("iterations", 1);

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
}

EmberHalo3D26Generator::~EmberHalo3D26Generator() {
	if(requests != NULL) {
		free(requests);
	}
}

void EmberHalo3D26Generator::configureEnvironment(const SST::Output* output, uint32_t myRank, uint32_t worldSize) {
	rank = myRank;

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
                                                        if(0 == rank) {
                                                                output->verbose(CALL_INFO, 2, 0, "Found an improved decomposition solution: %" PRIu32 " x %" PRIu32 " x %" PRIu32 "\n",
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

        if(0 == myRank) {
		output->output("Halo3D processor decomposition solution: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "\n", peX, peY, peZ);
		output->output("Halo3D problem size: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "\n", nx, ny, nz);
		output->output("Halo3D compute time: %" PRIu32 " ns\n", nsCompute);
		output->output("Halo3D copy time: %" PRIu32 " ns\n", nsCopyTime);
		output->output("Halo3D iterations: %" PRIu32 "\n", iterations);
		output->output("Halo3D iterms/cell: %" PRIu32 "\n", items_per_cell);
		output->output("Halo3D do reduction: %" PRIu32 "\n", performReduction);
	}

	assert( peX * peY * peZ == worldSize );

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", using decomposition: %" PRIu32 "x%" PRIu32 "x%" PRIu32 ".\n",
		rank, peX, peY, peZ);

	int32_t my_Z = 0;
	int32_t my_Y = 0;
	int32_t my_X = 0;

	getPosition(rank, peX, peY, peZ, &my_X, &my_Y, &my_Z);

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

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", World=%" PRId32 ", X=%" PRId32 ", Y=%" PRId32 ", Z=%" PRId32 ", Px=%" PRId32 ", Py=%" PRId32 ", Pz=%" PRId32 "\n", 
		rank, worldSize, my_X, my_Y, my_Z, peX, peY, peZ);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Total communication partners: %d\n", rank, (int) requestLength);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank, xface_up, xface_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank, yface_up, yface_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank, zface_up, zface_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LA: %" PRId32 ", LB: %" PRId32 ", LC: %" PRId32 ", LD: %" PRId32 "\n", rank, line_a, line_b, line_c, line_d);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LE: %" PRId32 ", LF: %" PRId32 ", LG: %" PRId32 ", LH: %" PRId32 "\n", rank, line_e, line_f, line_g, line_h);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", LI: %" PRId32 ", LJ: %" PRId32 ", LK: %" PRId32 ", LL: %" PRId32 "\n", rank, line_i, line_j, line_k, line_l);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", CA: %" PRId32 ", CB: %" PRId32 ", CC: %" PRId32 ", CD: %" PRId32 "\n", rank,
		corner_a, corner_b, corner_c, corner_d);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", CE: %" PRId32 ", CF: %" PRId32 ", CG: %" PRId32 ", CH: %" PRId32 "\n", rank,
		corner_e, corner_f, corner_g, corner_h);

	output->verbose(CALL_INFO, 4, 0, "Allocating request entries...\n");
	requests = (MessageRequest*) malloc(sizeof(MessageRequest) * (((size_t) requestLength) * 2));
}

void EmberHalo3D26Generator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		output->verbose(CALL_INFO, 1, 0, "Iteration on rank %" PRId32 "\n", rank);

		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		int nextRequest = 0;

		if(xface_down > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(xface_down, items_per_cell * sizeof_cell * ny * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(xface_up > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(xface_up, items_per_cell * sizeof_cell * ny * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(yface_down > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(yface_down, items_per_cell * sizeof_cell * nx * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(yface_up > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(yface_up, items_per_cell * sizeof_cell * nx * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(zface_down > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(zface_down, items_per_cell * sizeof_cell * ny * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(zface_up > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(zface_up, items_per_cell * sizeof_cell * ny * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_a > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_a, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_b > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_b, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_c > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_c, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_d > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_d, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_e > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_e, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_f > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_f, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_g > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_g, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_h > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_h, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_i > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_i, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_j > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_j, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_k > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_k, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(line_l > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(line_l, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_a > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_a, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_b > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_b, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_c > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_c, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_d > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_d, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_e > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_e, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_f > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_f, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_g > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_g, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}

		if(corner_h > -1) {
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(corner_h, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(recv);
		}


		// Enqueue the sends
		if(xface_down > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(xface_down, items_per_cell * sizeof_cell * ny * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(xface_up > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(xface_up, items_per_cell * sizeof_cell * ny * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(yface_down > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(yface_down, items_per_cell * sizeof_cell * nx * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(yface_up > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(yface_up, items_per_cell * sizeof_cell * nx * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(zface_down > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(zface_down, items_per_cell * sizeof_cell * ny * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(zface_up > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(zface_up, items_per_cell * sizeof_cell * ny * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_a > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_a, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_b > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_b, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_c > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_c, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_d > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_d, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_e > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_e, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_f > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_f, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_g > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_g, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_h > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_h, items_per_cell * sizeof_cell * ny, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_i > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_i, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_j > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_j, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_k > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_k, items_per_cell * sizeof_cell * nz, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(line_l > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(line_l, items_per_cell * sizeof_cell * nx, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_a > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_a, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_b > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_b, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_c > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_c, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_d > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_d, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_e > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_e, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_f > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_f, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_g > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_g, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		if(corner_h > -1) {
                       	EmberISendEvent* send = new EmberISendEvent(corner_h, items_per_cell * sizeof_cell * 1, 0, (Communicator) 0, &requests[nextRequest]);
			nextRequest++;

			evQ->push(send);
		}

		// Enqueue a wait all for all the communications we have set up
		evQ->push( new EmberWaitallEvent( nextRequest, requests, false ) );

		output->verbose(CALL_INFO, 1, 0, "Iteration on rank %" PRId32 " completed generation, %d events in queue\n",
			rank, (int)evQ->size());
	} else {
		// We are done
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
        	evQ->push(finalize);
	}
}

void EmberHalo3D26Generator::finish(const SST::Output* output) {

}
