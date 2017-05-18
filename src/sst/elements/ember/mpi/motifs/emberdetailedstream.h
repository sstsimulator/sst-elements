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


#ifndef _H_EMBER_DETAILED_STREAM
#define _H_EMBER_DETAILED_STREAM

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberDetailedStreamGenerator : public EmberMessagePassingGenerator {

public:
	EmberDetailedStreamGenerator(SST::Component* owner, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
	std::string getComputeModelName(); 

private:
	//enum Bench { COPY, TRIAD, NUM_BENCH }  m_bench;
    void computeDetailedCopy( std::queue<EmberEvent*>& evQ);
    void computeDetailedTriad( std::queue<EmberEvent*>& evQ);
	void print();
    
	uint32_t m_numLoops;
	uint32_t m_loopIndex;

	uint32_t m_stream_n;
	uint32_t m_n_per_call_triad;
	uint32_t m_n_per_call_copy;
	uint32_t m_operandwidth;

	std::vector<std::string> 	m_benchName;
	std::vector<size_t>      	m_benchSize;
    std::vector<uint64_t> 		m_startTime;
    std::vector<uint64_t> 		m_stopTime;
    Hermes::MemAddr    			m_streamBuf;

};

}
}

#endif
