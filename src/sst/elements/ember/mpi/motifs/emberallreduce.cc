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
#include "emberallreduce.h"

using namespace SST::Ember;

static void test(void* a, void* b, int* len, PayloadDataType* ) {

	printf("%s() len=%d\n",__func__,*len);
}

EmberAllreduceGenerator::EmberAllreduceGenerator(SST::ComponentId_t id,
                                            Params& params) :
	EmberMessagePassingGenerator(id, params, "Allreduce"),
    m_loopIndex(0)
{

	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_compute    = (uint32_t) params.find("arg.compute", 0);
	m_count      = (uint32_t) params.find("arg.count", 1);
	if ( params.find<bool>("arg.doUserFunc", false )  )   {
		m_op = op_create( test, 0 );
	} else {
		m_op = Hermes::MP::SUM;
	}
}

bool EmberAllreduceGenerator::generate( std::queue<EmberEvent*>& evQ) {

    if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank() ) {
            double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
            latency /= 1000000000.0;
            output( "%s: ranks %d, loop %d, %d double(s), latency %.3f us\n",
                    getMotifName().c_str(), size(), m_iterations, m_count, latency * 1000000.0  );
        }
        return true;
    }
    if ( 0 == m_loopIndex ) {
		memSetBacked();
		m_sendBuf = memAlloc(sizeofDataType(DOUBLE)*m_count);
		m_recvBuf = memAlloc(sizeofDataType(DOUBLE)*m_count);
        enQ_getTime( evQ, &m_startTime );
    }

    enQ_compute( evQ, m_compute );
    enQ_allreduce( evQ, m_sendBuf, m_recvBuf, m_count, DOUBLE, m_op, GroupWorld );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
