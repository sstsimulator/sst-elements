// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberring.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberRingGenerator::EmberRingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "Ring"),
    m_loopIndex(0)
{
	m_messageSize = (uint32_t) params.find("arg.messagesize", 1024);
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_computeTime = (uint64_t) params.find("arg.computeTime", 0);
	m_printRank = params.find<uint32_t>("arg.printRank", 0 );
    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
    
}

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

bool EmberRingGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
   if ( m_loopIndex == m_iterations ) {
        if ( rank() == m_printRank || -1 == m_printRank ) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;

            double latency = ((totalTime/m_iterations)/size());
            double bandwidth = (double) m_messageSize / latency;

            output("%s total time %.3f us, loop %d, bufLen %d"
                    ", latency %.3f us. bandwidth %f GB/s\n", 
                                getMotifName().c_str(),
                                totalTime * 1000000.0, m_iterations,
                                m_messageSize,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );
            output("%s: compute time %" PRIu64" ns\n",
                                getMotifName().c_str(), m_computeTime);
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());

        enQ_getTime( evQ, &m_startTime );
    }

    int to = mod( rank() + 1, size());
    int from = mod( (signed int) rank() - 1, size() );
    verbose( CALL_INFO, 2, 0, "to=%d from=%d\n",to,from);

    if ( 0 == rank() ) {
        enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld, &m_req[0] );
        enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG,
                                                GroupWorld, &m_req[1] );
    } else {
        enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG,
                                                GroupWorld, &m_req[0] );
        enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld, &m_req[1] );
    }

    enQ_compute( evQ, m_computeTime );

    enQ_wait( evQ, &m_req[0] );
    enQ_wait( evQ, &m_req[1] );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
