// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.x

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

#define max_num_tracreaders 128
static sstdisksim_straightdisk* __ptrs[128];

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    __dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

/******************************************************************************/
bool
sstdisksim_straightdisk::clock(Cycle_t current)
{
  static int end_sent = 0;
  sstdisksim_event* event = __parser->getNextEvent();
  /* At the end of our input */
  if ( event == NULL )
  {
    if ( end_sent < 1 )
    {
      event = new sstdisksim_event;
      event->etype = DISKSIMEND;
      end_sent++;
    }
    else 
      return false;
  }

  link->Send(0, event);
  return false;
}

/******************************************************************************/
sstdisksim_straightdisk::sstdisksim_straightdisk( ComponentId_t id,  
						Params_t& params ) :
  Component( id ),
  __dbg( *new Log< DISKSIM_DBG >( "DisksimTracereader::", false ) )
{
  __id = id;
  traceFile = "";
  __ptrs[id] = this;
  disksimTracereaderClockCycle = 0;
  
  if ( params.find( "debug" ) != params.end() ) 
  {
    if ( params[ "debug" ].compare( "yes" ) == 0 ) 
    {
      __dbg.enable();
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

    if ( ! it->first.compare("edf_file") ) 
    {
      edfFile = it->second;
    }

    ++it;
  }

  __parser = new sstdisksim_tau_parser(traceFile.c_str(), edfFile.c_str());

  registerTimeBase("1ps");
  link = configureLink( "link" );

  // Clock speed really doesn't matter much here-it is used to sync up the simulations.
  registerClock("1GHz", 
  		new Clock::Handler<sstdisksim_straightdisk>(this, 
  							   &sstdisksim_straightdisk::clock));
 
  DBG("Starting sstdisksim_straightdisk up\n");

  registerExit();
}

/******************************************************************************/
sstdisksim_straightdisk::~sstdisksim_straightdisk()
{
  delete(__parser);
}

/******************************************************************************/
int
sstdisksim_straightdisk::Setup()
{
  // Should be the last thing added 
  unregisterExit();

  return 0;
}

/******************************************************************************/
int 
sstdisksim_straightdisk::Finish()
{
  DBG("Shutting sstdisksim_straightdisk down\n");

  return 0;
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
