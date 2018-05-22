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


#include <sst_config.h>
#include "emberpingpong.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberPingPongGenerator::EmberPingPongGenerator(SST::Component* owner, 
                                            			Params& params ) :
	EmberMessagePassingGenerator(owner, params, "PingPong"),
    m_loopIndex(0),
    m_rank2(1),
    m_blockingSend( true ),
    m_blockingRecv( true ),
    m_waitall( false )
{
	m_messageSize = (uint32_t) params.find("arg.messageSize", 1024);
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_rank2 = (uint32_t) params.find("arg.rank2", 1);

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
    m_blockingSend = (uint32_t) params.find("arg.blockingSend", true);;
    m_blockingRecv = (uint32_t) params.find("arg.blockingRecv", true);;
    m_waitall = (uint32_t) params.find("arg.waitall", false);

    if ( 0 == rank() )  {
        output("rank2=%d messageSize=%d iterations=%d\n",m_rank2, m_messageSize, m_iterations);
    }
    if ( ! ( 0 == rank() || m_rank2 == rank() )  ) {
        m_loopIndex = m_iterations;
    } 
}

bool EmberPingPongGenerator::generate( std::queue<EmberEvent*>& evQ)
{ 
    if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank()) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;

            double latency = ((totalTime/m_iterations)/2);
            double bandwidth = (double) m_messageSize / latency;

            output("%s: otherRank %d, total time %.3f us, loop %d, bufLen %d"
                    ", latency %.3f us. bandwidth %f GB/s\n",
                                getMotifName().c_str(), m_rank2,
                                totalTime * 1000000.0, m_iterations,
                                m_messageSize,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());

        if ( 0 == rank() ) {
            enQ_getTime( evQ, &m_startTime );
        }
    }

    if ( 0 == rank()) {
    
        if ( m_blockingSend ) {
            enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, m_rank2, 
                                                TAG, GroupWorld );
        } else {
            enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, m_rank2, 
                                                TAG, GroupWorld, &m_req );
            if ( m_waitall ) {
                enQ_waitall( evQ, 1, &m_req, (MessageResponse**)&m_resp );
            } else {
                enQ_wait( evQ, &m_req );
            }
        }
        if ( m_blockingRecv ) {
            enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, m_rank2,
                                                TAG, GroupWorld, &m_resp );
        } else {
            enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, m_rank2,
                                                TAG, GroupWorld, &m_req );
            if ( m_waitall ) {
                enQ_waitall( evQ, 1, &m_req, (MessageResponse**)&m_resp );
            } else {
                enQ_wait( evQ, &m_req );
            }
        } 
	} else if ( m_rank2 == rank()) {
        if ( m_blockingRecv ) {
            enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, 0,
                                                TAG, GroupWorld, &m_resp );
        } else {
		    enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, 0, 
                                                TAG, GroupWorld, &m_req );
            if ( m_waitall ) {
                enQ_waitall( evQ, 1, &m_req, (MessageResponse**)&m_resp );
            } else {
                enQ_wait( evQ, &m_req );
            }
        }
        if ( m_blockingSend ) {
            enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, 0,
                                                TAG, GroupWorld );
        } else {
            enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, 0,
                                                TAG, GroupWorld, &m_req );
            if ( m_waitall ) {
                enQ_waitall( evQ, 1, &m_req, (MessageResponse**)&m_resp );
            } else {
                enQ_wait( evQ, &m_req );
            }
        }
	}

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
