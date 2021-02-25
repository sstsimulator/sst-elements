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
#include "emberallgatherv.h"

using namespace SST::Ember;

EmberAllgathervGenerator::EmberAllgathervGenerator(SST::ComponentId_t id,
                                            Params& params) :
	EmberMessagePassingGenerator(id, params, "Allgatherv"),
    m_loopIndex(0)
{
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_count      = (uint32_t) params.find("arg.count", 1);
    m_verify     = params.find<bool>("arg.verify",true);

    memSetBacked();
    m_sendBuf = memAlloc( m_count * sizeofDataType(INT) );
    m_recvBuf = memAlloc( m_count * sizeofDataType(INT)*  size() );

	for ( int i = 0; i < m_count; i++ ) {
    	((int*)m_sendBuf)[i] = rank();
	}

    m_sendCnt = m_count;
    m_recvCnts.resize(size());
    m_recvDsp.resize(size());

    for ( int i = 0; i < size(); i++ ) {
        m_recvCnts[i] = m_count;
        m_recvDsp[i] =  sizeofDataType(INT) * m_count * i;
    }
}

bool EmberAllgathervGenerator::generate( std::queue<EmberEvent*>& evQ) {

	if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank() ) {
			double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
            latency /= 1000000000.0;
            output( "%s: ranks %d, loop %d, bytes %d, latency %.3f us\n",
                    getMotifName().c_str(), size(), m_iterations, m_count * sizeofDataType(INT) , latency * 1000000.0  );
		}

        if ( m_verify ) {
            for ( int i = 0; i < size(); i++ ) {
            	for ( int j = 0; j < m_count; j++ ) {
					int index = i * m_count + j;
                	if ( i != ((int*)m_recvBuf)[ index ]) {
                    	printf("Error: Rank %d index %d failed  got=%d\n", rank(), i, ((int*)m_recvBuf)[ index ] );
                	}
				}
            }
        }

		return true;
    }

   if ( 0 == m_loopIndex ) {
        enQ_getTime( evQ, &m_startTime );
    }

    enQ_allgatherv( evQ, m_sendBuf, m_sendCnt, INT, m_recvBuf, &m_recvCnts[0], &m_recvDsp[0], INT, GroupWorld );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
