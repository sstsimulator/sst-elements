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
#include "embercomm.h"

#define TAG 0xdeadbeef

using namespace SST::Ember;

EmberCommGenerator::EmberCommGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "Comm"),
    m_workPhase(0),
    m_loopIndex(0)
{
    m_messageSize = (uint32_t) params.find_integer("arg.messagesize", 1024);
    m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);

    m_sendBuf = NULL;
    m_recvBuf = NULL;
}

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

bool EmberCommGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
    if ( 0 == m_workPhase ) {
        assert( size() > 2);
        assert( ! ( size() % 2 ) );
        enQ_commSplit( evQ, GroupWorld, rank()/(size()/2), 
                                rank() % (size()/2), &m_newComm );
        m_workPhase = 1;

    } else if ( 1 == m_workPhase ) {
        enQ_size( evQ, m_newComm, &m_new_size );
        enQ_rank( evQ, m_newComm, &m_new_rank );
        m_workPhase = 2;
    } else {
        
        verbose(CALL_INFO, 1, 0, "new size=%d rank=%d\n",
                                            m_new_size, m_new_rank );
        uint32_t to = mod(m_new_rank + 1, m_new_size);
        uint32_t from = mod( (signed int) m_new_rank - 1, m_new_size );
        verbose(CALL_INFO, 1, 0, "to=%d from=%d\n",to,from);

        if(0 == m_new_rank) {
	        enQ_send(evQ, m_sendBuf, m_messageSize, CHAR, to,
                                            TAG, m_newComm);
		    enQ_recv(evQ, m_recvBuf, m_messageSize, CHAR, from,
                                            TAG, m_newComm, &m_resp);
		 } else {
		    enQ_recv(evQ, m_recvBuf, m_messageSize, CHAR, from,
                                            TAG, m_newComm, &m_resp);
		    enQ_send(evQ, m_sendBuf, m_messageSize, CHAR, to,
                                            TAG, m_newComm);
	    }

        if ( ++m_loopIndex == m_iterations ) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}
