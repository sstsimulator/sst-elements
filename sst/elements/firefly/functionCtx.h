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

#ifndef COMPONENTS_FIREFLY_FUNCTIONCTX_H
#define COMPONENTS_FIREFLY_FUNCTIONCTX_H

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

class Hades;

class FunctionCtx {

 public:
    enum FunctionType {
        Init,
        Fini,
        Rank,
        Size,
        Barrier,
        Irecv,
        Isend,
        Send,
        Recv,
        Wait,
        NumFunctions
    };

    static const char* functionName[];

  public:
    FunctionCtx( Hermes::Functor* retFunc, FunctionType type, 
                            Hades* obj, int latency = 1 );
    ~FunctionCtx();

    virtual bool run( ) { 
        // tell the caller to delete us (we are done) 
        return true;
    }
    const char* name() { 
        return functionName[m_type]; 
    }
         
  protected:
    Hermes::Functor*    m_retFunc;
    FunctionType        m_type;       
    Hades*              m_obj;
    int                 m_retval;
    int                 m_latency;
};

}
}

#endif
