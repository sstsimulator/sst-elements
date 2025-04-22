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


#ifndef _H_EMBER_ALL_PINGPONG_MOTIF
#define _H_EMBER_ALL_PINGPONG_MOTIF

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberAllPingPongGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberAllPingPongGenerator,
        "ember",
        "AllPingPongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a All Ping Pong Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1024"},
        {   "arg.messageSize",      "Sets the message size of the ping pong operation", "128"},
        {   "arg.computetime",      "Sets the time spent computing some values",        "1000"},
    )

	EmberAllPingPongGenerator( SST::ComponentId_t, Params& params );
    bool generate( std::queue<EmberEvent*>& evQ);

private:
	uint32_t m_loopIndex;
	uint32_t m_iterations;
	uint32_t m_messageSize;
	uint32_t m_computeTime;
    uint64_t m_startTime;
    uint64_t m_stopTime;

    MessageResponse m_resp;
    void*    m_sendBuf;
    void*    m_recvBuf;
};

}
}

#endif
