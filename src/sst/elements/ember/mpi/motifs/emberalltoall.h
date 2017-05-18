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


#ifndef _H_EMBER_ALLTOALL_MOTIF
#define _H_EMBER_ALLTOALL_MOTIF

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberAlltoallGenerator : public EmberMessagePassingGenerator {

public:
	EmberAlltoallGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_compute;
	uint32_t m_iterations;
	uint32_t m_bytes;
    void*    m_sendBuf;
    void*    m_recvBuf;
    uint32_t m_loopIndex;
    int jobId; //NetworkSim
};

}
}

#endif
