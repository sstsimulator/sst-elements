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

#include "sst_config.h"

#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

void Nic::RecvMachine::printStatus( Output& out ) {
#ifdef NIC_RECV_DEBUG
    if ( m_nic.m_linkControl->requestToReceive( 0 ) ) {
        out.output( "%lu: %d: RecvMachine `%s` msgCount=%d runCount=%d,"
            " net event avail %d\n",
            Simulation::getSimulation()->getCurrentSimCycle(),
            m_nic. m_myNodeId, state( m_state), m_msgCount, m_runCount,
            m_nic.m_linkControl->requestToReceive( 0 ) );
    }
#endif
}
