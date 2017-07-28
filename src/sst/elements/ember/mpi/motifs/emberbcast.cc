// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include "emberbcast.h"

using namespace SST::Ember;

EmberBcastGenerator::EmberBcastGenerator(SST::Component* owner,
                                    Params& params) :
	EmberMessagePassingGenerator(owner, params, "Bcast"),
    m_loopIndex(0)
{
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_count      = (uint32_t) params.find("arg.count", 1);
    m_compute    = (uint32_t) params.find("arg.compute", 0);
	m_root    = (uint32_t) params.find("arg.root", 0);
    m_sendBuf = NULL;
}

bool EmberBcastGenerator::generate( std::queue<EmberEvent*>& evQ) {

    if ( m_loopIndex == m_iterations ) {
printf("%s\n",__func__);
        int typeSize = sizeofDataType(DOUBLE);
        if ( size() - 1 == rank() ) {
            double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
            latency /= 1000000000.0;
            output( "%s: ranks %d, loop %d, bytes %" PRIu32 ", latency %.3f us\n",
                    getMotifName().c_str(), size(), m_iterations, 
                        m_count * typeSize, latency * 1000000.0  );
        }
        return true;
    }

    if ( 0 == m_loopIndex ) {
        enQ_getTime( evQ, &m_startTime );
    }

    enQ_compute( evQ, m_compute );

    enQ_bcast( evQ, m_sendBuf, m_count, DOUBLE, m_root, GroupWorld );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}
