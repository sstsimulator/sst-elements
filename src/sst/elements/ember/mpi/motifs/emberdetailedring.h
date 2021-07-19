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


#ifndef _H_EMBER_DETAILED_RING
#define _H_EMBER_DETAILED_RING

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberDetailedRingGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberDetailedRingGenerator,
        "ember",
        "DetailedRingMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a Ring Motif with detailed model",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.messagesize",     "Sets the size of the message in bytes",        "1024"},
        {   "arg.iterations",      "Sets the number of ping pong operations to perform",   "1"},
        {   "arg.stream_n",        "Sets the size number of items to stream",   "1000"},
        {   "arg.printRank",       "Sets the rank that prints results",   "0"},
        {   "arg.detailedCompute", "Sets the list or ranks that do detailed compute",   ""},
        {   "arg.computeTime",     "Sets the compute time",   "0"},
        {   "arg.computeWindow",   "Sets the maximum compute time",   "0"},
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
	EmberDetailedRingGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
	std::string getComputeModelName();

private:
    void computeDetailed( std::queue<EmberEvent*>& evQ);
    void computeSimple( std::queue<EmberEvent*>& evQ);
    void (EmberDetailedRingGenerator::*m_computeFunc)( std::queue<EmberEvent*>& evQ );
    bool findNum( int num, std::string list );

    MessageRequest  m_req[2];
	std::string m_rankList;
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
