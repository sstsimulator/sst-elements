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

#include "luascript_backend.h"
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/******************************************************************************/
void 
luaRetrieveValues(lua_State* L, 
		  char* format,
		  ...)
{
  va_list ap;
  void* ptr;
  int i;

  va_start(ap, format);

  i = 0;
  while ( format[i] != 0 )
  {
    ptr = va_arg(ap, void*);
    switch (format[i])
    {
    case L_INT:
    case L_DOUBLE:
    case L_LONG:
      *((int*)ptr) = lua_tonumber(L,i+1);
      break;
    case L_STRING:
      strcpy(*((char**)ptr), lua_tostring(L,i+1));
      break;
    case L_BOOLEAN:
      *((bool*)ptr) = lua_toboolean(L,i+1);
      break;
    default:
      printf("Invalid or unsupported type passed into luaRetrieveValues\n");
      exit(1);
    }
    i++;
  }

  va_end(ap);
}

/******************************************************************************/
int
luaReturnValues(lua_State* L,
		char* format, 
		...)
{
  va_list ap;
  void *ptr;
  int i;
  int arg;

  va_start(ap, format);
  i = 0;
  while ( format[i] != 0 )
  {
    ptr = va_arg(ap, void*);
    switch ( format[i] )
    {
    case L_INT:
      lua_pushnumber(L, *((int*)ptr));
      break;
    case L_DOUBLE:
      lua_pushnumber(L, *((double*)ptr));
      break;
    case L_LONG:
      lua_pushnumber(L, *((long*)ptr));
      break;
    case L_STRING:
      lua_pushstring(L, *((char**)ptr));
      break;
    case L_BOOLEAN:
      lua_pushboolean(L, *((bool*)ptr));
      break;
    default:
      printf("Invalid or unsupported type passed into luaReturnValues()\n");
    }
    i++;
  }
  
  return i;
}
		   
/******************************************************************************/
void 
luaStartup(lua_State** L)
{
  *L = lua_open();
  luaL_openlibs(*L);
}

/******************************************************************************/
void 
luaShutdown(lua_State** L)
{
  lua_close(*L);
}

/******************************************************************************/
int 
function1(int a, int b, int c)
{
  return a+b+c;
}

/******************************************************************************/
int 
luafunction1(lua_State* L)
{
  char formatRetrieve[] = {L_INT, L_INT, L_INT, 0};
  char formatReturn[] = {L_INT, 0};
  int a, b, c;
  int res;

  luaRetrieveValues(L, formatRetrieve, &a, &b, &c);
  res = function1(a,b,c);
  return luaReturnValues(L, formatReturn, &res);
}

/******************************************************************************/
int 
function2(int a, int b)
{
  return a*b;
}

/******************************************************************************/
int 
luafunction2(lua_State* L)
{
  char formatRetrieve[] = {L_INT, L_INT, 0};
  char formatReturn[] = {L_INT, 0};
  int a, b;
  int res;

  luaRetrieveValues(L, formatRetrieve, &a, &b);
  res = function2(a,b);
  return luaReturnValues(L, formatReturn, &res);
}

/******************************************************************************/
int 
main ( int argc, char* argv[] )
{
  std::string file="testscript";
  lua_State* L;

  luaStartup(&L);
  lua_register(L, "function1", luafunction1);
  lua_register(L, "function2", luafunction2);
  luaL_dofile(L, file.c_str());  

  luaShutdown(&L);

  return 0;
}

