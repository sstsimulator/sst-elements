// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_DETAILED_RING
#define _H_EMBER_DETAILED_RING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberDetailedRingGenerator : public EmberMessagePassingGenerator {

public:
	EmberDetailedRingGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    MessageRequest  m_req[2];
	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint32_t m_loopIndex;
    MessageResponse m_resp;
    Hermes::MemAddr    m_sendBuf;
    Hermes::MemAddr    m_recvBuf;
    Hermes::MemAddr    m_streamBuf;
    uint64_t m_startTime;
    uint64_t m_stopTime;
};

}
}

#endif
