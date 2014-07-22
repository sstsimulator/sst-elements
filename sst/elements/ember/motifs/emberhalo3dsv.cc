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

#include "emberhalo3dsv.h"
#include "emberirecvev.h"
#include "embersendev.h"
#include "emberwaitev.h"
#include "emberallredev.h"

using namespace SST::Ember;

EmberHalo3DSVGenerator::EmberHalo3DSVGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nx  = (uint32_t) params.find_integer("nx", 100);
	ny  = (uint32_t) params.find_integer("ny", 100);
	nz  = (uint32_t) params.find_integer("nz", 100);

	peX = (uint32_t) params.find_integer("pex", 0);
	peY = (uint32_t) params.find_integer("pey", 0);
	peZ = (uint32_t) params.find_integer("pez", 0);

	items_per_cell = (uint32_t) params.find_integer("fields_per_cell", 1);
	item_chunk = (uint32_t) params.find_integer("field_chunk", 1);

	// Ensure the number of fields in a chunk is evenly divisible
	assert(items_per_cell % item_chunk == 0);

	performReduction = (uint32_t) (params.find_integer("doreduce", 1));
	sizeof_cell = (uint32_t) params.find_integer("datatype_width", 8);

	uint64_t pe_flops = (uint64_t) params.find_integer("peflops", 10000000000);
	uint64_t flops_per_cell = (uint64_t) params.find_integer("flopspercell", 26);

	const uint64_t total_grid_points = (uint64_t) (nx * ny * nz);
	const uint64_t total_flops       = total_grid_points * ((uint64_t) flops_per_cell);

	// Converts FLOP/s into nano seconds of compute
	const double compute_seconds = ( (double) total_flops / ( (double) pe_flops / 1000000000.0 ) );
	nsCompute  = (uint64_t) params.find_integer("computetime", (uint64_t) compute_seconds);
	nsCopyTime = (uint32_t) params.find_integer("copytime", 0);

	iterations = (uint32_t) params.find_integer("iterations", 1);

	x_down = -1;
	x_up   = -1;
	y_down = -1;
	y_up   = -1;
	z_down = -1;
	z_up   = -1;
}

EmberHalo3DSVGenerator::~EmberHalo3DSVGenerator() {

}

void EmberHalo3DSVGenerator::configureEnvironment(const SST::Output* output, uint32_t myRank, uint32_t worldSize) {
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
		output->output("Halo3D copy time:    %" PRIu32 " ns\n", nsCopyTime);
		output->output("Halo3D iterations:   %" PRIu32 "\n", iterations);
		output->output("Halo3D items/cell:   %" PRIu32 "\n", items_per_cell);
		output->output("Halo3D items/chunk:  %" PRIu32 "\n", item_chunk);
		output->output("Halo3D do reduction: %" PRIu32 "\n", performReduction);
	}

	assert( peX * peY * peZ == worldSize );

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", using decomposition: %" PRIu32 "x%" PRIu32 "x%" PRIu32 ".\n",
		rank, peX, peY, peZ);

	int32_t my_Z = 0;
	int32_t my_Y = 0;
	int32_t my_X = 0;

	getPosition(rank, peX, peY, peZ, &my_X, &my_Y, &my_Z);

	y_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z);
	y_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z);
	x_up    = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z);
	x_down  = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z);
	z_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z + 1);
	z_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z - 1);

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", World=%" PRId32 ", X=%" PRId32 ", Y=%" PRId32 ", Z=%" PRId32 ", Px=%" PRId32 ", Py=%" PRId32 ", Pz=%" PRId32 "\n", 
		rank, worldSize, my_X, my_Y, my_Z, peX, peY, peZ);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank, x_up, x_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank, y_up, y_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank, z_up, z_down);

//	assert( (x_up < worldSize) && (y_up < worldSize) && (z_up < worldSize) );
}

void EmberHalo3DSVGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {

		std::vector<MessageRequest*> requests;

		for(uint32_t var_count = 0; var_count < items_per_cell; var_count += item_chunk) {
			EmberComputeEvent* compute = new EmberComputeEvent(nsCompute * (double) item_chunk);
			evQ->push(compute);

			if(x_down > -1) {
				MessageRequest*  req  = new MessageRequest();
	                       	EmberIRecvEvent* recv = new EmberIRecvEvent(x_down, sizeof_cell * ny * nz * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(x_up > -1) {
				MessageRequest*  req  = new MessageRequest();
	                       	EmberIRecvEvent* recv = new EmberIRecvEvent(x_up, sizeof_cell * ny * nz * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(x_down > -1) {
	                       	EmberSendEvent* send = new EmberSendEvent(x_down, sizeof_cell * ny * nz * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			if(x_up > -1) {
	                       	EmberSendEvent* send = new EmberSendEvent(x_up, sizeof_cell * ny * nz * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			for(uint32_t i = 0; i < requests.size(); ++i) {
				EmberWaitEvent* wait = new EmberWaitEvent(requests[i]);
				evQ->push(wait);
			}

			requests.clear();

			if(nsCopyTime > 0) {
				evQ->push( new EmberComputeEvent(nsCopyTime) );
			}

			if(y_down > -1) {
				MessageRequest*  req  = new MessageRequest();
       	                	EmberIRecvEvent* recv = new EmberIRecvEvent(y_down, sizeof_cell * nx * nz * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(y_up > -1) {
				MessageRequest*  req  = new MessageRequest();
       	                	EmberIRecvEvent* recv = new EmberIRecvEvent(y_up, sizeof_cell * nx * nz * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(y_down > -1) {
       	                	EmberSendEvent* send = new EmberSendEvent(y_down, sizeof_cell * nx * nz * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			if(y_up > -1) {
       	                	EmberSendEvent* send = new EmberSendEvent(y_up, sizeof_cell * nx * nz * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			for(uint32_t i = 0; i < requests.size(); ++i) {
				EmberWaitEvent* wait = new EmberWaitEvent(requests[i]);
				evQ->push(wait);
			}

			requests.clear();

			if(nsCopyTime > 0) {
				evQ->push( new EmberComputeEvent(nsCopyTime) );
			}

			if(z_down > -1) {
				MessageRequest*  req  = new MessageRequest();
       	                	EmberIRecvEvent* recv = new EmberIRecvEvent(z_down, sizeof_cell * ny * nx * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(z_up > -1) {
				MessageRequest*  req  = new MessageRequest();
       	                	EmberIRecvEvent* recv = new EmberIRecvEvent(z_up, sizeof_cell * ny * nx * item_chunk, 0, (Communicator) 0, req);
				requests.push_back(req);

				evQ->push(recv);
			}

			if(z_down > -1) {
       	                	EmberSendEvent* send = new EmberSendEvent(z_down, sizeof_cell * ny * nx * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			if(z_up > -1) {
       	                	EmberSendEvent* send = new EmberSendEvent(z_up, sizeof_cell * ny * nx * item_chunk, 0, (Communicator) 0);
				evQ->push(send);
			}

			for(uint32_t i = 0; i < requests.size(); ++i) {
				EmberWaitEvent* wait = new EmberWaitEvent(requests[i]);
				evQ->push(wait);
			}

			requests.clear();

			if(nsCopyTime > 0) {
				evQ->push( new EmberComputeEvent(nsCopyTime) );
			}
		}

		if( (performReduction > 0) && (phase % performReduction == 0) ) {
			evQ->push( new EmberAllreduceEvent(1, EMBER_F64, EMBER_SUM, (Communicator) 0) );
		}
	} else {
		// We are done
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
        	evQ->push(finalize);
	}
}

void EmberHalo3DSVGenerator::finish(const SST::Output* output) {

}
