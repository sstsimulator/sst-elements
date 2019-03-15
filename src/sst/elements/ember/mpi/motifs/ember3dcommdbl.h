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


#ifndef _H_EMBER_3D_COMM_DOUBLING
#define _H_EMBER_3D_COMM_DOUBLING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class Ember3DCommDoublingGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        Ember3DCommDoublingGenerator,
        "ember",
        "CommDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a communication doubling pattern based on a research scientific analytics problem",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "pex",      "Sets the processors in the X-dimension", "1"   },
        {   "pey",      "Sets the processors in the Y-dimension", "1"   },
        {   "pez",      "Sets the processors in the Z-dimension", "1"   },
        {       "basephase",    "Starts the phase at offset.", "0" },
        {   "compute_at_step",  "Sets the computation time in between each communication phase in nanoseconds", "1000"  },
        {   "items_per_node",   "Sets the number of items to exchange between nodes per phase", "100" },
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )

public:
	Ember3DCommDoublingGenerator(SST::Component* owner, Params& params);
	~Ember3DCommDoublingGenerator() {}
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ );
	int32_t power3(const uint32_t expon);

private:
	uint32_t phase;
	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t computeBetweenSteps;
	uint32_t items_per_node;
//	uint32_t iterations;
	MessageRequest* requests;
	uint32_t next_request;

	uint32_t basePhase;
	uint32_t itemsThisPhase;

};

}
}

#endif
