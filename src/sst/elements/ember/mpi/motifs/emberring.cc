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
#include "emberring.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF
#define DATA_TYPE CHAR

EmberRingGenerator::EmberRingGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Ring"),
    m_loopIndex(0)
{
	m_messageSize = (uint32_t) params.find("arg.messagesize", 1024);
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
}

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

bool EmberRingGenerator::generate( std::queue<EmberEvent*>& evQ)
{
   if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank()) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;

			size_t nBytes = m_messageSize * sizeofDataType(DATA_TYPE);
            double latency = ((totalTime/m_iterations)/size());
            double bandwidth = (double) nBytes / latency;

            output("%s total time %.3f us, loop %d, bufLen %zu"
                    ", latency %.3f us. bandwidth %f GB/s\n",
                                getMotifName().c_str(),
                                totalTime * 1000000.0, m_iterations,
								nBytes,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());
    	m_sendBuf = memAlloc(m_messageSize * sizeofDataType(DATA_TYPE) );
    	m_recvBuf = memAlloc(m_messageSize * sizeofDataType(DATA_TYPE) );

        if ( 0 == rank() ) {
            enQ_getTime( evQ, &m_startTime );
        }
    }

    int to = mod( rank() + 1, size());
    int from = mod( (signed int) rank() - 1, size() );
    verbose( CALL_INFO, 2, 0, "to=%d from=%d\n",to,from);

	if ( m_loopIndex ) {
		int count;
		get_count( &m_resp, CHAR, &count );
		get_count( &m_resp, INT, &count );
		get_count( &m_resp, LONG, &count );
	}
    if ( 0 == rank() ) {
        enQ_send( evQ, m_sendBuf, m_messageSize, DATA_TYPE, to, TAG,
                                                GroupWorld );
	    enQ_recv( evQ, m_recvBuf, m_messageSize, DATA_TYPE, from, TAG,
                                                GroupWorld, &m_resp );
    } else {
	    enQ_recv( evQ, m_recvBuf, m_messageSize, DATA_TYPE, from, TAG,
                                                GroupWorld, &m_resp );
	    enQ_send( evQ, m_sendBuf, m_messageSize, DATA_TYPE, to, TAG,
                                                GroupWorld );
    }

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
