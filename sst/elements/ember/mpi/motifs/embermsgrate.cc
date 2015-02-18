// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "embermsgrate.h"

#define TAG 0xf00dbeef

using namespace SST::Ember;

EmberMsgRateGenerator::EmberMsgRateGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_startTime( 0 ),
    m_stopTime( 0 ),
    m_totalTime( 0 ),
    m_loopIndex( 0 )
{
    m_name = "MsgRate";

	m_msgSize    = (uint32_t) params.find_integer("arg.msgSize", 0);
	m_numMsgs    = (uint32_t) params.find_integer("arg.numMsgs", 1);
	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
    m_reqs.resize( m_numMsgs );
    m_resp.resize( m_numMsgs );
}

bool EmberMsgRateGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    assert( 2 == size() );

    // note that the first time through start and stop are 0
    m_totalTime += m_stopTime - m_startTime;

    // if are done printout results and exit 
    if ( m_loopIndex == m_iterations  ) {
        int totalMsgs = m_numMsgs * m_iterations;
        double tmp = (double) m_totalTime / 1000000000.0;
        m_output->output("MsgRate: %s msgSize %" PRIu32", totalTime %.6f "
                "sec, %.3f msg/sec, %.3f MB/s\n",
                        0 == rank() ? "Send" : "Recv",
                        m_msgSize,
                        tmp,
                        totalMsgs / tmp,
                        ((double)totalMsgs*m_msgSize/1000000.0)/tmp );

        return true;
    }

    if ( m_loopIndex == 0 && 0 == rank() ) {
        GEN_DBG( 1, "rank=%d size=%d\n", rank(), size());
    }


    GEN_DBG( 1, "%p %p\n",&m_reqs[0],&m_resp[0]);
    if ( 0 == rank() ) {
        enQ_barrier( evQ, GroupWorld );
        enQ_getTime( evQ, &m_startTime ); 
        for ( unsigned int i = 0; i < m_numMsgs; i++ ) {
            enQ_isend( evQ, NULL, m_msgSize, CHAR, 1, TAG,
                                                GroupWorld, &m_reqs[i] );
        }

        enQ_waitall( evQ, m_numMsgs, &m_reqs[0],
                                        (MessageResponse**)&m_resp[0] ); 
        enQ_getTime( evQ, &m_stopTime ); 
    } else {

        for ( unsigned int i = 0; i < m_numMsgs; i++ ) {
            enQ_irecv( evQ, NULL, m_msgSize, CHAR, 0, TAG, 
                                                GroupWorld, &m_reqs[i] );
        }

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
