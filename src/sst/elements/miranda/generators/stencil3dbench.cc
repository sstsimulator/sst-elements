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
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/stencil3dbench.h>

using namespace SST::Miranda;

Stencil3DBenchGenerator::Stencil3DBenchGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);

	out = new Output("Stencil3DBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	nX = params.find<uint32_t>("nx", 10);
	nY = params.find<uint32_t>("ny", 10);
	nZ = params.find<uint32_t>("nz", 10);

	datawidth = params.find<uint32_t>("datawidth", 8);

	startZ = params.find<uint32_t>("startz", 0);
	endZ   = params.find<uint32_t>("endz",   nZ);

	maxItr = params.find<uint32_t>("iterations", 1);
	currentItr = 0;

	currentZ = startZ + 1;
}

Stencil3DBenchGenerator::~Stencil3DBenchGenerator() {
	delete out;
}

void Stencil3DBenchGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 2, 0, "Enqueue iteration: %" PRIu32 "...\n", currentItr);
	out->verbose(CALL_INFO, 4, 0, "Itr: Z:[%" PRIu32 ",%" PRIu32 "], Y:[%" PRIu32 ",%" PRIu32 "], X:[%" PRIu32 ",%" PRIu32 "]\n",
		(startZ + 1), (endZ - 1), 1, (nY - 1), 1, (nX - 1));

	uint64_t countReqGen = 0;

		out->verbose(CALL_INFO, 2, 0, "Generating for plane Z=%" PRIu32 "..\n", currentZ);

		for(uint32_t curY = 1; curY < (nY - 1); curY++) {
			out->verbose(CALL_INFO, 4, 0, "Generating for plane (Z=%" PRIu32 ", Y=%" PRIu32 ")...\n", currentZ, curY);

			for(uint32_t curX = 1; curX < (nX - 1); curX++) {
				MemoryOpRequest* read_a = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_b = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_c = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, currentZ - 1), datawidth, READ);

				MemoryOpRequest* read_d = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_e = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_f = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , currentZ - 1), datawidth, READ);

				MemoryOpRequest* read_g = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_h = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, currentZ - 1), datawidth, READ);
				MemoryOpRequest* read_i = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, currentZ - 1), datawidth, READ);

				MemoryOpRequest* read_j = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, currentZ    ), datawidth, READ);
				MemoryOpRequest* read_k = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, currentZ    ), datawidth, READ);
				MemoryOpRequest* read_l = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, currentZ    ), datawidth, READ);

				MemoryOpRequest* read_m = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , currentZ    ), datawidth, READ);
				MemoryOpRequest* read_n = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , currentZ    ), datawidth, READ);
				MemoryOpRequest* read_o = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , currentZ    ), datawidth, READ);

				MemoryOpRequest* read_p = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, currentZ    ), datawidth, READ);
				MemoryOpRequest* read_q = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, currentZ    ), datawidth, READ);
				MemoryOpRequest* read_r = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, currentZ    ), datawidth, READ);

				MemoryOpRequest* read_s = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_t = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_u = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, currentZ + 1), datawidth, READ);

				MemoryOpRequest* read_v = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_w = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_x = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , currentZ + 1), datawidth, READ);

				MemoryOpRequest* read_y = new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_z = new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, currentZ + 1), datawidth, READ);
				MemoryOpRequest* read_zz = new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, currentZ + 1), datawidth, READ);


                                MemoryOpRequest* write_a = new MemoryOpRequest( (nX * nY * nZ * datawidth) +
                                        datawidth * convertPositionToIndex(curX    , curY    , currentZ    ), datawidth, WRITE);
				
                                write_a->addDependency(read_a->getRequestID());
				write_a->addDependency(read_b->getRequestID());
				write_a->addDependency(read_c->getRequestID());
				write_a->addDependency(read_d->getRequestID());
				write_a->addDependency(read_e->getRequestID());
				write_a->addDependency(read_f->getRequestID());
				write_a->addDependency(read_g->getRequestID());
				write_a->addDependency(read_h->getRequestID());
				write_a->addDependency(read_i->getRequestID());
				write_a->addDependency(read_j->getRequestID());
				write_a->addDependency(read_k->getRequestID());
				write_a->addDependency(read_l->getRequestID());
				write_a->addDependency(read_m->getRequestID());
				write_a->addDependency(read_n->getRequestID());
				write_a->addDependency(read_o->getRequestID());
				write_a->addDependency(read_p->getRequestID());
				write_a->addDependency(read_q->getRequestID());
				write_a->addDependency(read_r->getRequestID());
				write_a->addDependency(read_s->getRequestID());
				write_a->addDependency(read_t->getRequestID());
				write_a->addDependency(read_u->getRequestID());
				write_a->addDependency(read_v->getRequestID());
				write_a->addDependency(read_w->getRequestID());
				write_a->addDependency(read_x->getRequestID());
				write_a->addDependency(read_y->getRequestID());
				write_a->addDependency(read_z->getRequestID());
				write_a->addDependency(read_zz->getRequestID());

				q->push_back(read_a);
				q->push_back(read_b);
				q->push_back(read_c);
				q->push_back(read_d);
				q->push_back(read_e);
				q->push_back(read_f);
				q->push_back(read_g);
				q->push_back(read_h);
				q->push_back(read_i);
				q->push_back(read_j);
				q->push_back(read_k);
				q->push_back(read_l);
				q->push_back(read_m);
				q->push_back(read_n);
				q->push_back(read_o);
				q->push_back(read_p);
				q->push_back(read_q);
				q->push_back(read_r);
				q->push_back(read_s);
				q->push_back(read_t);
				q->push_back(read_u);
				q->push_back(read_v);
				q->push_back(read_w);
				q->push_back(read_x);
				q->push_back(read_y);
				q->push_back(read_z);
				q->push_back(read_zz);
				q->push_back(write_a);

				countReqGen += 28;
			}
		}

	out->verbose(CALL_INFO, 4, 0, "Generated %" PRIu64 " requests this iteration.\n", countReqGen);

	if(currentZ == (endZ - 2)) {
		currentZ = startZ + 1;
		currentItr++;
	} else {
		currentZ++;
	}
}

bool Stencil3DBenchGenerator::isFinished() {
	out->verbose(CALL_INFO, 8, 0, "Checking stencil completed - current iteration %" PRIu32 ", max itr: %" PRIu32 "\n", currentItr, maxItr);
	return (currentItr == maxItr);
}

void Stencil3DBenchGenerator::completed() {

}

void Stencil3DBenchGenerator::convertIndexToPosition(const uint32_t index,
		uint32_t* posX, uint32_t* posY, uint32_t* posZ) {

	const int32_t my_plane = index % (nX & nY);
	*posY = my_plane / nX;

	const int32_t remain = my_plane % nX;
	*posX = remain;
	*posZ = index / (nX * nY);
}

uint32_t Stencil3DBenchGenerator::convertPositionToIndex(const uint32_t posX,
		const uint32_t posY, const uint32_t posZ) {

	if( (posX >= nX) || (posY >= nY) || (posZ >= nZ) ) {
		out->fatal(CALL_INFO, -1, "Incorrect position calc: (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
			posX, posY, posZ);

		return 0;
	} else {
               	return (posZ * (nX * nY)) + (posY * nX) + posX;
        }
}
