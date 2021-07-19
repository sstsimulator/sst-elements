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
#include "emberscatterv.h"

using namespace SST::Ember;

EmberScattervGenerator::EmberScattervGenerator(SST::ComponentId_t id,
                                    Params& params) :
	EmberMessagePassingGenerator(id, params, "Scatterv"),
    m_loopIndex(0)
{
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_count      = (uint32_t) params.find("arg.count", 1);
    m_compute    = (uint32_t) params.find("arg.compute", 0);
	m_root    = (uint32_t) params.find("arg.root", 0);
	memSetBacked();
    m_sendBuf = memAlloc( m_count * sizeofDataType(LONG) * size() );
    m_recvBuf = memAlloc( m_count * sizeofDataType(LONG) );

	if ( m_root == rank() ) {
		m_sendCnts.resize( size() );
		m_sendDsp.resize( size() );
		for ( int i = 0; i < size(); i++ ) {
    		for ( int j = 0; j < m_count; j++ ) {
				int index = i * m_count + j;

        		((long*)m_sendBuf)[index] = i;
			}
			m_sendCnts[i] = m_count;
			m_sendDsp[i] = i * m_count * sizeof(long);
    	}
	}
}


bool EmberScattervGenerator::generate( std::queue<EmberEvent*>& evQ) {

    if ( m_loopIndex == m_iterations ) {
        int typeSize = sizeofDataType(LONG);
        if ( size() - 1 == rank() ) {
            double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
            latency /= 1000000000.0;
            output( "%s: ranks %d, loop %d, bytes %" PRIu32 ", latency %.3f us\n",
                    getMotifName().c_str(), size(), m_iterations,
                        m_count * typeSize, latency * 1000000.0  );
        }

		if ( m_root != rank() ) {
    		for ( int i = 0; i < m_count; i++ ) {
				if ( ((long*)m_recvBuf)[i] != rank() ) {
					printf("Error: Rank %d index %d verification failed, want %d got %ld\n", rank(), i, rank(), ((long*)m_recvBuf)[i] );
				}
			}
		}
        return true;
    }

    if ( 0 == m_loopIndex ) {
        enQ_getTime( evQ, &m_startTime );
    }

    enQ_compute( evQ, m_compute );

    enQ_scatterv( evQ, m_sendBuf, m_sendCnts.data(), m_sendDsp.data(), LONG, m_recvBuf, m_count, LONG, m_root, GroupWorld );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
