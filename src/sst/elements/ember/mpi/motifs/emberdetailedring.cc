// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberdetailedring.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberDetailedRingGenerator::EmberDetailedRingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "DetailedRing"),
    m_loopIndex(0)
{
	m_messageSize = params.find<uint32_t>("arg.messagesize", 1024);
	m_iterations = params.find<int32_t>("arg.iterations", 1);
    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
}

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

bool EmberDetailedRingGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
   if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank()) {
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
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());

        if ( 0 == rank() ) {
            enQ_getTime( evQ, &m_startTime );
        }
    }

    if ( haveDetailed() ) {
        verbose( CALL_INFO, 1, 0, "\n");
        Params params;

        std::string motif;

        motif = "miranda.CopyGenerator";
        params.insert("read_start_address", "0",true);
        params.insert("request_width", "16",true);
        params.insert("request_count", "65536",true);
        enQ_detailedCompute( evQ, motif, params );

        motif = "miranda.SingleStreamGenerator";
        params.insert("startat", "3",true);
        params.insert("count", "500000",true);
        params.insert("max_address", "512000",true);

        enQ_detailedCompute( evQ, motif, params );
    }

    int to = mod( rank() + 1, size());
    int from = mod( (signed int) rank() - 1, size() );
    verbose( CALL_INFO, 2, 0, "to=%d from=%d\n",to,from);

    if ( 0 == rank() ) {
        enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld );
	    enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG, 
                                                GroupWorld, &m_resp );
    } else {
	    enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG, 
                                                GroupWorld, &m_resp );
	   enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld );
    }

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
