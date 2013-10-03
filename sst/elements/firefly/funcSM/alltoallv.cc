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

#include "funcSM/alltoallv.h"

using namespace SST::Firefly;

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
//    printf("%s() a=%li b=%li res=%li\n",__func__,a,b,tmp);
    return tmp;
}

AlltoallvFuncSM::AlltoallvFuncSM( 
                    int verboseLevel, Output::output_location_t loc,
                    Info* info, ProtocolAPI* ctrlMsg, SST::Link* selfLink ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_selfLink( selfLink ),
    m_ctrlMsg( static_cast<CtrlMsg*>(ctrlMsg) ),
    m_event( NULL ),
    m_seq( 0 )
{
    m_dbg.setPrefix("@t:AlltoallvFuncSM::@p():@l ");
}

void AlltoallvFuncSM::handleEnterEvent( SST::Event *e ) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:AlltoallvFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event );
    m_event = static_cast< AlltoallEnterEvent* >(e);

    ++m_seq;
    m_count = 1;
    m_pending = false;
    m_size = m_info->getGroup( m_event->group )->size();
    m_rank = m_info->getGroup( m_event->group )->getMyRank();

    memcpy( recvChunkPtr(m_rank), sendChunkPtr(m_rank),recvChunkSize(m_rank));

    m_ctrlMsg->enter();
}

void AlltoallvFuncSM::handleSelfEvent( SST::Event *e )
{
    handleProgressEvent( e );
}

void AlltoallvFuncSM::handleProgressEvent( SST::Event *e )
{
    m_dbg.verbose(CALL_INFO,1,0,"m_count=%d\n",m_count);
    if ( m_count == m_size ) {
        m_dbg.verbose(CALL_INFO,1,0,"leave\n");
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
        return;
    }

    if ( ! m_pending ) {
        int rNode = mod((long) m_rank - m_count, m_size);
        int sNode = mod((long) m_rank + m_count, m_size);
        int tag = genTag();
        m_ctrlMsg->recv( recvChunkPtr(rNode), recvChunkSize(rNode), 
                rNode, tag, m_event->group, &m_recvReq ); 
        m_ctrlMsg->send( sendChunkPtr(sNode), sendChunkSize(sNode), 
                sNode, tag, m_event->group, &m_sendReq ); 
    
        m_dbg.verbose(CALL_INFO,1,0,"count=%d recvNode=%d sendNode=%d\n",
                            m_count, rNode, sNode);
        m_ctrlMsg->enter();
        m_pending = true;
        m_waitSend = true;
        m_delay = 0;
    } else {
        if ( m_waitSend) {
            if ( ! m_delay ) {
                m_test = m_ctrlMsg->test( &m_sendReq, m_delay );
                if ( m_delay ) {
                    m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                    m_selfLink->send( m_delay, NULL );
                    return;
                }
            } else {
                m_delay = 0;
            }
            if ( ! m_test ) {
                m_ctrlMsg->sleep();
                return;
            }
            m_dbg.verbose(CALL_INFO,1,0,"send %d complete\n", m_count );
            m_waitSend = false;
            m_delay = 0;
            handleProgressEvent(NULL);
            return;
            
        } else {
            if ( ! m_delay ) {
                m_test = m_ctrlMsg->test( &m_recvReq, m_delay );
                if ( m_delay ) {
                    m_dbg.verbose(CALL_INFO,1,0,"delay %d\n", m_delay );
                    m_selfLink->send( m_delay, NULL );
                    return;
                }
            } else {
                m_delay = 0;
            }
            if ( ! m_test ) {
                m_ctrlMsg->sleep();
                return;
            }
            m_dbg.verbose(CALL_INFO,1,0,"recv %d complete\n", m_count );
        }

        m_dbg.verbose(CALL_INFO,1,0,"bump count\n");
        m_pending = false;
        ++m_count;
        m_ctrlMsg->enter();
    }
}
