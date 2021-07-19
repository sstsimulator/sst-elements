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


#ifndef _H_EMBER_HALO_2D
#define _H_EMBER_HALO_2D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo2DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberHalo2DGenerator,
        "ember",
         "Halo2DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a 2D halo exchange Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",       "Sets the number of halo2d operations to perform",  "10"},
        {   "arg.computenano",      "Sets the number of nanoseconds to compute for",    "10"},
        {   "arg.messagesizex",     "Sets the message size in X-dimension (in bytes)",  "128"},
        {   "arg.messagesizey",     "Sets the message size in Y-dimension (in bytes)",  "128"},
        {   "arg.computecopy",      "Sets the time spent copying data between messages",    "5"},
        {   "arg.sizex",        "Sets the processor decomposition in Y", "0"},
        {   "arg.sizey",        "Sets the processor decomposition in X", "0"},
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
	EmberHalo2DGenerator(SST::ComponentId_t id, Params& params);
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ);
	void completed(const SST::Output* output, uint64_t );

private:
	uint32_t m_loopIndex;

	uint32_t sizeX;
	uint32_t sizeY;
	uint32_t nsCompute;
	uint32_t nsCopyTime;
	uint32_t messageSizeX;
	uint32_t messageSizeY;
	uint32_t iterations;
	uint32_t messageCount;

	bool sendWest;
	bool sendEast;
	bool sendNorth;
	bool sendSouth;

	int32_t  procEast;
	int32_t  procWest;
	int32_t  procNorth;
	int32_t  procSouth;

};

}
}

#endif
