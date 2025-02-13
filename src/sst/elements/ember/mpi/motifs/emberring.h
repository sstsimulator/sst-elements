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


#ifndef _H_EMBER_RING
#define _H_EMBER_RING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberRingGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberRingGenerator,
        "ember",
        "RingMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Ring Motif",
        SST::Ember::EmberRingGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messagesize",      "Sets the size of the message in bytes",        "1024"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
    )

	EmberRingGenerator(SST::ComponentId_t id, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    MessageRequest  m_req[2];
	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint32_t m_loopIndex;
    MessageResponse m_resp;
    void*    m_sendBuf;
    void*    m_recvBuf;
    uint64_t m_startTime;
    uint64_t m_stopTime;
};

}
}

#endif
