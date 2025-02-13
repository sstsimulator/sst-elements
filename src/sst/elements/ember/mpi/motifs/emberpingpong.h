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


#ifndef _H_EMBER_PING_PONG
#define _H_EMBER_PING_PONG

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberPingPongGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberPingPongGenerator,
        "ember",
        "PingPongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Ping-Pong Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messageSize",      "Sets the message size of the ping pong operation", "1024"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.rank2",        "Sets the 2nd rank to pingpong with (0 is the 1st)",    "1"},
        {   "arg.blockingSend",     "Sets the send mode",   "1"},
        {   "arg.blockingRecv",     "Sets the recv mode",   "1"},
        {   "arg.waitall",          "Sets the wait mode",   "1"},
    )

	EmberPingPongGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    MessageRequest  m_req;
    MessageResponse m_resp;
    void*    m_sendBuf;
    void*    m_recvBuf;

	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint32_t m_loopIndex;

    int      m_rank2;
    bool     m_blockingSend;
    bool     m_blockingRecv;
    bool     m_waitall;
};

}
}

#endif
