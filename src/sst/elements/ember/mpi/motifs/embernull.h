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


#ifndef _H_EMBER_NULL
#define _H_EMBER_NULL

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberNullGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberNullGenerator,
        "ember",
        "NullMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs an idle on the node, no traffic can be generated.",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

	EmberNullGenerator(SST::ComponentId_t id, Params& params) :
		EmberMessagePassingGenerator(id, params, "Null" )
	{ }

    bool generate( std::queue<EmberEvent*>& evQ)
	{
		return true;
	}
};

}
}

#endif
