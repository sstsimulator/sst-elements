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


#include <sst_config.h>
#include "emberTrafficGen.h"
#include <sst/core/rng/marsaglia.h>

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberTrafficGenGenerator::EmberTrafficGenGenerator(SST::ComponentId_t id,
                                                    Params& params) :
	EmberMessagePassingGenerator(id, params, "TrafficGen")
{
	m_messageSize = (uint32_t) params.find("arg.messageSize", 1024);

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);

    m_mean = params.find("arg.mean", 5000.0);
    m_stddev = params.find("arg.stddev", 300.0 );
    m_startDelay = params.find("arg.startDelay", .0 );

	configure();
}

void EmberTrafficGenGenerator::configure()
{
    assert( 2 == size() );

    m_random = new SSTGaussianDistribution( m_mean, m_stddev,
                        //new RNG::MarsagliaRNG( 11 + rank(), 79  ) );
                        new RNG::MarsagliaRNG( 11 + rank(), getJobId()  ) );

    if ( 0 == rank() ) {
        verbose(CALL_INFO, 1, 0, "startDelay %.3f ns\n",m_startDelay);
        verbose(CALL_INFO, 1, 0, "compute time: mean %.3f ns,"
        " stdDev %.3f ns\n", m_random->getMean(), m_random->getStandardDev());
        verbose(CALL_INFO, 1, 0, "messageSize %d\n", m_messageSize);
    }
}

bool EmberTrafficGenGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    double computeTime = m_random->getNextDouble();

    if ( computeTime < 0 ) {
        computeTime = 0.0;
    }
    verbose(CALL_INFO, 1, 0, "computeTime=%.3f ns\n", computeTime );
    enQ_compute( evQ, (computeTime + m_startDelay) * 1000 );
    m_startDelay = 0;

    int other = (rank() + 1) % 2;
    enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, other, TAG,
                                                GroupWorld, &m_req );
    enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, other, TAG, GroupWorld );
    enQ_wait( evQ, &m_req );

    return false;
}
