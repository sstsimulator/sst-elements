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

#include "funcSM/collectiveTree.h"
#include "funcSM/collectiveOps.h"
#include "info.h"

using namespace SST::Firefly;

const char* CollectiveTreeFuncSM::m_enumName[] = {
    FOREACH_ENUM(GENERATE_STRING)
};

void CollectiveTreeFuncSM::handleStartEvent( SST::Event *e, Retval& retval )
{
    assert( NULL == m_event );
    m_event = static_cast< CollectiveStartEvent* >(e);

    ++m_seq;

    m_yyy = new YYY( 2, m_info->getGroup(m_event->group)->getMyRank(),
                m_info->getGroup(m_event->group)->getSize(), m_event->root );

    m_dbg.debug(CALL_INFO,1,0,"%s group %d, root %d, size %d, rank %d\n",
                m_event->typeName(),
                m_event->group, m_event->root, m_yyy->size(),
                m_yyy->myRank());

    m_dbg.debug(CALL_INFO,1,0,"parent %d \n",m_yyy->parent());
    for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
        m_dbg.debug(CALL_INFO,1,0,"child[%d]=%d\n",i,m_yyy->calcChild(i));
    }

    m_recvReqV.resize( m_yyy->numChildren() );
    m_recvReqV_ptrs.resize( m_recvReqV.size() );
    for ( unsigned i = 0; i < m_recvReqV.size(); i++ ) {
        m_recvReqV_ptrs[i] = &m_recvReqV[i];
    }

    m_sendReqV.resize( m_yyy->numChildren() );
    m_sendReqV_ptrs.resize( m_sendReqV.size() );
    for ( unsigned i = 0; i < m_sendReqV.size(); i++ ) {
        m_sendReqV_ptrs[i] = &m_sendReqV[i];
    }

    m_bufV.resize( m_yyy->numChildren() + 1);

    m_bufLen = m_event->count * m_info->sizeofDataType( m_event->dtype );
    if ( m_bufLen <= m_smallCollectiveSize ) {
        m_vn = m_smallCollectiveVN;
    }


    m_bufV[0] = m_event->mydata.getBacking();

    for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
        if ( m_event->mydata.getBacking() ) {
            m_bufV[i+1] = malloc( m_bufLen );
            assert( m_bufV[i+1] );
        } else {
            m_bufV[i+1] = NULL;
        }
    }

    m_waitUpState.init();
    m_sendDownState.init();
    if ( m_event->type == CollectiveStartEvent::Bcast ) {
        m_state = WaitDown;
    } else {
        m_state = WaitUp;
    }
    handleEnterEvent( retval );
}

void CollectiveTreeFuncSM::handleEnterEvent( Retval& retval )
{
	Hermes::MemAddr addr;
    int child;
    m_dbg.debug(CALL_INFO,1,0,"%s state\n", stateName(m_state).c_str());

    switch ( m_state ) {
    case WaitUp:
        if (  m_yyy->numChildren() ) {

            switch ( m_waitUpState.state ) {
              case WaitUpState::Posting:
                child = m_waitUpState.count;

                ++m_waitUpState.count;

                if ( m_waitUpState.count == m_yyy->numChildren() ) {
                    m_waitUpState.count = 0;
                    m_waitUpState.state = WaitUpState::Waiting;
                }

                m_dbg.debug(CALL_INFO,1,0,"post irecv for child %d\n", child );
				addr.setSimVAddr( 1 );
				addr.setBacking( m_bufV[ child + 1 ] );
                proto()->irecv( addr, m_bufLen,
                        m_yyy->calcChild( child ),
                        genTag(), m_event->group,  &m_recvReqV[ child ] );
                return;

              case WaitUpState::Waiting:
                m_waitUpState.state = WaitUpState::DoOp;
                proto()->waitAll( m_recvReqV_ptrs );
                return;

              case WaitUpState::DoOp:
                m_dbg.debug(CALL_INFO,1,0,"all children have checked in\n");
                    if ( m_bufV[0] ) {
                        collectiveOp( &m_bufV[0], m_yyy->numChildren() + 1,
                            m_event->result.getBacking(), m_event->count,
                            m_event->dtype, m_event->op );
                    }
            }
        }
        m_state = SendUp;

    case SendUp:
        if ( -1 != m_yyy->parent() ) {
            void *ptr;

            m_state = WaitDown;

            ptr = m_yyy->numChildren() ?  m_event->result.getBacking() : m_event->mydata.getBacking();

            m_dbg.debug(CALL_INFO,1,0,"send message to parent %d\n",
                                                            m_yyy->parent());
			addr.setSimVAddr( 1 );
			addr.setBacking( ptr );
            proto()->send( addr, m_bufLen, m_yyy->parent(), genTag(), m_event->group, m_vn );
            return;
        }

    case WaitDown:
        if ( m_event->type != CollectiveStartEvent::Reduce &&
                                 -1 != m_yyy->parent() ) {
            m_state = SendDown;

            m_dbg.debug(CALL_INFO,1,0,"post recv from parent %d\n",
                                                            m_yyy->parent());
			addr.setSimVAddr( 1 );
			addr.setBacking( m_event->result.getBacking() );
            proto()->recv( addr, m_bufLen, m_yyy->parent(),
                                                         genTag(), m_event->group );
            return;
        }

    case SendDown:
        if ( m_event->type != CollectiveStartEvent::Reduce &&
                                    m_yyy->numChildren() ) {

			switch ( m_sendDownState.state ) {
			  case SendDownState::Sending:
				child = m_sendDownState.count;
				++m_sendDownState.count;

                if ( m_sendDownState.count == m_yyy->numChildren() ) {
                    m_sendDownState.count = 0;
                    m_sendDownState.state = SendDownState::Waiting;
                }

                m_dbg.debug(CALL_INFO,1,0,"isend to child %d\n", child );
				addr.setSimVAddr( 1 );
				addr.setBacking( m_event->result.getBacking() );
                proto()->isend( addr, m_bufLen, m_yyy->calcChild( child ),
                        genTag(), m_event->group, &m_sendReqV[ child ], m_vn );
				return;
			  case SendDownState::Waiting:
				m_state = Exit;
                m_dbg.debug(CALL_INFO,1,0,"wait on sends to children\n" );
                proto()->waitAll( m_sendReqV_ptrs );
				return;
			}
        }

    case Exit:
        m_dbg.debug(CALL_INFO,1,0,"Exit\n" );
        retval.setExit( 0 );
        for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
            if ( m_bufV[i+1] ) {
                free( m_bufV[i+1] );
            }
        }
        delete m_yyy;
        delete m_event;
        m_event = NULL;
    }
}
