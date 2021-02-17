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

#include <string.h>

#include "funcSM/gatherv.h"
#include "info.h"

using namespace SST::Firefly;

GathervFuncSM::GathervFuncSM( SST::Params& params ) :
    FunctionSMInterface(params),
    m_event( NULL ),
    m_seq( 0 )
{
    m_smallCollectiveVN = params.find<int>( "smallCollectiveVN", 0);
    m_smallCollectiveSize = params.find<int>( "smallCollectiveSize", 0);
}

void GathervFuncSM::handleStartEvent( SST::Event *e, Retval& retval )
{
    ++m_seq;

    assert( NULL == m_event );
    m_event = static_cast< GatherStartEvent* >(e);

    m_qqq = new QQQ( 2, m_info->getGroup(m_event->group)->getMyRank(),
                m_info->getGroup(m_event->group)->getSize(), m_event->root );

    m_dbg.debug(CALL_INFO,1,0,"group %d, root %d, size %d, rank %d\n",
                    m_event->group, m_event->root, m_qqq->size(),
                    m_qqq->myRank());

    m_dbg.debug(CALL_INFO,1,0,"parent %d \n",m_qqq->parent());
    for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) {
        m_dbg.debug(CALL_INFO,1,0,"child[%d]=%d\n",i,m_qqq->calcChild(i));
    }

    m_state = WaitUp;
    m_waitUpState.init();
    m_sendUpState.init();

    m_recvReqV.resize(m_qqq->numChildren());
    m_waitUpSize.resize(m_qqq->numChildren());

    handleEnterEvent( retval );
}

void GathervFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    switch( m_state ) {
    case WaitUp:
        if ( m_qqq->numChildren() && waitUp(retval) ) {
            return;
        }
        m_state = SendUp;
    case SendUp:
        if ( -1 != m_qqq->parent() && sendUp(retval) ) {
            return;
        }
        m_dbg.debug(CALL_INFO,1,0,"leave\n");
        retval.setExit(0);
        delete m_qqq;
        delete m_event;
        m_event = NULL;
    }
}

bool GathervFuncSM::waitUp(Retval& retval)
{
	Hermes::MemAddr addr;
    int len;
    m_dbg.debug(CALL_INFO,1,0,"\n");
    switch( m_waitUpState.state ) {

      case WaitUpState::PostSizeRecvs:

        m_dbg.debug(CALL_INFO,1,0,"post recv for child %d[%d]\n",
                 m_waitUpState.count, m_qqq->calcChild(m_waitUpState.count) );

		addr.setSimVAddr( 1 );
		addr.setBacking( &m_waitUpSize[m_waitUpState.count] );
        proto()->irecv( addr,
                    sizeof(m_waitUpSize[m_waitUpState.count]),
                    m_qqq->calcChild(m_waitUpState.count),
                    genTag(1),
                    &m_recvReqV[m_waitUpState.count] );

        ++m_waitUpState.count;

        if ( m_waitUpState.count == m_qqq->numChildren() ) {
            m_waitUpState.state = WaitUpState::WaitSizeRecvs;
            m_waitUpState.count = 0;
        }
        return true;

      case WaitUpState::WaitSizeRecvs:

        m_dbg.debug(CALL_INFO,1,0,"wait for Size msg from child %d[%d]\n",
                 m_waitUpState.count, m_qqq->calcChild(m_waitUpState.count) );
        proto()->wait( &m_recvReqV[m_waitUpState.count] );

        ++m_waitUpState.count;

        if ( m_waitUpState.count == m_qqq->numChildren() ) {
            m_waitUpState.state = WaitUpState::Setup;
        }
        return true;

      case WaitUpState::Setup:
        m_dbg.debug(CALL_INFO,1,0,"all children have checked in\n");
        // calculate the size of receive buffer
        // this nodes part of buffer
        len = m_info->sizeofDataType( m_event->sendtype ) * m_event->sendcnt;
        for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) {

            m_dbg.debug(CALL_INFO,1,0,"child %d[%d] has %d bytes\n",
                               i, m_qqq->calcChild(i), m_waitUpSize[i] );
            // childs part of buffer
            len += m_waitUpSize[i];
        }
        m_recvBuf.resize( len );
        m_waitUpState.state = WaitUpState::PostDataRecv;
        m_waitUpState.count = 0;
        m_waitUpState.len =
            m_info->sizeofDataType( m_event->sendtype ) * m_event->sendcnt;

      case WaitUpState::PostDataRecv:

        m_dbg.debug(CALL_INFO,1,0,"post recv for child %d[%d]\n",
                   m_waitUpState.count, m_qqq->calcChild(m_waitUpState.count) );
		addr.setSimVAddr( 1 );
		addr.setBacking( &m_recvBuf[m_waitUpState.len] );
        proto()->irecv( addr,
                    m_waitUpSize[m_waitUpState.count],
                    m_qqq->calcChild(m_waitUpState.count),
                    genTag(2),
                    &m_recvReqV[m_waitUpState.count] );
        m_waitUpState.len += m_waitUpSize[m_waitUpState.count];

        ++m_waitUpState.count;
        if ( m_waitUpState.count == m_qqq->numChildren() ) {
            m_waitUpState.state = WaitUpState::SendSize;
            m_waitUpState.count = 0;
        }

        return true;

    case WaitUpState::SendSize:
        m_dbg.debug(CALL_INFO,1,0,"send I'm ready message to"
                " child %d[%d]\n", m_waitUpState.count,
                m_qqq->calcChild( m_waitUpState.count ) );
		addr.setSimVAddr( 1 );
		addr.setBacking( NULL );
        proto()->send( addr, 0, m_qqq->calcChild( m_waitUpState.count ),
                            genTag(3), m_smallCollectiveVN );

        ++m_waitUpState.count;
        if ( m_waitUpState.count == m_qqq->numChildren() ) {
            m_waitUpState.state = WaitUpState::WaitDataRecv;
            m_waitUpState.count = 0;
        }

        return true;

    case WaitUpState::WaitDataRecv:
        m_dbg.debug(CALL_INFO,1,0,"wait for data from %d\n",
                            m_waitUpState.count );

        proto()->wait( &m_recvReqV[m_waitUpState.count] );

        ++m_waitUpState.count;
        if ( m_waitUpState.count == m_qqq->numChildren() ) {
            m_waitUpState.state = WaitUpState::DoRoot;
        }

        return true;

    case WaitUpState::DoRoot:
        m_dbg.debug(CALL_INFO,1,0,"doRoot\n");
        if ( -1 == m_qqq->parent() ) {
            doRoot( );
        }
    }
    return false;
}

void GathervFuncSM::doRoot()
{
    std::vector<int> map = m_qqq->getMap();

    int len = m_info->sizeofDataType( m_event->sendtype ) * m_event->sendcnt;
    m_dbg.debug(CALL_INFO,1,0,"I'm root buf.size()=%lu\n", m_recvBuf.size());

    memcpy( &m_recvBuf[0], m_event->sendbuf.getBacking(), len );

#if 0 // print debug
    for ( unsigned int i = 0; i < m_recvBuf.size()/4; i++ ) {
        printf("%#03x\n", ((unsigned int*) &m_recvBuf[0])[i]);
    }
#endif

    int offset = 0;
    for ( unsigned int  i = 0; i < map.size(); i++ ) {
        m_dbg.debug(CALL_INFO,1,0,"%d -> %d\n", map[i], i);

        int rank = map[i];

        int len;
        if ( m_event->recvcntPtr ) {
            int recvcnt = ((int*)m_event->recvcntPtr)[rank];
            int displs = ((int*)m_event->displsPtr)[rank];


            len = m_info->sizeofDataType( m_event->recvtype ) * recvcnt;

            m_dbg.debug(CALL_INFO,1,0,"rank %d, recvcnt %d, displs %d, "
                                "len=%u\n", rank, recvcnt, displs, len);
            memcpy( (unsigned char*) m_event->recvbuf.getBacking() + displs,
                            &m_recvBuf[offset], len);
        } else {
            len = m_info->sizeofDataType( m_event->recvtype ) *
                                    m_event->recvcnt;
            memcpy( (unsigned char*) m_event->recvbuf.getBacking() + len * rank,
                            &m_recvBuf[offset], len);
        }

        offset += len;
    }

#if 0 // print debug
    for ( unsigned int i = 0; i < m_recvBuf.size()/4; i++ ) {
        printf("%#03x\n", ((unsigned int*) m_event->recvbuf)[i]);
    }
#endif
}

bool GathervFuncSM::sendUp(Retval& retval)
{
	Hermes::MemAddr addr;
    size_t len;
    m_dbg.debug(CALL_INFO,1,0,"\n");

    switch ( m_sendUpState.state ) {
    case SendUpState::SendSize:
        len = m_info->sizeofDataType( m_event->sendtype ) *
                    m_event->sendcnt;
        if ( 0 == m_qqq->numChildren() ) {
            m_recvBuf.resize( len );
        }
        memcpy( &m_recvBuf[0], m_event->sendbuf.getBacking(), len );
        m_intBuf = m_recvBuf.size();
        m_dbg.debug(CALL_INFO,1,0,"send Sening %d bytes message to %d\n",
                                                m_intBuf, m_qqq->parent());
		addr.setSimVAddr( 1 );
		addr.setBacking( &m_intBuf );
        {
            int vn = 0;
            if ( sizeof(m_intBuf) < m_smallCollectiveSize ) {
                vn = m_smallCollectiveVN;
            }
            proto()->send( addr, sizeof(m_intBuf), m_qqq->parent(),
                            genTag(1), vn );
        }

        m_sendUpState.state = SendUpState::RecvGo;
        return true;

    case SendUpState::RecvGo:
        m_dbg.debug(CALL_INFO,1,0,"post receive for Go msg, parrent=%d\n",
                                        m_qqq->parent());
		addr.setSimVAddr( 1 );
		addr.setBacking( NULL );
        proto()->recv( addr, 0, m_qqq->parent(), genTag(3) );

        m_sendUpState.state = SendUpState::SendBody;
        return true;

    case SendUpState::SendBody:

        m_dbg.debug(CALL_INFO,1,0,"sending body to parent %d\n",
                                            m_qqq->parent());
		addr.setSimVAddr( 1 );
		addr.setBacking( &m_recvBuf[0] );
        {
            int vn = 0;
            if ( m_recvBuf.size() < m_smallCollectiveSize ) {
                vn = m_smallCollectiveVN;
            }
            proto()->send( addr, m_recvBuf.size(), m_qqq->parent(),
                            genTag(2), vn );
        }
#if 0 // print debug
        for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
            printf("%#03x\n", m_recvBuf[i]);
        }
#endif
        m_sendUpState.state = SendUpState::SentBody;
        return true;

      case SendUpState::SentBody:
        break;
    }
    return false;
}
