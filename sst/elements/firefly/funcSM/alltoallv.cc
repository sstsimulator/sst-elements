// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>

#include <string.h>

#include "funcSM/alltoallv.h"

using namespace SST::Firefly;

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

const char* AlltoallvFuncSM::m_enumName[] = {
    FOREACH_ENUM(GENERATE_STRING)
};

void AlltoallvFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    assert( NULL == m_event );
    m_event = static_cast< AlltoallStartEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"Start size=%d\n",m_size);

    ++m_seq;
    m_count = 1;
    m_state = PostRecv;
    m_size = m_info->getGroup( m_event->group )->getSize();
    m_rank = m_info->getGroup( m_event->group )->getMyRank();

    void* recv = recvChunkPtr(m_rank);
    void* send = sendChunkPtr(m_rank);
    if ( recv && send ) {
        memcpy( recv, send, recvChunkSize(m_rank));
    }

    handleEnterEvent( retval );
}

void AlltoallvFuncSM::handleEnterEvent( Retval& retval )
{
    int node;
    switch ( m_state ) {
      case PostRecv:

        if ( m_count == m_size ) {
            m_dbg.verbose(CALL_INFO,1,0,"leave\n");
            retval.setExit(0);
            delete m_event;
            m_event = NULL;
            break;
        }

        node = mod((long) m_rank - m_count, m_size);

        m_dbg.verbose(CALL_INFO,1,0,"count=%d irecv src=%d\n", m_count, node );

        proto()->irecv( recvChunkPtr(node), recvChunkSize(node), 
                        node, genTag(), &m_recvReq ); 
        m_state = Send;
        break;

      case Send:
        node = mod((long) m_rank + m_count, m_size);

        m_dbg.verbose(CALL_INFO,1,0,"count=%d send dest=%d\n", m_count, node );

        proto()->send( sendChunkPtr(node), sendChunkSize(node), 
                                            node, genTag() ); 
        m_state = WaitRecv;
        break;

      case WaitRecv:
        m_dbg.verbose(CALL_INFO,1,0,"count=%d wait\n", m_count );
        proto()->wait( &m_recvReq );
        ++m_count;
        m_state = PostRecv;
        break;
    }
}
