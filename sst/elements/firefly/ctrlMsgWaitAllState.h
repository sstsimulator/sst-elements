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

#ifndef COMPONENTS_FIREFLY_CTRLMSGWAITALLSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGWAITALLSTATE_H

#include <sst/core/output.h>
#include "ctrlMsgState.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T1 >
class WaitAllState : StateBase< T1 > 
{
  public:
    WaitAllState( int verbose, Output::output_location_t loc, T1& obj ) :
        StateBase<T1>( verbose, loc, obj ), 
        m_unblock( this, &WaitAllState<T1>::unblock )
    {
        char buffer[100];
        snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::WaitAllState::@p():@l ",
                            obj.nic().getNodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);
    }
    void enter( std::vector<CommReq*>&, FunctorBase_1<CommReq*,bool>*, 
                                    FunctorBase_0<bool>* funtor = NULL );

    bool unblock();
  private:
    Output& dbg() { return StateBase<T1>::m_dbg; }
    T1& obj() { return StateBase<T1>::obj; }

    FunctorBase_1<CommReq*,bool>*       m_functor;
    Functor_0<WaitAllState<T1>,bool>    m_unblock;
    std::vector<CommReq*>               m_reqs; 
    WaitReq*                            m_waitReq;
};

template< class T1 >
void WaitAllState<T1>::enter( std::vector<CommReq*>& reqs, 
    FunctorBase_1<CommReq*,bool>* functor, FunctorBase_0<bool>* stateFunctor ) 
{
    dbg().verbose(CALL_INFO,1,0,"num reqs %lu\n", reqs.size());
    StateBase<T1>::setExit( stateFunctor );

    m_reqs = reqs;
    m_functor = functor;

    std::vector<_CommReq*> tmp;
    std::vector<CommReq*>::iterator iter = reqs.begin();
    for ( ; iter != reqs.end(); ++iter ) {
        tmp.push_back( (*iter)->req );
    }

    obj().m_processQueuesState->enterWait( new WaitReq( tmp ), &m_unblock );
}

template< class T1 >
bool WaitAllState<T1>::unblock()
{
    dbg().verbose(CALL_INFO,1,0,"\n");
    std::vector<CommReq*>::iterator iter = m_reqs.begin();

    for ( ; iter != m_reqs.end(); ++iter ) {
        if ( (*iter)->req->isDone() ) {
            obj().passCtrlToFunction( obj().waitallStateDelay(), m_functor, (*iter) );
            m_reqs.clear();
            delete (*iter)->req;
            return false;
        }
    }
    dbg().verbose(CALL_INFO,1,0,"no CommReq must be a CommEvent\n");
    obj().passCtrlToFunction( obj().waitallStateDelay(), m_functor, NULL );
    return false;
}


}
}
}

#endif
