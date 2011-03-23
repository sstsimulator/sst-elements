
#ifndef _debug_h
#define _debug_h

#include <log.h>
#include <cxxabi.h>

extern Log<> _dbg;
extern Log<> _info;

#define DBGX( x, fmt, args... ) \
{\
     char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    _dbg.write( x, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
						__func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

#define DBGC( x, fmt, args... ) \
    _dbg.write( x, "%s():%d: "fmt, __func__, __LINE__, ##args)

#define INFO( fmt, args... ) \
    _info.write( 1, fmt, ##args)

#define WHERE csprintf("%s::%s():%d", abi::__cxa_demangle(typeid(*this).name(),0,0,NULL) , __func__, __LINE__ )

#endif
