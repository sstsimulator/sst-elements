// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGREADSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGREADSTATE_H

#include <sst/core/output.h>
#include "ctrlMsgState.h"
#include "ctrlMsgXXX.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T1 >
class ReadState : StateBase< T1 > 
{
  public:
    ReadState( int verbose, Output::output_location_t loc, T1& obj ) : 
        StateBase<T1>( verbose, loc, obj ),
        m_unblock( this, &ReadState<T1>::unblock )
    {
        char buffer[100];
        snprintf(buffer,100,"@t:%#x:%d:CtrlMsg::ReadState::@p():@l ",
                            obj.info()->nodeId(), obj.info()->worldRank());
        dbg().setPrefix(buffer);

    }
    void enter( nid_t nid, region_t region , void* buf, size_t len,
         FunctorBase_0<bool>* functor, FunctorBase_0<bool>* stateFunctor = NULL);

    bool unblock();

  private:
    Output& dbg() { return StateBase<T1>::m_dbg; }
    T1& obj() { return StateBase<T1>::obj; }

    Functor_0<ReadState<T1>,bool> m_unblock;
    FunctorBase_0<bool>*    m_functor;
};

template< class T1 >
void ReadState<T1>::enter( nid_t nid, region_t region , void* buf, size_t len,
             FunctorBase_0<bool>* functor, FunctorBase_0<bool>* stateFunctor)
{
    dbg().verbose(CALL_INFO,1,0,"nid=%d region=%d buf=%p len=%lu \n",
                    nid, region, buf, len );

    StateBase<T1>::set( stateFunctor );
    m_functor = functor;
   
    obj().m_processQueuesState->enterRead( nid, region, buf, len, &m_unblock ); 
}

template< class T1 >
bool ReadState<T1>::unblock()
{
    dbg().verbose(CALL_INFO,1,0,"\n");
    obj().passCtrlToFunction( 0, m_functor );
    return false;
}


}
}
}

#endif
