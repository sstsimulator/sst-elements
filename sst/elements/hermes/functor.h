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

#ifndef _H_HERMES_FUNCTOR
#define _H_HERMES_FUNCTOR

template < class TRetval = void >
class VoidArg_FunctorBase { 
  public:
    virtual TRetval operator()() = 0;
};

template <class TClass, class TArg, class TRetval = void > 
class StaticArg_Functor : public VoidArg_FunctorBase< TRetval >
{
  private:
    TClass* m_obj;
    TRetval ( TClass::*m_fptr )( TArg );
    TArg m_arg;

  public:
    StaticArg_Functor( TClass* obj, TRetval ( TClass::*fptr )( TArg ), TArg arg) :
        m_obj( obj ),
        m_fptr( fptr ),
        m_arg( arg )
    { }

    virtual TRetval operator()() {
        return (*m_obj.*m_fptr)(m_arg );
    }
};

template < class TArg, class TRetval = void >
class Arg_FunctorBase { 
  public:
    virtual TRetval operator()(TArg) = 0;
};

template <class TClass, class TArg, class TRetval = void > 
class Arg_Functor : public Arg_FunctorBase< TArg, TRetval >
{
  private:
    TClass* m_obj;
    TRetval ( TClass::*m_fptr )( TArg );

  public:
    Arg_Functor( TClass* obj, TRetval ( TClass::*fptr )( TArg )) :
        m_obj( obj ),
        m_fptr( fptr )
    { }
    Arg_Functor() {}

    virtual TRetval operator()( TArg arg ) {
        return (*m_obj.*m_fptr)(arg );
    }
};

#endif
