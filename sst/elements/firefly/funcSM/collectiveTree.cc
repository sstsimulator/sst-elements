// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization.h"

#include "funcSM/collectiveTree.h"
#include "funcSM/collectiveOps.h"

using namespace SST::Firefly;

CollectiveTreeFuncSM::CollectiveTreeFuncSM( 
                    int verboseLevel, Output::output_location_t loc,
                    Info* info, SST::Link* progressLink, 
                    ProtocolAPI* ctrlMsg, SST::Link* selfLink ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_event( NULL ),
    m_toProgressLink( progressLink ),
    m_selfLink( selfLink ),
    m_ctrlMsg( static_cast<CtrlMsg*>(ctrlMsg) ),
    m_seq( 0 )
{
    m_dbg.setPrefix("@t:CollectiveTreeFuncSM::@p():@l ");
}

void CollectiveTreeFuncSM::handleEnterEvent( SST::Event *e) 
{
    assert( NULL == m_event );
    m_event = static_cast< CollectiveEnterEvent* >(e);

    ++m_seq;

    m_yyy = new YYY( 2, m_info->getGroup(m_event->group)->getMyRank(),
                m_info->getGroup(m_event->group)->size(), m_event->root ); 

    m_dbg.verbose(CALL_INFO,1,0,"%s group %d, root %d, size %d, rank %d\n",
                    m_event->all ? "ALL ": "",
                    m_event->group, m_event->root, m_yyy->size(), 
                    m_yyy->myRank());
    
    m_dbg.verbose(CALL_INFO,1,0,"parent %d \n",m_yyy->parent());
    for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"child[%d]=%d\n",i,m_yyy->calcChild(i));
    }

    m_recvReqV.resize( m_yyy->numChildren() + 1);
    m_bufV.resize( m_yyy->numChildren() + 1);

    m_bufLen = m_event->count * m_info->sizeofDataType( m_event->dtype );  

    m_bufV[0] = m_event->mydata;
    for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
        m_bufV[i+1] = malloc( m_bufLen );
        assert( m_bufV[i+1] );
    }

    m_pending = false;
    m_state = WaitUp;
    m_delay = 0;
    m_count = 0;
    m_toProgressLink->send( 0, NULL );
}

void CollectiveTreeFuncSM::handleSelfEvent( SST::Event *e )
{
    handleProgressEvent( e );
}

void CollectiveTreeFuncSM::handleProgressEvent( SST::Event *e )
{
    switch ( m_state ) {
    case WaitUp:
        if (  m_yyy->numChildren() ) {
            m_dbg.verbose(CALL_INFO,1,0,"WaitUp\n");
            if ( ! m_pending ) {
                for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
                    m_dbg.verbose(CALL_INFO,1,0,"post recv for %d\n",i);
                    m_ctrlMsg->recv( m_bufV[i+1], m_bufLen, m_yyy->calcChild(i), 
                            genTag(), m_event->group, &m_recvReqV[i+1] );
                }
                m_count = 0;
                m_pending = true;
            } else {
                if ( ! m_delay ) {
                    m_test = m_ctrlMsg->test(&m_recvReqV[m_count+1], m_delay);  
                    if ( m_delay ) {
                        m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                        m_selfLink->send( m_delay, NULL );
                        break;
                    }
                } else {
                    m_delay = 0;
                }
                if ( m_test ) {
                    m_dbg.verbose(CALL_INFO,1,0,"got message from %d\n",
                                                                    m_count);
                     ++m_count;
                } else {
                    m_ctrlMsg->sleep();
                    m_toProgressLink->send(0, NULL );
                    break;
                }
            }
            if ( m_count < m_yyy->numChildren() ) { 
                m_toProgressLink->send(0, NULL );
                break;
            } else {
                m_dbg.verbose(CALL_INFO,1,0,"all children have checked in\n");
                if ( m_event->count ) {
                    collectiveOp( &m_bufV[0], m_yyy->numChildren() + 1,
                        m_event->result, m_event->count,
                        m_event->dtype, m_event->op );  
                }
            }
        } 
        m_pending = false;
        m_state = SendUp;

    case SendUp:
        if ( -1 != m_yyy->parent() ) {
            m_dbg.verbose(CALL_INFO,1,0,"SendUp\n");
            if ( ! m_pending ) {
                m_dbg.verbose(CALL_INFO,1,0,"send message to parent %d\n",
                                                            m_yyy->parent());
                if ( m_yyy->numChildren() ) {
                    m_ctrlMsg->send( m_event->result, m_bufLen, m_yyy->parent(),
                            genTag(), m_event->group, &m_sendReq );
                } else {
                    m_ctrlMsg->send( m_event->mydata, m_bufLen, m_yyy->parent(),
                            genTag(), m_event->group, &m_sendReq );
                }
                m_toProgressLink->send(0, NULL );
                m_pending = true;
                break;
            } else {
                // we don't need to wait for the send to complete as  
                // hades will not return until the send is complete 
            }
        }
        m_pending = false;
        m_state = WaitDown;

    case WaitDown:
        if ( m_event->all && -1 != m_yyy->parent() ) {
            m_dbg.verbose(CALL_INFO,1,0,"WaitDown\n");
            if ( ! m_pending ) {
                m_ctrlMsg->recv( m_event->result, m_bufLen, m_yyy->parent(),
                    genTag(), m_event->group, &m_recvReqV[0] );
                m_dbg.verbose(CALL_INFO,1,0,"post recv from parent %d\n",
                                                            m_yyy->parent());
                m_pending = true;
                m_toProgressLink->send(0, NULL );
                break;
            } else {
                if ( ! m_delay ) {
                    m_test = m_ctrlMsg->test( &m_recvReqV[0], m_delay );
                    if ( m_delay ) {
                        m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                        m_selfLink->send( m_delay, NULL );
                        break;
                    }
                } else {
                    m_delay = 0;
                }
                if ( m_test ) {
                    m_dbg.verbose(CALL_INFO,1,0,"got message from %d\n",
                                                            m_yyy->parent());
                } else {
                    m_ctrlMsg->sleep();
                    m_toProgressLink->send(0, NULL );
                    break;
                }
            }
        }
        m_pending = true;
        m_state = SendDown;
        m_count = 0;

    case SendDown:
        if ( m_event->all && m_yyy->numChildren() ) {
            m_dbg.verbose(CALL_INFO,1,0,"SendDown\n");
            if ( m_count < m_yyy->numChildren() ) {
                m_ctrlMsg->send( m_event->result, m_bufLen,
                    m_yyy->calcChild(m_count++), 
                    genTag(), m_event->group, &m_sendReq );
                m_toProgressLink->send(0, NULL );
                break;
            } 
        }
        m_dbg.verbose(CALL_INFO,1,0,"leave\n");
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        delete m_yyy;
        delete m_event;
        for ( unsigned int i = 0; i < m_yyy->numChildren(); i++ ) {
            free( m_bufV[i+1] );
        }
        m_event = NULL;
    }
}
