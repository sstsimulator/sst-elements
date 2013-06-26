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

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "sst/elements/hermes/msgapi.h"

#include "functionCtx.h"
#include "hades.h"

using namespace SST::Firefly;
using namespace Hermes;

#define stringify( x ) #x

const char* FunctionCtx::functionName[] = {
        stringify(Init),
        stringify(Fini),
        stringify(Rank),
        stringify(Size),
        stringify(Barrier),
        stringify(Irecv),
        stringify(Isend),
        stringify(Send),
        stringify(Recv),
        stringify(Wait),
    };

FunctionCtx::FunctionCtx( Functor* retFunc, FunctionType type,
                Hades* obj, int latency ) :
        m_retFunc(retFunc),
        m_type(type),
        m_obj(obj),
        m_retval( SUCCESS ),
        m_latency( latency )
{
    printf("FunctionCtx::%s() %s\n",__func__, functionName[m_type]);
}

FunctionCtx::~FunctionCtx() 
{
    printf("FunctionCtx::%s() %s\n",__func__, functionName[m_type]);
    m_obj->sendReturn( m_latency, m_retFunc, m_retval );
}
