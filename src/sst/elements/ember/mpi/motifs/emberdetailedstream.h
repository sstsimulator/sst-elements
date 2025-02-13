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


#ifndef _H_EMBER_DETAILED_STREAM
#define _H_EMBER_DETAILED_STREAM

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberDetailedStreamGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberDetailedStreamGenerator,
        "ember",
        "DetailedStreamMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Stream Motif with detailed model",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
       {   "arg.stream_n",         "Sets the size number of items to stream",   "1024"},
       {   "arg.n_per_call_copy",  "Sets the size of the copy ",   "1000"},
       {   "arg.n_per_call_triad", "Sets the size of the triad",   "1000"},
       {   "arg.operandwidth",     "Sets the operand width",   "8"},
       {   "arg.n_loops",          "Sets the number of loops",   "2"},
    )

public:
	EmberDetailedStreamGenerator(SST::ComponentId_t, Params& params);
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
