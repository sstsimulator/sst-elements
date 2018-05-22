// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#define UTIL_OUT out
#define UTIL_PARAMS _params
#define UTIL_LEVEL 2

#define FIND_BOOL(param,def)   UTIL_PARAMS.find_integer( param,def)
#define FIND_INT(param,def)    UTIL_PARAMS.find_integer( param,def)
#define FIND_FLOAT(param,def)  UTIL_PARAMS.find_floating(param,def)
#define FIND_STRING(param,def) UTIL_PARAMS.find_string(  param,def)
#define FIND_HEX(param,def)    UTIL_PARAMS.find_integer( param,def)

#define REPORT_BOOL(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,             \
                                      #var " = %s\n",(var ? "true" : "false"))

#define REPORT_STRING(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,           \
                                        #var " = '%s'\n",var.c_str())

#define REPORT_INT(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,              \
                                     #var " = %d\n",var)

#define REPORT_INT32(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,            \
                                       #var " = %ld\n",var)

#define REPORT_INT64(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,            \
                                       #var " = %lld\n",var)

#define REPORT_HEX(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,              \
                                     #var " = %#08X\n",var)

#define REPORT_HEX32(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,            \
                                       #var " = %#016lX\n",var)

#define REPORT_HEX64(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,            \
                                       #var " = %#032llX\n",var)

#define REPORT_FLOAT(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,            \
                                       #var " = %f\n",var)

#define REPORT_DOUBLE(var) UTIL_OUT.verbose(CALL_INFO,UTIL_LEVEL,0,           \
                                        #var " = %lf\n",var)

#define FIND_REPORT_BOOL(var, param, def)                                     \
  var =  FIND_BOOL(param,def);                                                \
  REPORT_BOOL(var)

#define FIND_REPORT_STRING(var,param,def)                                     \
  var = FIND_STRING(param,def);                                               \
  REPORT_STRING(var)

#define FIND_REPORT_INT(var,param,def)                                        \
  var = FIND_INT(param,def);                                                  \
  REPORT_INT(var)

#define FIND_REPORT_INT32(var,param,def)                                      \
  var = FIND_INT(param,def);                                                  \
  REPORT_INT32(var)

#define FIND_REPORT_INT64(var,param,def)                                      \
  var = FIND_INT(param,def);                                                  \
  REPORT_INT64(var)

#define FIND_REPORT_FLOAT(var,param,def)                                      \
  var = FIND_FLOAT(param,def);                                                \
  REPORT_FLOAT(var)

#define FIND_REPORT_DOUBLE(var,param,def)                                     \
  var = FIND_FLOAT(param,def);                                                \
  REPORT_DOUBLE(var)

#define FIND_REPORT_HEX(var,param,def)                                        \
  var = FIND_HEX(param,def);                                                  \
  REPORT_HEX(var)

#define FIND_REPORT_HEX32(var,param,def)                                      \
  var = FIND_HEX(param,def);                                                  \
  REPORT_HEX32(var)

#define FIND_REPORT_HEX64(var,param,def)                                      \
  var = FIND_HEX(param,def);                                                  \
  REPORT_HEX64(var)


#define NULL_PARAM      {NULL,NULL,NULL}
#define NULL_PORT       {NULL,NULL,NULL}
#define NULL_MODULE     {NULL,NULL,NULL,NULL,NULL,NULL,NULL}
#define NULL_COMPONENT  {NULL,NULL,NULL,NULL,NULL,NULL,0}

#endif//COMMON_UTIL_H
