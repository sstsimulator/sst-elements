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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <iostream>
#include <stdarg.h>
#include <string.h>

#include "sstdisksim_straightdisk.h"
#include "sst/core/element.h"
#include "sstdisksim.h"

#include "sstdisksim_posix_call.h"

#include <map>
typedef std::map<size_t, size_t> arg_map;
arg_map fd_map;

#define DBG( fmt, args... ) \
    __dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

/******************************************************************************/
sstdisksim_event*
sstdisksim_straightdisk::getNextEvent()
{
  sstdisksim_event* ev = new sstdisksim_event();
  ev->completed = 0;
  ev->devno = 0;  // we always have our own disk

  long long cur_pos = 0;
  arg_map::iterator iter;

  sstdisksim_posix_call* cur_event = __list.pop_entry();
  
  bool looping = true;
  while ( looping == true && cur_event != NULL )
  {
    switch(cur_event->call)
    {
    case _CALL_PREAD:
    case _CALL_PREAD64:
      ev->etype = DISKSIMREAD;
      ev->count = cur_event->args[_ARG_COUNT].t;
      ev->pos = cur_event->args[_ARG_OFFSET].t;  

      if ( ev->count < 0 )
      {
	free(cur_event);
	cur_event = __list.pop_entry();
	break;
      }

      fd_map.erase(cur_event->args[_ARG_FD].t);
      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));
      //      printf("%d PREAD pos %lld count %lld \n", ___j++, ev->pos, ev->count);

      looping = false;
      break;

    case _CALL_PWRITE:
    case _CALL_PWRITE64:
      ev->etype = DISKSIMWRITE;
      ev->count = cur_event->args[_ARG_COUNT].t;
      ev->pos = cur_event->args[_ARG_OFFSET].t;  

      if ( ev->count < 0 )
      {
	free(cur_event);
	cur_event = __list.pop_entry();
	break;
      }

      fd_map.erase(cur_event->args[_ARG_FD].t);
      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));
      //      printf("%d PWRITE pos %lld count %lld \n", ___j++, ev->pos, ev->count);

      looping = false;
      break;

    case _CALL_READ:
    case _CALL_READV:
    case _CALL_FREAD:
      ev->etype = DISKSIMREAD;
      ev->count = cur_event->args[_ARG_COUNT].t;
      ev->pos = 0;  

      if ( ev->count < 0 )
      {
	free(cur_event);
	cur_event = __list.pop_entry();
	break;
      }

      iter = fd_map.find(cur_event->args[_ARG_FD].t);

      if ( iter != fd_map.end() )
      {
	ev->pos =  iter->second;
	fd_map.erase(cur_event->args[_ARG_FD].t);
      }

      /*      if ( cur_event->call->call == _CALL_READV )
      	printf("V");
      else if ( cur_event->call->call == _CALL_FREAD )
      	printf ("F");

	printf("%d READ pos %lld count %lld %d\n", ___j++, ev->pos, ev->count);*/

      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));
      looping = false;
      break;

    case _CALL_WRITE:
    case _CALL_WRITEV:
    case _CALL_FWRITE:
      ev->etype = DISKSIMWRITE;
      ev->count = cur_event->args[_ARG_COUNT].t;
      iter = fd_map.find(cur_event->args[_ARG_FD].t);
      ev->pos = 0;  

      if ( ev->count < 0 )
      {
	free(cur_event);
	cur_event = __list.pop_entry();
	break;
      }

      if ( iter != fd_map.end() )
      {
	ev->pos =  iter->second;
	fd_map.erase(cur_event->args[_ARG_FD].t);
      }
      
      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_event->args[_ARG_COUNT].t + ev->pos));


      /*      if ( cur_event->call->call == _CALL_WRITEV )
	printf("V");
      else if ( cur_event->call->call == _CALL_FWRITE )
	printf ("F");
   
	printf("%d WRITE pos %lld count %lld %d\n", ___j++, ev->pos, ev->count);*/

      looping = false;
      break;

    case _CALL_CLOSE:
    case _CALL_FCLOSE:
      fd_map.erase(cur_event->args[_ARG_FD].t);
      free(cur_event);
      cur_event = __list.pop_entry();
      break;

    case _CALL_LSEEK:
    case _CALL_LSEEK64:
    case _CALL_FSEEK:
      switch ( cur_event->args[_ARG_WHENCE].i )
	{
	case 0: //seek_set
	  cur_pos = cur_event->args[_ARG_OFFSET].l;
	  break;
	case 1: //seek_cur
	  iter = fd_map.find(cur_event->args[_ARG_FD].t);
	  cur_pos = cur_event->args[_ARG_OFFSET].l + iter->second;
	  break;
	  
	case 2: //seek_end - unsupported because we don't know file sizes
	default:
	  cur_pos = 0;
	  break;
	}
      
      fd_map.erase(cur_event->args[_ARG_FD].t);
      fd_map.insert(std::pair<size_t, long>(cur_event->args[_ARG_FD].t,
					    cur_pos));

      /*      if ( cur_event->call->call == _CALL_FSEEK )
	printf("F");
      else if ( cur_event->call->call == _CALL_LSEEK64 )
        printf ("64");

	printf("LSEEK pos %lld\n", cur_pos);*/
      free(cur_event);
      cur_event = __list.pop_entry();

      break;    
    case _END_CALLS:
      looping = false;
      ev->etype = DISKSIMEND;  
      break;
    case _CALL_CREAT:
    case _CALL_CREAT64:
    case _CALL_FSYNC:
    case _CALL_FOPEN:
    case _CALL_FOPEN64:
    case _CALL_OPEN:
    case _CALL_OPEN64:
    default:
      free(cur_event);
      cur_event = __list.pop_entry();
      break;
    }
  }

  if ( cur_event == NULL )
  {
    free(ev);
    return NULL;
  }

  free(cur_event);
  
  /*
  //skippy test override
  static int __i = 0;
  static int __pos = 0;
  if ( __i < 1000 )
  {
    __pos += __i*512;
    ev->etype = DISKSIMWRITE;
    ev->pos = 512 + __pos;
    ev->count = 512;  
    __i++;
  }
  else 
    return NULL;
  */

  return ev;
}

/******************************************************************************/
bool
sstdisksim_straightdisk::clock(Cycle_t current)
{
  sstdisksim_event* event = getNextEvent();
  static bool _ended = false;

  /* At the end of our input */
  if ( event == NULL )
    return false;

  if ( event->etype == DISKSIMEND )
  {
    primaryComponentOKToEndSim();
    _ended = true;
  }
  
  link->send(0, event); 
  return false;
}

/******************************************************************************/
sstdisksim_straightdisk::sstdisksim_straightdisk( ComponentId_t id,  
						  Params_t& params ) :
  Component( id ),
  __dbg( *new Log< DISKSIM_DBG >( "DisksimTracereader::", false ) )
{
  __id = id;

  registerTimeBase("1ps");

  // Clock speed really doesn't matter much here-it is used to sync up the simulations.
  registerClock("1GHz", 
  		new Clock::Handler<sstdisksim_straightdisk>(this, 
  							   &sstdisksim_straightdisk::clock));
 
  straightdisk = configureLink("straightdisk",  
			       new Event::Handler<sstdisksim_straightdisk>(this,&sstdisksim_straightdisk::handleEvent));

  link = configureLink( "link" );
  if ( link == 0 )
  {
    printf("No disk specified.  See sstdisksim_tracereader.xml or README for more info.\n");
    exit(1);
  }

  printf("Starting disk controller plain disk up\n");

  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

/******************************************************************************/
sstdisksim_straightdisk::~sstdisksim_straightdisk()
{
}

/******************************************************************************/
void 
sstdisksim_straightdisk::handleEvent(Event* event)
{
  sstdisksim_posix_event* ev = static_cast<sstdisksim_posix_event*>(event);

  __list.add_entry(ev);

  delete(ev);
}

/******************************************************************************/
void sstdisksim_straightdisk::setup() 
{
}

/******************************************************************************/
void sstdisksim_straightdisk::finish() 
{
  DBG("Shutting sstdisksim_straightdisk down\n");

}

/******************************************************************************/
static Component*
create_sstdisksim_straightdisk(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new sstdisksim_straightdisk( id, params );
}

/******************************************************************************/
static const ElementInfoComponent components[] = {
    { "sstdisksim_straightdisk",
      "sstdisksim_straightdisk driver",
      NULL,
      create_sstdisksim_straightdisk
    },
    { NULL, NULL, NULL, NULL }
};

/******************************************************************************/
extern "C" 
{
  ElementLibraryInfo sstdisksim_straightdisk_eli = {
    "sstdisksim_straightdisk",
    "sstdisksim_straightdisk serialization",
    components
  };
}
