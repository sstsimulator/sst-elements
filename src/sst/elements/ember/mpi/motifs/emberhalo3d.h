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


#ifndef _H_EMBER_HALO_3D_BLOCKING
#define _H_EMBER_HALO_3D_BLOCKING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo3DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberHalo3DGenerator,
        "ember",
        "Halo3DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a 3D blocking motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.nx",           "Sets the problem size in X-dimension",         "100"},
        {   "arg.ny",           "Sets the problem size in Y-dimension",         "100"},
        {   "arg.nz",           "Sets the problem size in Z-dimension",         "100"},
        {   "arg.pex",          "Sets the processors in X-dimension (0=auto)",      "0"},
        {   "arg.pey",          "Sets the processors in Y-dimension (0=auto)",      "0"},
        {   "arg.pez",          "Sets the processors in Z-dimension (0=auto)",      "0"},
        {   "arg.fields_per_cell",  "Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
        {   "arg.doreduce",     "How often to do reduces, 1 = each iteration",      "1"},
        {   "arg.datatype_width",   "Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure byte count is correct.", "8"},
        {   "arg.peflops",      "Sets the FLOP/s rate of the processor (used to calculate compute time if not supplied, default is 10000000000 FLOP/s)", "10000000000"},
        {   "arg.flopspercell",     "Sets the number of number of floating point operations per cell, default is 26 (27 point stencil)",    "26"},
        {   "arg.computetime",      "Sets the number of nanoseconds to compute for",    "10"},
        {   "arg.copytime",     "Sets the time spent copying data between messages",    "5"},
        {   "arg.iterations",       "Sets the number of halo3d operations to perform",  "10"},
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
	EmberHalo3DGenerator(SST::ComponentId_t, Params& params);
	~EmberHalo3DGenerator() {}
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ );

private:
	uint32_t m_loopIndex;

	bool performReduction;

	uint32_t iterations;

	// Share these over all instances of the motif
	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t nsCompute;
	uint32_t nsCopyTime;

	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t items_per_cell;
	uint32_t sizeof_cell;

	int32_t  x_down;
	int32_t  x_up;
	int32_t  y_down;
	int32_t  y_up;
	int32_t  z_down;
	int32_t  z_up;
    int jobId; //NetworkSim
	uint64_t m_startTime;  //NetworkSim
    uint64_t m_stopTime;  //NetworkSim

};

}
}

#endif
