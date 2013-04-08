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

#ifndef PTL_INTERNAL_DEBUG_H
#define PTL_INTERNAL_DEBUG_H

#include <stdio.h>

extern int _ptl_dbg_rank;

#ifndef DEBUG

#define PTL_DBG( fmt, args...)
#define PTL_DBG2( fmt, args...)

#else 

#define PTL_DBG( fmt, args...) \
fprintf(stderr,"%d:%s():%i: " fmt, _ptl_dbg_rank, __FUNCTION__, __LINE__, ## args);

#ifdef __cplusplus
#include <cxxabi.h>

#define PTL_DBG2( fmt, args...) \
{\
     char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%d:%s::%s():%d: "fmt, _ptl_dbg_rank, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}


#endif

#endif


#endif
