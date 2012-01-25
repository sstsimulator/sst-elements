
#ifndef DEBUG_H
#define DEBUG_H

#include <cxxabi.h>
#include <stdio.h>

#define TRACE_ADD(...)
#define TRACE_INIT(...)

#define PRINT_AT( x, fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    fflush(stderr);\
    if ( realname ) free(realname);\
}

#endif
