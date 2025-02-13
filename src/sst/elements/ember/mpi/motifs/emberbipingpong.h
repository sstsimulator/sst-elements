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


#ifndef _H_EMBER_BIPINGPONG
#define _H_EMBER_BIPINGPONG

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberBiPingPongGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberBiPingPongGenerator,
        "ember",
        "BiPingPongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a InOut Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messageSize",      "Sets the message size of the operation",   "1024"},
        {   "arg.iterations",       "Sets the number of operations to perform",     "1"},
    )

	EmberBiPingPongGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    void*    m_sendBuf;
    void*    m_recvBuf;
    MessageRequest m_req;

	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint32_t m_loopIndex;
};

}
}

#endif
