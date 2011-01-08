// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <iostream>
#include <stdarg.h>
#include <string.h>

#include "sstdisksim_luascriptreader.h"
#include "sst/core/element.h"
#include "sstdisksim.h"

#define max_num_tracreaders 128
static sstdisksim_luascriptreader* __ptrs[128];

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

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
/*************LUA FUNCTIONS GO HERE********************************************/
/******************************************************************************/
int
sstdisksim_luascriptreader::luaRead(int count, int pos, int devno)
{
  sstdisksim_event* event = new sstdisksim_event();
  event->count = count;
  event->pos = pos;
  event->devno = devno;
  event->etype = DISKSIMREAD;
  event->completed = false;

  link->Send(0, event);

  return 0;
}

/******************************************************************************/
static int
luaReadCall(lua_State* L)
{
  lua_getglobal(L, "sst_thread_id");
  int id = (int)lua_tointeger(L, -1);
  lua_pop(L,1);

  char formatRetrieve[] = {L_INT, L_INT, L_INT, 0};
  char formatReturn[] = {L_INT, 0};
  
  int a, b, c;
  int res;

  luaRetrieveValues(L, formatRetrieve, &a, &b, &c);
  res = __ptrs[id]->luaRead(a, b, c);
  luaReturnValues(L, formatReturn, &res);

  return 0;
}

/******************************************************************************/
int
sstdisksim_luascriptreader::luaWrite(int count, int pos, int devno)
{
  sstdisksim_event* event = new sstdisksim_event();
  event->count = count;
  event->pos = pos;
  event->devno = devno;
  event->etype = DISKSIMWRITE;
  event->completed = false;

  link->Send(0, event);

  return 0;
}

/******************************************************************************/
static int
luaWriteCall(lua_State* L)
{
  lua_getglobal(L, "sst_thread_id");
  int id = (int)lua_tointeger(L, -1);
  lua_pop(L,1);

  char formatRetrieve[] = {L_INT, L_INT, L_INT, 0};
  char formatReturn[] = {L_INT, 0};
  
  int a, b, c;
  int res;

  lua_resume(L, 0);

  luaRetrieveValues(L, formatRetrieve, &a, &b, &c);
  res = __ptrs[id]->luaWrite(a, b, c);
  luaReturnValues(L, formatReturn, &res);

  return res;
}

/******************************************************************************/
sstdisksim_luascriptreader::sstdisksim_luascriptreader( ComponentId_t id,  Params_t& params ) :
  Component( id ),
  m_dbg( *new Log< DISKSIM_DBG >( "Disksim::", false ) )
{
  __id = id;
  __done = 0;
  traceFile = "";
  __ptrs[id] = this;
  __otherthread = NULL;
  
  if ( params.find( "debug" ) != params.end() ) 
  {
    if ( params[ "debug" ].compare( "yes" ) == 0 ) 
    {
      m_dbg.enable();
    }
  } 

  Params_t::iterator it = params.begin();
  while( it != params.end() ) 
  {
    DBG("key=%s value=%s\n",
        it->first.c_str(),it->second.c_str());

    if ( ! it->first.compare("trace_file") ) 
    {
      traceFile = it->second;
    }

    ++it;
  }

  registerTimeBase("1ps");
  link = configureLink( "link" );

  printf("Starting sstdisksim_luascriptreader up\n");

  __L = luaL_newstate();
  luaL_openlibs(__L);
  lua_newtable(__L);

  lua_register(__L, "luaRead", luaReadCall);
  lua_register(__L, "luaWrite", luaWriteCall);

  lua_pushinteger(__L, id);
  lua_setglobal(__L, "sst_thread_id");

  registerExit();
}

/******************************************************************************/
sstdisksim_luascriptreader::~sstdisksim_luascriptreader()
{
}

/******************************************************************************/
int
sstdisksim_luascriptreader::Setup()
{
  luaL_dofile(__L, traceFile.c_str());

  sstdisksim_event* event = new sstdisksim_event();
  event->etype = DISKSIMEND;
  link->Send(0, event);

  unregisterExit();

  return 0;
}

/******************************************************************************/
int 
sstdisksim_luascriptreader::Finish()
{
  printf("Shutting sstdisksim_luascriptreader down\n");

  lua_close(__L);

  return 0;
}

/******************************************************************************/
static Component*
create_sstdisksim_luascriptreader(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new sstdisksim_luascriptreader( id, params );
}

/******************************************************************************/
static const ElementInfoComponent components[] = {
    { "sstdisksim_luascriptreader",
      "sstdisksim_luascriptreader driver",
      NULL,
      create_sstdisksim_luascriptreader
    },
    { NULL, NULL, NULL, NULL }
};

/******************************************************************************/
extern "C" 
{
  ElementLibraryInfo sstdisksim_luascriptreader_eli = {
    "sstdisksim_luascriptreader",
    "sstdisksim_luascriptreader serialization",
    components
  };
}
