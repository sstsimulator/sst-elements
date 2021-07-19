// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

class EmberNtoMGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberNtoMGenerator,
        "ember",
        "NtoMMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a N to M Motif",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messageSize",      "Sets the message size of the ping pong operation", "1024"},
        {   "arg.iterations",       "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.numRecvBufs",      "Sets the number of preposted recv buffers",   "1"},
        {   "arg.targetRankList",   "Sets the targer ranks", "0"},
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
	EmberNtoMGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    bool source( std::queue<EmberEvent*>& evQ);
    bool target( std::queue<EmberEvent*>& evQ);
    bool findNum( int num, std::string& numList );
	void getTargetRanks( std::string&, std::vector<uint32_t>& );
	enum { Init, Run, Fini } m_phase;
	bool m_target;
    std::vector<MessageRequest>  m_req;
    MessageResponse m_resp;
    void*    m_sendBuf;
    void*    m_recvBuf;
	uint32_t m_currentBuf;
	uint32_t m_currentMsg;
	uint32_t m_totalMsgs;

	uint32_t m_messageSize;
	uint32_t m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint32_t m_loopIndex;
	uint32_t m_currentTarget;

    int      m_numRecvBufs;
	std::vector<uint32_t> m_targetRanks;
};

}
}

#endif
