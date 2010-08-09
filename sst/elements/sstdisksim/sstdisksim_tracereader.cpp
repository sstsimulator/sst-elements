// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
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

#include "sstdisksim_tracereader.h"
#include "sst/core/element.h"
#include "sstdisksim.h"

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

/******************************************************************************/
sstdisksim_tracereader::sstdisksim_tracereader( ComponentId_t id,  Params_t& params ) :
  Component( id ),
  m_dbg( *new Log< DISKSIM_DBG >( "Disksim::", false ) )
{
  std::string traceFile = "";
  __id = id;
  __done = 0;
  
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

  printf("Starting sstdisksim_tracereader up\n");

  registerExit();
 
  return;
}

/******************************************************************************/
sstdisksim_tracereader::~sstdisksim_tracereader()
{
}

/******************************************************************************/
int
sstdisksim_tracereader::Setup()
{
  for ( int i = 0; i < 400; i++ )
  {
    sstdisksim_event* event = new sstdisksim_event();
    event->count = 512*33+15;
    event->pos = 0;
    event->devno = 0;
    event->done = 0;
    if ( i%2 == 0 )
      event->etype = READ;
    else
      event->etype = WRITE;
    
    link->Send(0, event);
  }

  sstdisksim_event* event = new sstdisksim_event();
  event->done = 1;
  link->Send(0, event);

  unregisterExit();

  return 0;
}

/******************************************************************************/
int 
sstdisksim_tracereader::Finish()
{
  printf("Shutting sstdisksim_tracereader down\n");

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
    "sstdisksim_tracereader serilaization",
    components
  };
}
