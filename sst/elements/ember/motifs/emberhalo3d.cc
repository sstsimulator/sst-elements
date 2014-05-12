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

#include "emberhalo3d.h"
#include "emberirecvev.h"
#include "embersendev.h"
#include "emberwaitev.h"
#include "emberallredev.h"

using namespace SST::Ember;

EmberHalo3DGenerator::EmberHalo3DGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nx  = (uint32_t) params.find_integer("nx", 100);
	ny  = (uint32_t) params.find_integer("ny", 100);
	nz  = (uint32_t) params.find_integer("nz", 100);

	peX = (uint32_t) params.find_integer("pex", 0);
	peY = (uint32_t) params.find_integer("pey", 0);
	peZ = (uint32_t) params.find_integer("pez", 0);

	items_per_cell = (uint32_t) params.find_integer("itemspercell", 1);
	performReduction = (params.find_integer("doreduce", 1) == 1);

	nsCompute  = (uint32_t) params.find_integer("compute", 100);
	nsCopyTime = (uint32_t) params.find_integer("copytime", 0);

	iterations = (uint32_t) params.find_integer("iterations", 1);

	x_down = -1;
	x_up   = -1;
	y_down = -1;
	y_up   = -1;
	z_down = -1;
	z_up   = -1;
}

EmberHalo3DGenerator::~EmberHalo3DGenerator() {

}

void EmberHalo3DGenerator::configureEnvironment(const SST::Output* output, uint32_t myRank, uint32_t worldSize) {
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

		if(0 == myRank) {
			output->output("Halo3D processor decomposition solution: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "\n", peX, peY, peZ);
		}
	}

	assert( peX * peY * peZ == worldSize );

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", using decomposition: %" PRIu32 "x%" PRIu32 "x%" PRIu32 ".\n",
		rank, peX, peY, peZ);

	const int32_t my_plane = rank % (peX * peY);
	const int32_t my_Y     = my_plane / peX;
	const int32_t remain   = my_plane % peX;
	const int32_t my_X     = remain != 0 ? remain : 0;
	const int32_t my_Z     = rank / (peX * peY);

	y_up   = (my_Y != 0) ? rank - peX : -1;
	y_down = (my_Y != (peY - 1)) ? rank + peX : -1;

	x_up   = (my_X != (peX - 1)) ? rank + 1 : -1;
	x_down = (my_X != 0) ? rank - 1 : -1;

	z_up   = (my_Z != peZ - 1) ? rank + (peX*peY) : -1;
	z_down = (my_Z != 0) ? rank - (peX*peY) : -1;

	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank, x_up, x_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank, y_up, y_down);
	output->verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank, z_up, z_down);
}

void EmberHalo3DGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	if(phase < iterations) {
		EmberComputeEvent* compute = new EmberComputeEvent(nsCompute);
		evQ->push(compute);

		std::vector<MessageRequest*> requests;

		if(x_down > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(x_down, items_per_cell * ny * nz, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(x_up > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(x_up, items_per_cell * ny * nz, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(x_down > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(x_down, items_per_cell * ny * nz, 0, (Communicator) 0);
			evQ->push(send);
		}

		if(x_up > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(x_up, items_per_cell * ny * nz, 0, (Communicator) 0);
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

		// -- ////////////////////////////////////////////////////////////////////////////

		if(y_down > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(y_down, items_per_cell * nx * nz, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(y_up > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(y_up, items_per_cell * nx * nz, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(y_down > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(y_down, items_per_cell * nx * nz, 0, (Communicator) 0);
			evQ->push(send);
		}

		if(y_up > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(y_up, items_per_cell * nx * nz, 0, (Communicator) 0);
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

		// -- ////////////////////////////////////////////////////////////////////////////

		if(z_down > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(z_down, items_per_cell * ny * nx, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(z_up > -1) {
			MessageRequest*  req  = new MessageRequest();
                       	EmberIRecvEvent* recv = new EmberIRecvEvent(z_up, items_per_cell * ny * nx, 0, (Communicator) 0, req);
			requests.push_back(req);

			evQ->push(recv);
		}

		if(z_down > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(z_down, items_per_cell * ny * nx, 0, (Communicator) 0);
			evQ->push(send);
		}

		if(z_up > -1) {
                       	EmberSendEvent* send = new EmberSendEvent(z_up, items_per_cell * ny * nx, 0, (Communicator) 0);
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

		if(performReduction) {
			evQ->push( new EmberAllreduceEvent(1, EMBER_F64, EMBER_SUM, (Communicator) 0) );
		}
	} else {
		// We are done
		EmberFinalizeEvent* finalize = new EmberFinalizeEvent();
        	evQ->push(finalize);
	}
}

void EmberHalo3DGenerator::finish(const SST::Output* output) {

}
