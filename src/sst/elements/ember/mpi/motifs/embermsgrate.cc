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
#include "embermsgrate.h"

#define TAG 0xf00dbeef

using namespace SST::Ember;

EmberMsgRateGenerator::EmberMsgRateGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "MsgRate"),
    m_startTime( 0 ),
    m_stopTime( 0 ),
    m_totalTime( 0 ),
    m_totalPostTime( 0 ),
    m_preWaitTime( 0 ),
    m_recvStartTime( 0 ),
    m_recvStopTime( 0 ),
    m_loopIndex( 0 )
{
	m_msgSize    = (uint32_t) params.find("arg.msgSize", 0);
	m_numMsgs    = (uint32_t) params.find("arg.numMsgs", 1);
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
    m_reqs.resize( m_numMsgs );
    m_resp.resize( m_numMsgs );
}

bool EmberMsgRateGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    assert( 2 == size() );

    // note that the first time through start and stop are 0
    m_totalTime += m_stopTime - m_startTime;
    if ( 0 == rank() ) {
        m_totalPostTime += m_preWaitTime - m_startTime;
    } else {
        m_totalPostTime += m_recvStopTime - m_recvStartTime;
    }

    // if are done printout results and exit
    if ( m_loopIndex == m_iterations  ) {
        int totalMsgs = m_numMsgs * m_iterations;
        double tmp = (double) m_totalTime / 1000000000.0;
        double postingLat = m_totalPostTime / totalMsgs;
        output("MsgRate: %s msgSize %" PRIu32", totalTime %.6f "
                "sec, %.3f msg/sec, %.3f MB/s, %.0f ns/%s\n",
                        0 == rank() ? "Send" : "Recv",
                        m_msgSize,
                        tmp,
                        totalMsgs / tmp,
                        ((double)totalMsgs*m_msgSize/1000000.0)/tmp,
                        postingLat, 0 == rank() ? "isend":"irecv" );

        return true;
    }

    if ( m_loopIndex == 0 && 0 == rank() ) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());
    }


    verbose(CALL_INFO, 1, 0, "%p %p\n",&m_reqs[0],&m_resp[0]);
    if ( 0 == rank() ) {
        enQ_barrier( evQ, GroupWorld );
        enQ_getTime( evQ, &m_startTime );
        for ( unsigned int i = 0; i < m_numMsgs; i++ ) {
            enQ_isend( evQ, NULL, m_msgSize, CHAR, 1, TAG,
                                                GroupWorld, &m_reqs[i] );
        }
        enQ_getTime( evQ, &m_preWaitTime );

        enQ_waitall( evQ, m_numMsgs, &m_reqs[0],
                                        (MessageResponse**)&m_resp[0] );
        enQ_getTime( evQ, &m_stopTime );
    } else {

        enQ_getTime( evQ, &m_recvStartTime );
        for ( unsigned int i = 0; i < m_numMsgs; i++ ) {
            enQ_irecv( evQ, NULL, m_msgSize, CHAR, 0, TAG,
                                                GroupWorld, &m_reqs[i] );
        }
        enQ_getTime( evQ, &m_recvStopTime );

        enQ_barrier( evQ, GroupWorld );
        enQ_getTime( evQ, &m_startTime );
        enQ_waitall( evQ, m_numMsgs, &m_reqs[0],
                                        (MessageResponse**)&m_resp[0] );
        enQ_getTime( evQ, &m_stopTime );
    }

    if ( ++m_loopIndex == m_iterations ) {
        enQ_compute(evQ,0);
    }

    return false;
}
