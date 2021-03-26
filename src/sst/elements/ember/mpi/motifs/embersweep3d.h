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


#ifndef _H_EMBER_SWEEP_3D
#define _H_EMBER_SWEEP_3D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberSweep3DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberSweep3DGenerator,
        "ember",
        "Sweep3DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a 3D sweep communication motif from all 8 vertices",
        SST::Ember::EmberSweep3DGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.pex",          "Sets the processor array size in X-dimension, 0 means auto-calculate", "0"},
        {   "arg.pey",          "Sets the processor array size in Y-dimension, 0 means auto-calculate", "0"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.nx",           "Sets the problem size in the X-dimension", "50"},
        {   "arg.ny",           "Sets the problem size in the Y-dimension", "50"},
        {   "arg.nz",           "Sets the problem size in the Z-dimension", "50"},
        {   "arg.kba",          "Sets the KBA (Nz-K blocking factor, default is 1 (no blocking))", "1"},
        {   "arg.datatype_width",           "Sets the width of the datatype used at the cell", "8"},
        {   "arg.fields_per_cell",          "Sets the number of fields at each cell point", "8"},
        {   "arg.flops_per_cell",           "Sets the number of FLOPs per cell", "275" },
        {   "arg.nodeflops",                "Sets the FLOP/s count for the MPI rank", "1000000000"},
        {   "arg.computetime",      "Sets the compute time per nx * ny block in picoseconds", "1000"},
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
	EmberSweep3DGenerator(SST::ComponentId_t id, Params& params);
	void configure();
    bool generate( std::queue<EmberEvent*>& evQ );

private:
    uint32_t m_loopIndex;
    uint8_t  m_InnerLoopIndex;

	int32_t px;
	int32_t py;
	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t kba;
	uint32_t data_width;
	uint32_t fields_per_cell;

	int32_t  x_up;
	int32_t  x_down;
	int32_t  y_up;
	int32_t  y_down;

	uint32_t nsCompute;
	uint32_t iterations;
};

}
}

#endif
