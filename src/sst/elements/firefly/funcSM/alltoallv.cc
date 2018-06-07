// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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


    ++m_seq;
    m_count = 1;
    m_state = PostRecv;
    m_size = m_info->getGroup( m_event->group )->getSize();
    m_rank = m_info->getGroup( m_event->group )->getMyRank();

    m_dbg.debug(CALL_INFO,1,0,"Start size=%d\n",m_size);

    void* recv = recvChunkPtr(m_rank);
    void* send = sendChunkPtr(m_rank);
    if ( recv && send ) {
        memcpy( recv, send, recvChunkSize(m_rank));
    }
    
    retval.setDelay( 0 );
}

void AlltoallvFuncSM::handleEnterEvent( Retval& retval )
{
	Hermes::MemAddr addr;
    MP:: RankID rank;
    switch ( m_state ) {
      case PostRecv:

        if ( m_count == m_size ) {
            m_dbg.debug(CALL_INFO,1,0,"leave\n");
            retval.setExit(0);
            delete m_event;
            m_event = NULL;
            break;
        }
        rank = mod((long) m_rank - m_count, m_size);

        m_dbg.debug(CALL_INFO,1,0,"count=%d irecv src=%d\n", 
                                                        m_count, rank );

		addr.setSimVAddr( 1 ); 
		addr.setBacking( recvChunkPtr(rank) );
        proto()->irecv( addr, recvChunkSize(rank), 
                        rank, genTag(), m_event->group, &m_recvReq ); 
        m_state = Send;
        break;

      case Send:
        rank = mod((long) m_rank + m_count, m_size);

        m_dbg.debug(CALL_INFO,1,0,"count=%d send dest=%d\n", 
                                                        m_count, rank );

		
		addr.setSimVAddr( 1 ); 
		addr.setBacking( sendChunkPtr(rank) ); 
        proto()->send( addr, sendChunkSize(rank), 
                                            rank, genTag(), m_event->group ); 
        m_state = WaitRecv;
        break;

      case WaitRecv:
        m_dbg.debug(CALL_INFO,1,0,"count=%d wait\n", m_count );
        proto()->wait( &m_recvReq );
        ++m_count;
        m_state = PostRecv;
        break;
    }
}
