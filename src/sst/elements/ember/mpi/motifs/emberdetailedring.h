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


#ifndef _H_EMBER_DETAILED_RING
#define _H_EMBER_DETAILED_RING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberDetailedRingGenerator : public EmberMessagePassingGenerator {

public:
	EmberDetailedRingGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
	std::string getComputeModelName(); 

private:
    void computeDetailed( std::queue<EmberEvent*>& evQ);
    void computeSimple( std::queue<EmberEvent*>& evQ);
    void (EmberDetailedRingGenerator::*m_computeFunc)( std::queue<EmberEvent*>& evQ );
    
    MessageRequest  m_req[2];
	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint32_t m_loopIndex;
	uint32_t m_stream_n;
	uint32_t m_printRank;
    uint64_t m_computeTime;
    uint64_t m_computeWindow;
    MessageResponse m_resp;
    Hermes::MemAddr    m_sendBuf;
    Hermes::MemAddr    m_recvBuf;
    Hermes::MemAddr    m_streamBuf;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_startCompute;
    uint64_t m_stopCompute;
    uint32_t m_doCompute;
};

}
}

#endif
