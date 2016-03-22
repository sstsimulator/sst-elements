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


#ifndef _H_EMBER_COMM
#define _H_EMBER_COMM

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCommGenerator : public EmberMessagePassingGenerator {

public:
	EmberCommGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    MessageResponse m_resp;
    Communicator    m_newComm[2];

    uint32_t m_iterations;
    uint32_t m_messageSize;
    int      m_new_size;
    uint32_t m_new_rank;
    void*    m_sendBuf;
    void*    m_recvBuf;
    int      m_workPhase;
    uint32_t m_loopIndex;
};

}
}

#endif
