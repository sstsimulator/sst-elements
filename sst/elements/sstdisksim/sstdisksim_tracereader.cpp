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

#include "sstdisksim_diskmodel.h"
#include "sstdisksim_straightdisk.h"

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
  sstdisksim_posix_event* event = __parser->getNextEvent();
  static bool _ended = false;

  /* At the end of our input */
  if ( event == NULL )
  {
    if ( _ended == false )
    {
      unregisterExit();
      _ended = true;
    }

    return false;
  }

  diskmodel->Send(0, event);
  return false;
}

/******************************************************************************/
sstdisksim_tracereader::sstdisksim_tracereader( ComponentId_t id,  
						Params_t& params ) :
  Component( id ),
  __dbg( *new Log< DISKSIM_DBG >( "DisksimTracereader::", false ) )
{
  __id = id;
  traceFile = "";
  __ptrs[id] = this;
  
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

  registerTimeBase("1ps");

  diskmodel = configureLink( "straightdisk" );
  if ( diskmodel == 0 )
    diskmodel = configureLink( "raid0" );

  if ( diskmodel == 0 )
  {
    printf("No diskmodel specified.  See sstdisksim_tracereader.xml or README for more info.\n");
    exit(1);
  }

  __parser = new sstdisksim_tau_parser(traceFile.c_str(), edfFile.c_str());

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
