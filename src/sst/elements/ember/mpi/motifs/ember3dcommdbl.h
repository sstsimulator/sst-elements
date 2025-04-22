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
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "pex",      "Sets the processors in the X-dimension", "1"   },
        {   "pey",      "Sets the processors in the Y-dimension", "1"   },
        {   "pez",      "Sets the processors in the Z-dimension", "1"   },
        {   "basephase",        "Starts the phase at offset.", "0" },
        {   "items_per_node",   "Sets the number of items to exchange between nodes per phase", "100" },
        {   "compute_at_step",  "Sets the computation time in between each communication phase in nanoseconds", "1000"  },
    )

public:
    Ember3DCommDoublingGenerator( SST::ComponentId_t, Params& params );
    ~Ember3DCommDoublingGenerator() {}
    void configure() override;
    bool generate( std::queue<EmberEvent*>& evQ ) override;
    int32_t power3( const uint32_t expon );

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
