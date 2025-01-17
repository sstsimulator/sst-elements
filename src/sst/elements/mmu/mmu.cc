// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <math.h>
#include "mmu.h"
#include "mmuEvents.h"

using namespace SST;
using namespace SST::MMU_Lib;

MMU::MMU(SST::ComponentId_t id, SST::Params& params) : SubComponent(id), m_nicTlbLink(nullptr)
{
    m_numCores = params.find<int>("num_cores", 0);
    if ( 0 == m_numCores ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: %s, num_cores is zero\n",getName().c_str());
    }

    m_numHwThreads = params.find<int>("num_threads", 0);
    if ( 0 == m_numHwThreads ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: %s, num_threads is zero\n",getName().c_str());
    }

    int pageSize = params.find<int>("page_size", 0);
    if ( 0 == pageSize ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: %s, page_size is zero\n",getName().c_str());
    }
    m_pageShift = log2( pageSize );

    auto useNicTlb = params.find<bool>("useNicTlb",false);

    for ( int i = 0; i < m_numCores; i++ ) {
        std::string name;
        std::ostringstream core;
        core << i;

        name = "core" + core.str() + ".dtlb";
        Link* dtlb = configureLink( name, new Event::Handler<MMU,int>(this, &MMU::handleTlbEvent, i * 2 + 0 ) );
        if ( nullptr == dtlb ) {
            m_dbg.fatal(CALL_INFO, -1, "Error: %s was unable to configure dtlb link `%s`\n",getName().c_str(),name.c_str());
        }

        name = "core" + core.str() + ".itlb";
        Link* itlb = configureLink( name, new Event::Handler<MMU,int>(this, &MMU::handleTlbEvent, i * 2 + 1 ) );
        if ( nullptr == itlb ) {
            m_dbg.fatal(CALL_INFO, -1, "Error: %s was unable to configure itlb link `%s`\n",getName().c_str(),name.c_str());
        }
        m_coreLinks.push_back( new CoreTlbLinks( dtlb, itlb ));
    }  

    if ( useNicTlb ) {
        std::string name = "nicTlb";
        m_nicTlbLink = configureLink( name, new Event::Handler<MMU>(this, &MMU::handleNicTlbEvent ) );
        if ( nullptr == m_nicTlbLink ) {
            m_dbg.fatal(CALL_INFO, -1, "Error: %s was unable to configure itlb link `%s`\n",getName().c_str(),name.c_str());
        }
    }
}

void MMU::init( unsigned int phase ) 
{
    m_dbg.debug(CALL_INFO_LONG,2,0,"phase=%d\n",phase);
    if ( 0 == phase ) {
        for ( int i=0; i < m_coreLinks.size(); i++ ) {
            m_dbg.debug(CALL_INFO_LONG,1,0,"send Init event to %d, pageShift=%d\n",i, m_pageShift);
            m_coreLinks[i]->dtlb->sendUntimedData( new TlbInitEvent( m_pageShift ) );
            m_coreLinks[i]->itlb->sendUntimedData( new TlbInitEvent( m_pageShift ) );
        }
        if ( m_nicTlbLink ) {
            m_nicTlbLink->sendUntimedData( new TlbInitEvent( m_pageShift ) );
        }
    }
}
