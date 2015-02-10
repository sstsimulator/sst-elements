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
#include "emberTrafficGen.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberTrafficGenGenerator::EmberTrafficGenGenerator(SST::Component* owner, 
                                                    Params& params) :
	EmberMessagePassingGenerator(owner, params)
{
    m_name = "TrafficGen";

	m_messageSize = (uint32_t) params.find_integer("arg.messageSize", 1024);
	m_computeTime = (uint64_t) params.find_integer("arg.computeTime", 100000);

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize);
}

bool EmberTrafficGenGenerator::generate( std::queue<EmberEvent*>& evQ)
{ 
    enQ_compute( evQ, m_computeTime );
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

    return false;
}
