// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGFUNCTORS_H
#define COMPONENTS_FIREFLY_CTRLMSGFUNCTORS_H

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class TRetval = void  >
class FunctorBase_0 {
  public:
    virtual TRetval operator()() = 0;
	virtual ~FunctorBase_0() {}
};

template< class T1, class TRetval = void  >
class FunctorBase_1 {
  public:
    virtual TRetval operator()(T1) = 0;
	virtual ~FunctorBase_1() {}
};

template< class T1, class T2, class TRetval = void  >
class FunctorBase_2 {
  public:
    virtual TRetval operator()(T1,T2) = 0;
	virtual ~FunctorBase_2() {}
};

template< class T1, class T2, class T3, class TRetval = void  >
class FunctorBase_3 {
  public:
    virtual TRetval operator()(T1,T2,T3) = 0;
	virtual ~FunctorBase_3() {}
};


template <class T1, class TRetval = void >
class Functor_0 : public FunctorBase_0< TRetval >
{
  private:
    T1* m_obj;
    TRetval ( T1::*m_fptr )();

  public:
    Functor_0( T1* obj, TRetval ( T1::*fptr )( ) ) :
        m_obj( obj ),
        m_fptr( fptr )
    { }

    virtual TRetval operator()() {
        return (*m_obj.*m_fptr)();
    }
};

template <class T1, class T2, class TRetval = void >
class Functor_1 : public FunctorBase_1< T2, TRetval >
{
  private:
    T1* m_obj;
    TRetval ( T1::*m_fptr )(T2);

  public:
    Functor_1( T1* obj, TRetval ( T1::*fptr )( T2 ) ) :
        m_obj( obj ),
        m_fptr( fptr )
    { }

    virtual TRetval operator()( T2 arg ) {
        return (*m_obj.*m_fptr)( arg );
    }
};

template <class T1, class T2, class TRetval = void >
class FunctorStatic_0 : public FunctorBase_0< TRetval >
{
  private:
    T1* m_obj;
    TRetval ( T1::*m_fptr )( T2 );
    T2  m_arg;

  public:
    FunctorStatic_0( T1* obj, TRetval ( T1::*fptr )( T2 ), T2 arg ) :
        m_obj( obj ),
        m_fptr( fptr ),
        m_arg( arg )
    { }

    virtual TRetval operator()() {
        return (*m_obj.*m_fptr)( m_arg );
    }
};

template <class T1, class T2, class T3, class TRetval = void >
class FunctorStatic_1 : public FunctorBase_1< T3, TRetval >
{
  private:
    T1* m_obj;
    TRetval ( T1::*m_fptr )( T2, T3 );
    T2  m_arg;

  public:
    FunctorStatic_1( T1* obj, TRetval ( T1::*fptr )( T2, T3 ), T2 arg ) :
        m_obj( obj ),
        m_fptr( fptr ),
        m_arg( arg )
    { }

    virtual TRetval operator()( T3 arg ) {
        return (*m_obj.*m_fptr)( m_arg, arg );
    }
};

template <class T1, class T2, class T3, class T4, class T5, class TRetval = void >
class FunctorStatic_3 : public FunctorBase_3< T3, T4, T5, TRetval >
{
  private:
    T1* m_obj;
    TRetval ( T1::*m_fptr )( T2, T3, T4, T5 );
    T2  m_arg;

  public:
    FunctorStatic_3( T1* obj, TRetval ( T1::*fptr )( T2, T3, T4, T5), T2 arg ) :
        m_obj( obj ),
        m_fptr( fptr ),
        m_arg( arg )
    { }

    virtual TRetval operator()( T3 arg1, T4 arg2, T5 arg3) {
        return (*m_obj.*m_fptr)( m_arg, arg1, arg2, arg3 );
    }
};


}
}
}

#endif
