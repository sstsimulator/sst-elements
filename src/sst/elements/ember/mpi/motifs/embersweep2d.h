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


#ifndef _H_EMBER_SWEEP_2D
#define _H_EMBER_SWEEP_2D

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberSweep2DGenerator : public EmberMessagePassingGenerator {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberSweep2DGenerator,
        "ember",
        "Sweep2DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a 2D sweep exchange Motif with multiple vertex communication ordering",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.computetime",      "Sets the compute time per KBA-data block in nanoseconds", "1000"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.nx",           "Sets the problem size in the X-dimension", "50"},
        {   "arg.ny",           "Sets the problem size in the Y-dimension", "50"},
        {   "arg.y_block",      "Sets the Y-blocking factor (must be Ny % y_block == 0, default is 1 (no blocking))", "1"},
    )

    EmberSweep2DGenerator( SST::ComponentId_t, Params& params );
    void configure() override;
    bool generate( std::queue<EmberEvent*>& evQ ) override;

private:
	uint32_t m_loopIndex;

	int32_t  x_up;
	int32_t  x_down;

	uint32_t nx;
	uint32_t ny;
	uint32_t y_block;

	uint32_t nsCompute;
	uint32_t iterations;
};

}
}

#endif
