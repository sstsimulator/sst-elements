// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberDetailedStreamGenerator,
        "ember",
        "DetailedStreamMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Stream Motif with detailed model",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "time-Init", "Time spent in Init event",          "ns",  0},
        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
        { "time-Rank", "Time spent in Rank event",          "ns", 0},
        { "time-Size", "Time spent in Size event",          "ns", 0},
        { "time-Send", "Time spent in Recv event",          "ns", 0},
        { "time-Recv", "Time spent in Recv event",          "ns", 0},
        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
        { "time-Isend", "Time spent in Isend event",        "ns", 0},
        { "time-Wait", "Time spent in Wait event",          "ns", 0},
        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
        { "time-Compute", "Time spent in Compute event",    "ns", 0},
        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    )

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
