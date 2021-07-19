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
#include "emberincast.h"

using namespace SST::Ember;

#define TAG 0x10101

EmberIncastGenerator::EmberIncastGenerator(SST::ComponentId_t id, Params& params ) :
	EmberMessagePassingGenerator(id, params, "Incast"),
	m_currentItr(0),
	m_incastTarget(0)
{
	m_messageSize = (uint32_t) params.find("arg.messageSize", 1024);
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_incastTarget = params.find<int>( "arg.incasttarget", 0 );

	if( rank() == m_incastTarget ) {
	    	m_recvBuf = (char*) memAlloc( m_messageSize * (size() - 1) );
		m_req     = new MessageRequest[size()-1];
	} else {
    		m_sendBuf = (char*) memAlloc( m_messageSize );
		m_req     = nullptr;
	}
}

bool EmberIncastGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	if( m_currentItr == m_iterations ) {
		return true;
	} else {
		if( rank() == m_incastTarget ) {
			int next_index = 0;

			for( int i = 0; i < size(); ++i ) {
				if( rank() != i ) {
					enQ_irecv( evQ, &m_recvBuf[next_index * m_messageSize], m_messageSize, CHAR, i,
                                               	TAG, GroupWorld, &m_req[next_index] );
					next_index++;
				}
			}

			verbose(CALL_INFO, 1, 0, "Incast target (rank=%d) is posting wait-all for %d irecv messages.\n", rank(), size() - 1);
			enQ_waitall( evQ, size() - 1, m_req, NULL );
		} else {
			verbose(CALL_INFO, 1, 0, "Non-target rank (%d) posting message to incast target (rank=%d, size=%" PRIu32 ")...\n", rank(),
				m_incastTarget, m_messageSize);
			enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, m_incastTarget, TAG, GroupWorld );
		}
	}

	m_currentItr++;

    	return false;
}
