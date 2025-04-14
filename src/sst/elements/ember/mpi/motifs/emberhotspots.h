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


#ifndef _H_EMBER_TRAFFIC_GEN
#define _H_EMBER_TRAFFIC_GEN

#include <sst/core/rng/gaussian.h>
#include <sst/core/rng/marsaglia.h>

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHotSpotsGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberHotSpotsGenerator,
        "ember",
        "HotSpotsMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Models network traffic with hot spots",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.messageSize",    "Sets the size of exchange",    "1"},
        { "arg.mean",   "Sets the mean time between exchange",  "1"},
        { "arg.stddev", "Sets the stddev of time between exchange",     "1"},
        { "arg.startDelay", "Sets the stddev of time between exchange",     "1"},
    )

    EmberHotSpotsGenerator(SST::ComponentId_t, Params& params);
    bool generate( std::queue<EmberEvent*>& evQ);
    bool primary( ) {
        return true;
    }
    void recv_stopping();
    void recv_allstopped();
    void recv_data();
    void send_data();
    void accumulate_data();
    void delay();
    void wait_for_any();
    void get_total_bytes();
    bool check_stop();
    void check_finish();
    bool start_final_recvs();
    bool check_termination();

private:
    std::string m_pattern;
    int m_rank;
    int m_commSize;
    uint32_t m_messageSize;
    uint32_t m_maxMessageSize;
    void* m_sendBuf;
    void* m_recvBuf;
    MessageRequest m_req;

    // original "plusOne" pattern
    double  m_startDelay;
    double  m_mean;
    double  m_stddev;
    SSTGaussianDistribution* m_random;
    enum {DATA, STOPPING, ALLSTOPPED};
    enum {STOP_REQUEST, RECV_REQUEST, SEND_REQUEST};
    std::queue<EmberEvent*>* evQ_;
    bool m_dataSendActive;
    bool m_needToWait;
    bool m_finishing;
    bool m_finalRecvs;
    bool m_finished;
    int m_requestIndex;
    unsigned int m_generateLoopIndex;
    unsigned int m_iterations;
    unsigned int m_currentIteration;
    unsigned int m_numStopped;
    unsigned int m_numFinalRecvs;
    unsigned int m_debug;
    Hermes::MemAddr m_rankBytes;
    Hermes::MemAddr m_totalBytes;
    Hermes::MemAddr m_rankSends;
    Hermes::MemAddr m_reducedSends;
    MessageRequest m_requests[3];
    MessageResponse m_anyResponse;
    uint64_t m_dataSize;
    uint64_t m_delay;
    uint64_t m_startTime;
    uint64_t m_currentTime;
    uint64_t m_stopTime;
    uint64_t m_stopTimeActual;
    uint64_t m_numRecv;
    unsigned int m_hotSpots;
    unsigned int m_hotSpotsRatio;
    unsigned int m_hotCounter;
    unsigned int m_hotCounterInitial;
    std::vector<unsigned int> m_hotRanks;
    std::set<unsigned int> m_hotRanks_set;
    double  m_meanMessageSize;
    double  m_stddevMessageSize;
    SSTGaussianDistribution* m_distMessageSize;
    double  m_meanComputeDelay;
    double  m_stddevComputeDelay;
    SSTGaussianDistribution* m_distComputeDelay;
    SST::RNG::MarsagliaRNG* m_distPartner;
};

}
}

#endif
