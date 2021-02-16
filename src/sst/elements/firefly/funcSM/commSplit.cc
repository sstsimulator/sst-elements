// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

#include "funcSM/commSplit.h"
#include "info.h"

using namespace SST::Firefly;

void CommSplitFuncSM::handleStartEvent( SST::Event *e, Retval& retval )
{
    m_commSplitEvent = static_cast< CommSplitStartEvent* >(e);
    assert( m_commSplitEvent );

    m_dbg.debug(CALL_INFO,1,0,"oldGroup=%d\n", m_commSplitEvent->oldComm );
    m_dbg.debug(CALL_INFO,1,0,"color=%d key=%d\n",
                m_commSplitEvent->color, m_commSplitEvent->key );

    Group* oldGrp = m_info->getGroup( m_commSplitEvent->oldComm );
    assert(oldGrp);

    uint32_t cnt = oldGrp->getSize();

    if (1 == cnt ) {

        assert( m_commSplitEvent->key == 0 );
        *m_commSplitEvent->newComm = m_info->newGroup();
        Group* newGroup = m_info->getGroup( *m_commSplitEvent->newComm );
        assert( newGroup );
        newGroup->initMapping( 0, oldGrp->getMapping(0), 1 );
        newGroup->setMyRank( 0 );

        retval.setExit(0);
        return;
    }

    m_dbg.debug(CALL_INFO,1,0,"grpSize=%d\n", cnt );

	m_sendbuf.setSimVAddr( 1 );
    m_sendbuf.setBacking( (int*) malloc( sizeof(int) * 2 ) );
	m_recvbuf.setSimVAddr( 1 );
    m_recvbuf.setBacking( (int*) malloc( cnt * sizeof(int) * 2) );
    ((int*)m_sendbuf.getBacking())[0] = m_commSplitEvent->color;
    ((int*)m_sendbuf.getBacking())[1] = m_commSplitEvent->key;

    MP::PayloadDataType datatype = MP::INT;

    m_dbg.debug(CALL_INFO,1,0,"send=%p recv=%p\n",&m_sendbuf,&m_recvbuf);

    GatherStartEvent* tmp = new GatherStartEvent( m_sendbuf, 2,
           datatype, m_recvbuf, 2, datatype, m_commSplitEvent->oldComm );

    AllgatherFuncSM::handleStartEvent(
                        static_cast<SST::Event*>( tmp ), retval );
}

void CommSplitFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.debug(CALL_INFO,1,0,"\n");

    AllgatherFuncSM::handleEnterEvent( retval );

    if ( retval.isExit() ) {
        *m_commSplitEvent->newComm = m_info->newGroup();
        Group* newGroup = m_info->getGroup( *m_commSplitEvent->newComm );
        assert( newGroup );

        Group* oldGrp = m_info->getGroup( m_commSplitEvent->oldComm);
        assert( oldGrp );

        for ( int i = 0; i < oldGrp->getSize(); i++ ) {
            m_dbg.debug(CALL_INFO,1,0,"i=%d color=%#x key=%#x\n", i,
                 	((int*)m_recvbuf.getBacking())[i*2],
					((int*)m_recvbuf.getBacking())[i*2 + 1] );
            if ( m_commSplitEvent->color ==
					((int*)m_recvbuf.getBacking())[i*2] ) {

                m_dbg.debug(CALL_INFO,1,0,"add: oldRank=%d newRank=%d\n",
                                    i,
					((int*)m_recvbuf.getBacking())[i*2 + 1]);
                newGroup->initMapping(
					((int*)m_recvbuf.getBacking())[i*2 + 1],
                                    oldGrp->getMapping(i), 1 );

                if ( oldGrp->getMyRank() == i ) {
                    newGroup->setMyRank(
						((int*)m_recvbuf.getBacking())[i*2 + 1] );
                }
            }
        }

        delete m_commSplitEvent;
        free( m_sendbuf.getBacking() );
        free( m_recvbuf.getBacking() );
    }
}
