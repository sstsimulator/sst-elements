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


#ifndef _H_EMBER_FINI
#define _H_EMBER_FINI

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberFiniGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberFiniGenerator,
        "ember",
        "FiniMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a communication finalize Motif",
        SST::Ember::EmberFiniGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    EmberFiniGenerator(SST::ComponentId_t id, Params& params) :
        EmberMessagePassingGenerator(id, params, "Fini")
    { }

    bool generate( std::queue<EmberEvent*>& evQ)
    {
        verbose(CALL_INFO, 2, 0, "\n" );
        enQ_fini( evQ );
        return true;
    }

    void completed(const SST::Output* output, uint64_t time ) {
        EmberMessagePassingGenerator::completed(output,time);
    }
};

}
}

#endif
