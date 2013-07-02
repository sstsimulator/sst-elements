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

#include "funcCtx/size.h"
#include "hades.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}


using namespace SST::Firefly;
using namespace Hermes;

SizeCtx::SizeCtx( Communicator group, int* size,
            Functor* retFunc, FunctionType type, Hades* obj ) : 
    FunctionCtx( retFunc, type, obj ),
    m_group( group ),
    m_size( size )
{ }

bool SizeCtx::run( ) 
{
    DBGX("\n");
    
    *m_size = m_obj->m_groupMap[m_group]->size(); 
    return true; 
}
