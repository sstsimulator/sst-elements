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


#ifndef _H_EMBER_NAS_LU
#define _H_EMBER_NAS_LU

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberNASLUGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberNASLUGenerator,
        "ember",
        "NASLUMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a NAS-LU communication motif from 2 (opposite) vertices",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.computetime",      "Sets the compute time per KBA-data block in nanoseconds", "1000"},
        {   "arg.pex",          "Sets the processor array size in X-dimension, 0 means auto-calculate", "0"},
        {   "arg.pey",          "Sets the processor array size in Y-dimension, 0 means auto-calculate", "0"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.nx",           "Sets the problem size in the X-dimension", "50"},
        {   "arg.ny",           "Sets the problem size in the Y-dimension", "50"},
        {   "arg.nz",           "Sets the problem size in the Z-dimension", "50"},
        {   "arg.nzblock",      "Sets the Z-dimensional block size (Nz % Nzblock == 0, default is 1)", "1"},
    )

    EmberNASLUGenerator( SST::ComponentId_t, Params& params );
    void configure() override;
    bool generate( std::queue<EmberEvent*>& evQ ) override;

private:
	uint32_t m_loopIndex;

	int32_t px;
	int32_t py;
	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t nzblock;

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
