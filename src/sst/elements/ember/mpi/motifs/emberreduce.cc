// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include "emberreduce.h"

using namespace SST::Ember;

EmberReduceGenerator::EmberReduceGenerator(SST::Component* owner,
                                    Params& params) :
	EmberMessagePassingGenerator(owner, params, "Reduce"),
    m_loopIndex(0)
{
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_count      = (uint32_t) params.find("arg.count", 1);
	m_redRoot    = (uint32_t) params.find("arg.root", 0);
    m_sendBuf = NULL;
    m_recvBuf = NULL;

}

bool EmberReduceGenerator::generate( std::queue<EmberEvent*>& evQ) {
    if ( 0 == m_loopIndex ) {
        verbose(CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());
    }

    enQ_compute( evQ, 11000 );
    enQ_reduce( evQ, m_sendBuf, m_recvBuf, m_count, DOUBLE,
                                                 SUM, m_redRoot, GroupWorld );
    if ( ++m_loopIndex == m_iterations ) {
        return true;
    } else {
        return false;
    }
}
