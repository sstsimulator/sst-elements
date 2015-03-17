// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "funcSM/commSplit.h"
#include "info.h"

using namespace SST::Firefly;

void CommSplitFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    m_commSplitEvent = static_cast< CommSplitStartEvent* >(e);
    assert( m_commSplitEvent );
    
    m_dbg.verbose(CALL_INFO,1,0,"oldGroup=%d\n", m_commSplitEvent->oldComm );
    m_dbg.verbose(CALL_INFO,1,0,"color=%d key=%d\n", 
                m_commSplitEvent->color, m_commSplitEvent->key );

    Group* oldGrp = m_info->getGroup( m_commSplitEvent->oldComm );
    assert(oldGrp);

    uint32_t cnt = oldGrp->getSize();

    m_dbg.verbose(CALL_INFO,1,0,"grpSize=%d\n", cnt );

    m_sendbuf = (int*) malloc( sizeof(int) * 2 );
    m_recvbuf = (int*) malloc( cnt * sizeof(int) * 2);
    m_sendbuf[0] = m_commSplitEvent->color;
    m_sendbuf[1] = m_commSplitEvent->key;

    MP::PayloadDataType datatype = MP::INT;

    m_dbg.verbose(CALL_INFO,1,0,"send=%p recv=%p\n",m_sendbuf,m_recvbuf);

    GatherStartEvent* tmp = new GatherStartEvent( m_sendbuf, 2,
           datatype, m_recvbuf, 2, datatype, m_commSplitEvent->oldComm );

    AllgatherFuncSM::handleStartEvent(
                        static_cast<SST::Event*>( tmp ), retval );
}

void CommSplitFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    AllgatherFuncSM::handleEnterEvent( retval );

    if ( retval.isExit() ) {
        *m_commSplitEvent->newComm = m_info->newGroup(); 
        Group* newGroup = m_info->getGroup( *m_commSplitEvent->newComm );
        assert( newGroup );

        Group* oldGrp = m_info->getGroup( m_commSplitEvent->oldComm);
        assert( oldGrp );
    
        for ( int i = 0; i < oldGrp->getSize(); i++ ) {
            m_dbg.verbose(CALL_INFO,1,0,"i=%d color=%#x key=%#x\n", i,
                                 m_recvbuf[i*2], m_recvbuf[i*2 + 1] );
            if ( m_commSplitEvent->color == m_recvbuf[i*2] ) {
            
                m_dbg.verbose(CALL_INFO,1,0,"add: oldRank=%d newRank=%d\n",
                                    i,m_recvbuf[i*2 + 1]);
                newGroup->initMapping( m_recvbuf[i*2 + 1],
                                    oldGrp->getMapping(i), 1 ); 

                if ( oldGrp->getMyRank() == i ) {
                    newGroup->setMyRank( m_recvbuf[i*2 + 1] );
                }
            }
        }  

        delete m_commSplitEvent;
        free( m_sendbuf );
        free( m_recvbuf );
    }
}
