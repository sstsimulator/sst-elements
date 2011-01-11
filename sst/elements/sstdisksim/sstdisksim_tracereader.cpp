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

#include "sstdisksim_tracereader.h"
#include "sst/core/element.h"
#include "sstdisksim.h"

#define max_num_tracreaders 128
static sstdisksim_tracereader* __ptrs[128];

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    __dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

/******************************************************************************/
bool
sstdisksim_tracereader::clock(Cycle_t current)
{
  sstdisksim_event* event = __parser->getNextEvent();
  /* At the end of our input */
  if ( event == NULL )
  {
    return false;
  }

  link->Send(0, event);
  return false;
}

/******************************************************************************/
int
sstdisksim_tracereader::traceRead(int count, int pos, int devno)
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
int
sstdisksim_tracereader::traceWrite(int count, int pos, int devno)
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
sstdisksim_tracereader::sstdisksim_tracereader( ComponentId_t id,  
						Params_t& params ) :
  Component( id ),
  __dbg( *new Log< DISKSIM_DBG >( "DisksimTracereader::", false ) )
{
  __id = id;
  __done = 0;
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

    ++it;
  }

  __parser = new sstdisksim_otf_parser(traceFile);

  registerTimeBase("1ps");
  link = configureLink( "link" );

  // Clock speed really doesn't matter much here-it is used to sync up the simulations.
  registerClock("1GHz", 
  		new Clock::Handler<sstdisksim_tracereader>(this, 
  							   &sstdisksim_tracereader::clock));
 
  DBG("Starting sstdisksim_tracereader up\n");

  registerExit();
}

/******************************************************************************/
sstdisksim_tracereader::~sstdisksim_tracereader()
{
  delete(__parser);
}

/******************************************************************************/
int
sstdisksim_tracereader::Setup()
{
  unregisterExit();

  return 0;
}

/******************************************************************************/
int 
sstdisksim_tracereader::Finish()
{
  DBG("Shutting sstdisksim_tracereader down\n");

  return 0;
}

/******************************************************************************/
static Component*
create_sstdisksim_tracereader(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new sstdisksim_tracereader( id, params );
}

/******************************************************************************/
static const ElementInfoComponent components[] = {
    { "sstdisksim_tracereader",
      "sstdisksim_tracereader driver",
      NULL,
      create_sstdisksim_tracereader
    },
    { NULL, NULL, NULL, NULL }
};

/******************************************************************************/
extern "C" 
{
  ElementLibraryInfo sstdisksim_tracereader_eli = {
    "sstdisksim_tracereader",
    "sstdisksim_tracereader serialization",
    components
  };
}
