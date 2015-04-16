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
#include "emberbcast.h"

using namespace SST::Ember;

EmberBcastGenerator::EmberBcastGenerator(SST::Component* owner,
                                    Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_loopIndex(0)
{
    m_name = "Bcast";

	m_iterations = (uint32_t) params.find_integer("arg.iterations", 1);
	m_count      = (uint32_t) params.find_integer("arg.count", 1);
    m_compute    = (uint32_t) params.find_integer("arg.compute", 0);
	m_root    = (uint32_t) params.find_integer("arg.root", 0);
    m_sendBuf = NULL;
}

bool EmberBcastGenerator::generate( std::queue<EmberEvent*>& evQ) {

    if ( m_loopIndex == m_iterations ) {
        int typeSize = sizeofDataType(DOUBLE);
        if ( size() - 1 == rank() ) {
            double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
            latency /= 1000000000.0;
            m_output->output( "%s: ranks %d, loop %d, bytes %" PRIu32 ", latency %.3f us\n",
                    m_name.c_str(), size(), m_iterations, 
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
