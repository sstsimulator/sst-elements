// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
	EmberPingPongGenerator(SST::Component* owner, Params& params);
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
