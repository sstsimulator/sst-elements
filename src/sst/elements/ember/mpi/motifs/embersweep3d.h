// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
    SST_ELI_REGISTER_SUBCOMPONENT(
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

    EmberSweep3DGenerator( SST::ComponentId_t id, Params& params );
    void configure() override;
    bool generate( std::queue<EmberEvent*>& evQ ) override;

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
