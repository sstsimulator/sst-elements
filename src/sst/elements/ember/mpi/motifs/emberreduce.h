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


#ifndef _H_EMBER_REDUCE_MOTIF
#define _H_EMBER_REDUCE_MOTIF

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberReduceGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberReduceGenerator,
        "ember",
        "ReduceMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a reduction operation with type set to float64 and operation SUM from a user-specified reduction-tree root",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",       "Sets the number of reduce operations to perform",  "1"},
        {   "arg.count",        "Sets the number of elements to reduce",        "1"},
        {   "arg.root",         "Sets the root of the reduction",           "0"},
    )

	EmberReduceGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    uint32_t m_iterations;
    uint32_t m_count;
    void*    m_sendBuf;
    void*    m_recvBuf;
    int      m_redRoot;
    uint32_t m_loopIndex;
};

}
}

#endif
