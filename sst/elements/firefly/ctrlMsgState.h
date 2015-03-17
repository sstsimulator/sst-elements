
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

#ifndef COMPONENTS_FIREFLY_CTRLMSGSTATE_H
#define COMPONENTS_FIREFLY_CTRLMSGSTATE_H

namespace SST {
namespace Firefly {
namespace CtrlMsg {

#include "ctrlMsgXXX.h"

class  StateArgsBase {
  protected:
    virtual ~StateArgsBase() = 0; 
};

class  LocalsBase {
  public:
    virtual ~LocalsBase(){} 
};

template< class T1 >
class StateBase {
  public:

    StateBase( int verbose, Output::output_location_t loc, T1& _obj ) 
        : obj( _obj ), m_functor(NULL) 
    {
        m_dbg.init("", verbose, 0, loc );
    }
    virtual ~StateBase() {}

    virtual void exit( int delay = 0 ) {
        //m_dbg.verbose(CALL_INFO,1,0,"\n");
        obj.schedFunctor( m_functor, delay );
        m_functor = NULL;
    }

    void setExit( FunctorBase_0<bool>* functor ) {
        //m_dbg.verbose(CALL_INFO,1,0,"\n");
        assert( !m_functor );
        m_functor = functor;
    }
    FunctorBase_0<bool>* clearExit(  ) {
        FunctorBase_0<bool>* functor = m_functor;
        m_functor = NULL;
        return functor;
    }

    T1&         obj;

  protected:

    FunctorBase_0<bool>*  m_functor;
    Output                m_dbg;
};

}
}
}

#endif
