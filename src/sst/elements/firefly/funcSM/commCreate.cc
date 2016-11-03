// Copyright 2013-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2016, Sandia Corporation
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

#include "funcSM/commCreate.h"
#include "info.h"

using namespace SST::Firefly;

void CommCreateFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    CommCreateStartEvent* event = static_cast< CommCreateStartEvent* >(e);
    assert( event );
    
    m_dbg.verbose(CALL_INFO,1,0,"oldGroup=%d\n", event->oldComm );
    m_dbg.verbose(CALL_INFO,1,0,"nRaks=%lu\n", 
                event->nRanks );

    Group* oldGrp = m_info->getGroup( event->oldComm);
    assert(oldGrp);

    uint32_t cnt = oldGrp->getSize();

    m_dbg.verbose(CALL_INFO,1,0,"old grpSize=%d\n", cnt );

    *event->newComm = m_info->newGroup(); 
    Group* newGroup = m_info->getGroup( *event->newComm );
    assert( newGroup );

    for ( size_t i = 0; i < event->nRanks; i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"i=%lu %d \n", i, event->ranks[i] );

        newGroup->initMapping( i, oldGrp->getMapping( event->ranks[i]), 1 ); 

        if ( oldGrp->getMyRank() == event->ranks[i] ) {
            newGroup->setMyRank( i );
        }

    }  
    delete event;

    BarrierStartEvent* tmp = new BarrierStartEvent( MP::GroupWorld  );

    BarrierFuncSM::handleStartEvent(static_cast<SST::Event*>(tmp), retval );
}

void CommCreateFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    BarrierFuncSM::handleEnterEvent( retval );

}
