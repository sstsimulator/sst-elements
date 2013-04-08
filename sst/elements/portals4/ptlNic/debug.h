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

#ifndef DEBUG_H
#define DEBUG_H

#include <cxxabi.h>
#include <stdio.h>

#define TRACE_ADD(...)
#define TRACE_INIT(...)

#if 1 
#define PRINT_AT( x, fmt, args... )
#define _PRINT_AT( x, prefix, fmt, args... )
#else

#define PRINT_AT( x, fmt, args... ) _PRINT_AT( x, "", fmt, ##args )

#define _PRINT_AT( x, prefix, fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s%s::%s():%d: "fmt, prefix, \
        realname ? realname : "?????????", __func__, __LINE__, ##args);\
    fflush(stderr);\
    if ( realname ) free(realname);\
}
#endif

#endif
