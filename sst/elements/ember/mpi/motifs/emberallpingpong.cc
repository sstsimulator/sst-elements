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
#include "emberallpingpong.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberAllPingPongGenerator::EmberAllPingPongGenerator(SST::Component* owner,
                                            Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_loopIndex(0)
{
    m_name = "PingPong";

	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
	m_messageSize = (uint32_t) params.find_integer("arg.messagesize", 128);
	m_computeTime = (uint32_t) params.find_integer("arg.computetime", 1000);

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
}

bool EmberAllPingPongGenerator::generate( std::queue<EmberEvent*>& evQ)
{
   if ( m_loopIndex == 0 ) {
        
        initOutput();
        GEN_DBG( 1, "rank=%d size=%d\n", rank(), size() );
    }

    enQ_compute( evQ, m_computeTime ); 

    if ( rank() < size()/2 ) {
        enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, 
                    rank() + (size()/2), TAG, GroupWorld );
        enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, 
                    rank() + (size()/2), TAG, GroupWorld, &m_resp );

    } else {
        enQ_recv( evQ, m_recvBuf, m_messageSize, CHAR, 
                    rank() - (size()/2), TAG, GroupWorld, &m_resp );
        enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, 
                    rank() - (size()/2), TAG, GroupWorld );
    }

    if ( ++m_loopIndex == m_iterations ) {
        return true;
    } else {
        return false;
    }

}
