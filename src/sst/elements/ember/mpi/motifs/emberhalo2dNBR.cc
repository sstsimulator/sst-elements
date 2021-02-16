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

#include "emberhalo2dNBR.h"

using namespace SST::Ember;

EmberHalo2DNBRGenerator::EmberHalo2DNBRGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Halo2DNBR"),
	m_loopIndex(0)
{
	iterations = (uint32_t) params.find("arg.iterations", 10);
	nsCompute = (uint32_t) params.find("arg.computenano", 10);
	messageSizeX = (uint32_t) params.find("arg.messagesizey", 128);
	messageSizeY = (uint32_t) params.find("arg.messagesizex", 128);

	sizeX = (uint32_t) params.find("arg.sizex", 0);
	sizeY = (uint32_t) params.find("arg.sizey", 0);

	// Set configuration so we do not exchange messages
	procWest  = 0;
	procEast = 0;
	procNorth = 0;
	procSouth = 0;

	sendWest = false;
	sendEast = false;
	sendNorth = false;
	sendSouth = false;

	messageCount = 0;

	configure();
}

void EmberHalo2DNBRGenerator::completed(const SST::Output* output, uint64_t) {
	output->verbose(CALL_INFO, 2, 0, "Generator finishing, sent: %" PRIu32 " messages.\n", messageCount);
}

void EmberHalo2DNBRGenerator::configure()
{
    if(0 == rank()) {
        output("PingPong, size=%d msgSizeX=%d msgSizeY=%d"
            " iter=%d nsCompute=%d\n",
            size(),messageSizeX,messageSizeY,iterations,nsCompute);
    }

	// Do we need to auto-size the 2DNBR processor array?
	if(0 == sizeX || 0 == sizeY) {
		uint32_t localX = SST::Math::square_root(size());

		while(localX >= 1) {
			uint32_t localY = (uint32_t) size() / localX;

			if(localY * localX == (unsigned)size()) {
				break;
			}

			localX--;
		}

		sizeX = localX;
		sizeY = size() / sizeX;
	}

	// Check we are not leaving ranks behind
	assert((unsigned)size() == (sizeX * sizeY));

	verbose(CALL_INFO, 2, 0, "Processor grid dimensions, X=%" PRIu32 ", Y=%" PRIu32 "\n",
		sizeX, sizeY);

	if(rank() % sizeX > 0) {
		sendWest = true;
		procWest = rank() - 1;
	}

	if((rank() + 1) % sizeX != 0) {
		sendEast = true;
		procEast = rank() + 1;
	}

	if( (unsigned) rank() >= sizeX) {
		sendNorth = true;
		procNorth = rank() - sizeX;
	}

	if( (unsigned) rank() < (size() - sizeX)) {
		sendSouth = true;
		procSouth = rank() + sizeX;
	}

	verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 ", send left=%s %" PRIu32 ", send right= %s %" PRIu32
		", send above=%s %" PRIu32 ", send below=%s %" PRIu32 "\n",
		rank(),
		(sendWest  ? "Y" : "N"), procWest,
		(sendEast ? "Y" : "N"), procEast,
		(sendSouth ? "Y" : "N"), procSouth,
		(sendNorth ? "Y" : "N"), procNorth);
}

bool EmberHalo2DNBRGenerator::generate( std::queue<EmberEvent*>& evQ )
{
    if( 0 == m_loopIndex) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(),size());
    }

		verbose(CALL_INFO, 2, 0, "Halo 2DNBR motif generating events for loopIndex %" PRIu32 "\n", m_loopIndex);

		enQ_compute( evQ, nsCompute );

		// Do the horizontal exchange first
		if(rank() % 2 == 0) {
			if(sendEast) {
				MessageRequest*  req  = new MessageRequest();
				enQ_irecv( evQ, procEast, messageSizeX, 0, GroupWorld, req);
				enQ_send( evQ, procEast, messageSizeX, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}

			if(sendWest) {
				MessageRequest*  req  = new MessageRequest();
				enQ_irecv( evQ, procWest, messageSizeX, 0, GroupWorld, req );
				enQ_send( evQ, procWest, messageSizeX, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}
		} else {

			if(sendWest) {
				MessageRequest*  req  = new MessageRequest();
				enQ_irecv( evQ, procWest, messageSizeX, 0, GroupWorld, req);
				enQ_send( evQ, procWest, messageSizeX, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}

			if(sendEast) {
				MessageRequest*  req  = new MessageRequest();
				enQ_irecv( evQ, procEast, messageSizeX, 0, GroupWorld,req);
				enQ_send( evQ, procEast, messageSizeX, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}
		}

		// Now do the vertical exchanges
		if( (rank() / sizeX) % 2 == 0) {
			if(sendNorth) {
				MessageRequest* req  = new MessageRequest();
				enQ_irecv( evQ, procNorth, messageSizeY, 0, GroupWorld,req);
				enQ_send( evQ, procNorth, messageSizeY, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}

			if(sendSouth) {
				MessageRequest* req    = new MessageRequest();
				enQ_irecv( evQ, procSouth, messageSizeY, 0, GroupWorld,req);
				enQ_send( evQ, procSouth, messageSizeY, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}
		} else {

			if(sendSouth) {
				MessageRequest* req    = new MessageRequest();
				enQ_irecv( evQ, procSouth, messageSizeY, 0, GroupWorld,req);
				enQ_send( evQ, procSouth, messageSizeY, 0, GroupWorld);
				enQ_wait( evQ,  req );

				messageCount++;
			}

			if(sendNorth) {
				MessageRequest* req    = new MessageRequest();
				enQ_irecv( evQ, procNorth, messageSizeY, 0, GroupWorld,req);
				enQ_send( evQ, procNorth, messageSizeY, 0, GroupWorld);

				enQ_wait( evQ,  req );

				messageCount++;
			}
		}

		verbose(CALL_INFO, 2, 0, "Halo 2DNBR motif completed event generation for loopIndex: %" PRIu32 "\n", m_loopIndex);

    if ( ++m_loopIndex == iterations ) {
        return true;
    } else {
        return false;
    }

}
