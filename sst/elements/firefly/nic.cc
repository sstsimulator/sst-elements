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


#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "sst/core/component.h"

#include "sst/core/element.h"

#include "nic.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}


using namespace SST;
using namespace SST::Firefly;

Nic::Nic(ComponentId_t id, Params_t &params)
{
    DBGX("\n");
}

Nic::~Nic()
{
    DBGX("\n");
}
