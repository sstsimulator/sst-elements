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


#ifndef _H_EMBER_RANDOM_GEN
#define _H_EMBER_RANDOM_GEN

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberRandomTrafficGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberRandomTrafficGenerator,
        "ember",
        "RandomMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a random traffic pattern communication",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messagesize",      "Sets the message size of the communications (in count of DOUBLE)", "1"},
        {   "arg.iterations",       "Sets the number of iterations to perform",     "1"},
    )

	EmberRandomTrafficGenerator(SST::ComponentId_t, Params& params);
    	bool generate( std::queue<EmberEvent*>& evQ);

protected:
	uint32_t maxIterations;
	uint32_t iteration;
	uint32_t msgSize;

};

}
}

#endif
