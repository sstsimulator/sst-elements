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


#ifndef _debug_h
#define _debug_h

#include "log.h"
#include <cxxabi.h>

namespace SST {
namespace M5 {

extern Log<> _dbg;
extern Log<> _info;
extern bool SST_M5_debug;

extern void enableDebug( std::string name );

}
}


#define PRINTFN(...) do {                                      \
    if ( Trace::enabled && SST_M5_debug )                       \
        Trace::dprintf(curTick(), name(), __VA_ARGS__);           \
} while (0)

#define _error(name, fmt, args...) \
{\
fprintf(stderr,"%s::%s():%i:FAILED: " fmt, #name, __FUNCTION__, __LINE__, ## args);\
exit(-1); \
}


#if 0
#define DBGX_M5( x, fmt, args... ) \
{\
    fprintf( stderr, "%7lu: %s: %s():%d: "fmt, curTick(), name().c_str(), \
                        __func__, __LINE__, ##args);\
}

#define DBGX( x, fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
	SST::M5::_dbg.write( x, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
						__func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

#else

#define DBGX_M5( x, fmt, args... ) do { } while(0)
#define DBGX( x, fmt, args... ) do { } while(0)

#endif

#define DBGC( x, fmt, args... ) \
	SST::M5::_dbg.write( x, "%s():%d: "fmt, __func__, __LINE__, ##args)

#define INFO( fmt, args... ) \
    SST::M5::_info.write( 1, fmt, ##args)


#define WHERE csprintf("%s::%s():%d", abi::__cxa_demangle(typeid(*this).name(),0,0,NULL) , __func__, __LINE__ )

#endif
