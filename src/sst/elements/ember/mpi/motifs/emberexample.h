// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_EMBER_EXAMPLE
#define _H_EMBER_EXAMPLE

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class ExampleGenerator : public EmberMessagePassingGenerator {
    
public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        ExampleGenerator,
        "ember",
        "ExampleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs an idle cycle on the node, no traffic is generated.",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.messageSize",    "Sets the size of exchange",    "1"},
    );

    SST_ELI_DOCUMENT_STATISTICS(
    );

    ExampleGenerator(SST::ComponentId_t id, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
};

}
}

#endif /* _H_EMBER_EXAMPLE */
