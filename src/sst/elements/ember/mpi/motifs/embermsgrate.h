// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_MSGRATE
#define _H_EMBER_MSGRATE

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberMsgRateGenerator : public EmberMessagePassingGenerator {

public:
	EmberMsgRateGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:

    uint32_t m_msgSize;
    uint32_t m_numMsgs;
    uint32_t m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_totalTime;
    uint64_t m_totalPostTime;
    uint64_t m_preWaitTime;
    uint64_t m_recvStartTime;
    uint64_t m_recvStopTime;
    uint32_t m_loopIndex;

    std::vector<MessageRequest>     m_reqs;
    std::vector<MessageResponse>    m_resp;
};

}
}

#endif
