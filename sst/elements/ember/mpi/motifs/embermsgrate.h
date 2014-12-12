// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_MSGRATE
#define _H_EMBER_MSGRATE

#include "embermpigen.h"

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
    uint32_t m_loopIndex;

    std::vector<MessageRequest>     m_reqs;
    std::vector<MessageResponse>    m_resp;
};

}
}

#endif
