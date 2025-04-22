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

class EmberTrafficGenGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberTrafficGenGenerator,
        "ember",
        "TrafficGenMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Models network traffic",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.messageSize",    "Sets the size of exchange",    "1"},
        { "arg.mean",   "Sets the mean time between exchange",  "1"},
        { "arg.stddev", "Sets the stddev of time between exchange",     "1"},
        { "arg.startDelay", "Sets the stddev of time between exchange",     "1"},
    )

public:
	EmberTrafficGenGenerator( SST::ComponentId_t, Params& params );
    bool generate( std::queue<EmberEvent*>& evQ ) override;
    bool generate_plusOne( std::queue<EmberEvent*>& evQ );
    bool primary() override
    {
        if (m_pattern == "plusOne")
            return false;
        return true;
    }
    void configure() override;
    void configure_plusOne();

    // extended patterns
    bool generate_random( std::queue<EmberEvent*>& evQ );
    void recv_data();
    void send_data();
    void wait_for_any();
    void recv_stopping();
    void recv_allstopped();
    bool check_stop();

private:
    std::string m_pattern;

    uint32_t m_messageSize;
    uint32_t m_maxMessageSize;
    void* m_sendBuf;
    void* m_recvBuf;
    MessageRequest m_req;

    // original "plusOne" pattern
    //    MessageResponse m_resp;
    double  m_startDelay;
    double  m_mean;
    double  m_stddev;
    SSTGaussianDistribution* m_random;

    // extended patterns
    enum {DATA, STOPPING, ALLSTOPPED};
    std::queue<EmberEvent*>* evQ_;
    bool m_dataSendActive;
    bool m_dataRecvActive;
    bool m_needToWait;
    bool m_stopped;
    unsigned int m_generateLoopIndex;
    unsigned int m_iterations;
    unsigned int m_currentIteration;
    unsigned int m_numStopped;
    Hermes::MemAddr m_rankBytes;
    Hermes::MemAddr m_totalBytes;
    Hermes::MemAddr m_allStopped;
    MessageRequest* m_dataSendRequest;
    MessageRequest* m_dataRecvRequest;
    MessageRequest m_stopRequest;
    MessageRequest* m_allRequests;
    MessageResponse m_anyResponse;
    uint32_t m_rank;
    uint32_t m_hotSpots;
    uint32_t m_hotSpotsRatio;
    std::vector<uint32_t> m_hotRanks;
    std::set<uint32_t> m_hotRanks_set;
    uint32_t m_hotCounter;
    uint32_t m_hotCounterInitial;
    uint32_t m_debug;
    uint64_t m_dataSize;
    uint64_t m_computeDelay;
    uint64_t m_startTime;
    uint64_t m_currentTime;
    uint64_t m_stopTime;
    uint64_t m_stopTimeActual;
    int m_requestIndex;
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
