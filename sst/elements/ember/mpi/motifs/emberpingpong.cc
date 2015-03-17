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
#include "emberpingpong.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberPingPongGenerator::EmberPingPongGenerator(SST::Component* owner, 
                                                    Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_loopIndex(0) 
{
    m_name = "PingPong";

	m_messageSize = (uint32_t) params.find_integer("arg.messageSize", 1024);
	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
}

bool EmberPingPongGenerator::generate( std::queue<EmberEvent*>& evQ)
{ 
    if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank()) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;

            double latency = ((totalTime/m_iterations)/2);
            double bandwidth = (double) m_messageSize / latency;

            m_output->output("%s: total time %.3f us, loop %d, bufLen %d"
                    ", latency %.3f us. bandwidth %f GB/s\n",
                                m_name.c_str(),
                                totalTime * 1000000.0, m_iterations,
                                m_messageSize,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        GEN_DBG( 1, "rank=%d size=%d\n", rank(), size());

        if ( 0 == rank() ) {
            enQ_getTime( evQ, &m_startTime );
        }
    }

    if ( 0 == rank()) {
        enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, 1, 
                                                TAG, GroupWorld );
        enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, 1,
                                                TAG, GroupWorld, &m_resp );
	} else if ( 1 == rank()) {
		enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, 0, 
                                                TAG, GroupWorld, &m_resp );
        enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, 0,
                                                TAG, GroupWorld );
	}

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
