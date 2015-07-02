// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGRECVSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGRECVSTATE_H

#include <sst/core/output.h>
#include "ctrlMsgState.h"
#include "ctrlMsgXXX.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T1 >
class RecvState : StateBase< T1 > 
{
  public:
    RecvState( int verbose, int mask, T1& obj ) : 
        StateBase<T1>( verbose, mask, obj ),
        m_afterProcess( this, &RecvState<T1>::afterProcess ),
        m_unblock( this, &RecvState<T1>::unblock )
    {
        char buffer[100];
        snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::RecvState::@p():@l ",
                            obj.nic().getNodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);
    }
    void enter( bool blocking, std::vector<IoVec>&,
            MP::PayloadDataType, MP::RankID dest, uint32_t tag,
            MP::Communicator group, CommReq*,
            FunctorBase_0<bool>*, FunctorBase_0<bool>* = NULL );

    bool afterProcess();
    bool unblock();

  private:
    Output& dbg() { return StateBase<T1>::m_dbg; }
    T1& obj() { return StateBase<T1>::obj; }

    Functor_0<RecvState<T1>,bool> m_afterProcess;
    Functor_0<RecvState<T1>,bool> m_unblock;

    _CommReq*           m_req;
    FunctorBase_0<bool>*  m_functor;
    bool                m_blocking;
};

template< class T1 >
void RecvState<T1>::enter( bool blocking, std::vector<IoVec>& ioVec,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, CommReq* commReq,
        FunctorBase_0<bool>* functor, FunctorBase_0<bool>* stateFunctor ) 
{
    dbg().verbose(CALL_INFO,1,1,"%s src=%d tag=%#x functor=%p\n",
                blocking ? "blocking" : "non-blocking", src, tag, functor );

    StateBase<T1>::setExit( stateFunctor );
    m_functor = functor;
    m_blocking = blocking;

    m_req = new _CommReq( blocking ? _CommReq::Recv: _CommReq::Irecv,
        ioVec, obj().info()->sizeofDataType(dtype), src, tag, group );
    dbg().verbose(CALL_INFO,1,1,"new CommReq %p\n",m_req );
    if ( ! blocking ) {
        commReq->req = m_req;
    }

    obj().m_processQueuesState->enterRecv( m_req, &m_afterProcess );
}

template< class T1 >
bool RecvState<T1>::afterProcess()
{
    dbg().verbose(CALL_INFO,2,1,"\n");

    if ( m_blocking ) {
        if ( m_req->isDone() ) {
            obj().passCtrlToFunction( obj().recvStateDelay(), m_functor );
            dbg().verbose(CALL_INFO,1,1,"delete CommReq %p\n",m_req );
            delete m_req;
        } else {
            obj().m_processQueuesState->enterWait( new WaitReq(m_req), &m_unblock );
        }
    } else {
        obj().passCtrlToFunction( obj().recvStateDelay(), m_functor );
    }

    return false;
}

template< class T1 >
bool RecvState<T1>::unblock()
{
    obj().passCtrlToFunction( obj().recvStateDelay(), m_functor );
    dbg().verbose(CALL_INFO,1,1,"delete CommReq %p\n",m_req );
    delete m_req;
    return false;
}


}
}
}

#endif
