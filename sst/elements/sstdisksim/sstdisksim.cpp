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

#include "sstdisksim.h"
#include "sst/core/element.h"

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

/******************************************************************************/
void
panic(const char *s)
{
  perror(s);
  exit(1);
}

/******************************************************************************/
void
print_statistics(sstdisksim_stat *s, const char *title)
{
  double avg, std;

  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  printf("%s: n=%d average=%f std. deviation=%f\n", title, s->n, avg, std);
}

/******************************************************************************/
void
add_statistics(sstdisksim_stat *s, double x)
{
  s->n++;
  s->sum += x;
  s->sqr += x*x;
}

/******************************************************************************/
/*
 * Schedule next callback at time t.
 * Note that there is only *one* outstanding callback at any given time.
 * The callback is for the earliest event.
 */
void
syssim_schedule_callback(disksim_interface_callback_t fn, 
			 SysTime t, 
			 void *ctx)
{
  sstdisksim* ptr = (sstdisksim*)ctx;
  ptr->__next_event = t;
}


/******************************************************************************/
/*
 * de-scehdule a callback.
 */
void
syssim_deschedule_callback(double t, void *ctx)
{
  sstdisksim* ptr = (sstdisksim*)ctx;
  ptr->__next_event = -1;
}

/******************************************************************************/
void
syssim_report_completion(SysTime t, struct disksim_request *r, void *ctx)
{
  sstdisksim* ptr = (sstdisksim*)ctx;
  ptr->__completed = 1;
  ptr->__now = t;
  add_statistics(&(ptr->__st), t - r->start);
}

/******************************************************************************/
sstdisksim::sstdisksim( ComponentId_t id,  Params_t& params ) :
  Component( id ),
  m_dbg( *new Log< DISKSIM_DBG >( "Disksim::", false ) )
{
  std::string parameterFile = "";
  std::string outputFile = "";
  long long numSectors = 0;
  std::string frequency = "";
  __id = id;
  
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

    if ( ! it->first.compare("param_file") ) 
    {
      parameterFile = it->second;
    }
    if ( ! it->first.compare("output_file") ) 
    {
      outputFile = it->second;
    }
    if ( ! it->first.compare("num_sectors") ) 
    {
      numSectors = atol((it->second).c_str());
    }

    ++it;
  }

  registerExit();
 
  __disksim = disksim_interface_initialize(parameterFile.c_str(), 
					   outputFile.c_str(),
					   syssim_report_completion,
					   syssim_schedule_callback,
					   syssim_deschedule_callback,
					   this,
					   0,
					   0);

  __completed = 1;
  __now = 0;
  __next_event = -1;
  
  EventHandler_t* linkHandler = new EventHandler<sstdisksim,bool,Event*>
    (this,&sstdisksim::handleEvent);
  link = LinkAdd( "link", linkHandler );

  registerTimeBase("1ns");

  printf("Starting disksim up\n");

  return;
}

/******************************************************************************/
sstdisksim::~sstdisksim()
{
}

/******************************************************************************/
int
sstdisksim::Setup()
{
  sstdisksim_event* event = new sstdisksim_event();

  if ( __id == 0 )
  {
    event->etype = READ;
    event->id = 0;
    event->addr = 0;
    link->Send(0, event);
  }

  return 0;
}

/******************************************************************************/
int 
sstdisksim::Finish()
{
  disksim_interface_shutdown(__disksim, __now);

  printf("Shutting sstdisksim down\n");

  return 0;
}

/******************************************************************************/
void 
sstdisksim::readBlock(unsigned id, uint64_t addr, uint64_t clockcycle)
{
  struct disksim_request* r = new(struct disksim_request);
  r->start = __now;
  r->flags = DISKSIM_READ;
  r->devno = 0;
  r->bytecount = SECTOR;
  r->blkno = addr;

  disksim_interface_request_arrive(__disksim, __now, r);

  while(__next_event >= 0) 
  {
    __now = __next_event;
    __next_event = -1;
    disksim_interface_internal_event(__disksim, __now, 0);
  }
  
  if (!__completed) 
  {
    fprintf(stderr,
	    "disksim sim: internal error. Last event not completed\n");
    exit(1);
  }
}

/******************************************************************************/
void 
sstdisksim::writeBlock(unsigned id, uint64_t addr, uint64_t clockcycle)
{
  struct disksim_request* r = new(struct disksim_request);
  r->start = __now;
  r->flags = DISKSIM_WRITE;
  r->devno = 0;
  r->bytecount = SECTOR;
  r->blkno = addr;

  disksim_interface_request_arrive(__disksim, __now, r);

  while(__next_event >= 0) 
  {
    __now = __next_event;
    __next_event = -1;
    disksim_interface_internal_event(__disksim, __now, 0);
  }
  
  if (!__completed) 
  {
    fprintf(stderr,
	    "disksim sim: internal error. Last event not completed\n");
    exit(1);
  }
}

/******************************************************************************/
bool 
sstdisksim::handleEvent(Event* ev)
{
  sstdisksim_event* event = static_cast<sstdisksim_event*>(ev);
  if ( event->etype == READ )
    readBlock(event->id, event->addr, 0);
  else
    writeBlock(event->id, event->addr, 0);

  link->Send(0, event);

  unregisterExit();

  return false;
}

//BOOST_CLASS_EXPORT(sstdisksim_event)
//BOOST_CLASS_EXPORT(sstdisksim)
/******************************************************************************/
static Component*
create_sstdisksim(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new sstdisksim( id, params );
}

/******************************************************************************/
static const ElementInfoComponent components[] = {
    { "sstdisksim",
      "sstdisksim driver",
      NULL,
      create_sstdisksim
    },
    { NULL, NULL, NULL, NULL }
};

/******************************************************************************/
extern "C" 
{
  ElementLibraryInfo sstdisksim_eli = {
    "sstdisksim",
    "sstdisksim serilaization",
    components
  };
}
/*extern "C" {
  sstdisksim* sstdisksimAllocComponent( SST::ComponentId_t id,
					SST::Component::Params_t& params )
  {
    return new sstdisksim( id, params );
  }
  }*/

