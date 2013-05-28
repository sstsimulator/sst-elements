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

#ifndef _callback_h
#define _callback_h

namespace SST {
namespace Portals4 {

class CallbackBase {
  public:
    virtual bool operator()() = 0;
    virtual ~CallbackBase() {}
};

template <class X, class Y = void > class Callback : public CallbackBase
{
    typedef bool (X::*FuncPtr)(Y*);

  public:
    Callback( X* object, FuncPtr funcPtr, Y* data = 0 ) :
        done( false ),
        m_object( object ),
        m_funcPtr( funcPtr ),
        m_data( data )
    {
    }
    virtual bool operator()() {
        return (*m_object.*m_funcPtr)( m_data );
    }
    bool done;
  private:
    X*      m_object;
    FuncPtr m_funcPtr;
    Y*      m_data;
};

}
}

#endif
