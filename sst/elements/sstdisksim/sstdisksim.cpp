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

#include "sstdisksim.h"
#include "sst/core/element.h"
#include <sst/core/timeConverter.h>

#define	BLOCK	4096
#define	SECTOR	512
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

#define DBG( fmt, args... ) \
    __dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

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
  r->completed = 1;
  ptr->__now = t;
  add_statistics(&(ptr->__st), t - r->start);
}

/******************************************************************************/
sstdisksim::sstdisksim( ComponentId_t id,  Params& params ) :
  Component( id ),
  __dbg( *new Log< DISKSIM_DBG >( "Disksim::", false ) )
{
  std::string parameterFile = "";
  std::string outputFile = "";
  long long numSectors = 0;
  __id = id;
  __done = 0;
  __st.n = 0;
  __st.sum = 0;
  __st.sqr = 0;
  __event_total = 1;
  __cycle = 0;
  
  if ( params.find( "debug" ) != params.end() ) 
  {
    if ( params[ "debug" ].compare( "yes" ) == 0 ) 
    {
      __dbg.enable();
    }
  } 

  Params::iterator it = params.begin();
  while( it != params.end() ) 
  {
    DBG("key=%s value=%s\n",
        it->first.c_str(),it->second.c_str());

    if ( ! it->first.compare("param_file") ) 
    {
      parameterFile = it->second;
    }
    else if ( ! it->first.compare("output_file") ) 
    {
      outputFile = it->second;
    }
    else if ( ! it->first.compare("num_sectors") ) 
    {
      numSectors = atol((it->second).c_str());
    }

    ++it;
  }

  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
 
  __disksim = disksim_interface_initialize(parameterFile.c_str(), 
					   outputFile.c_str(),
					   syssim_report_completion,
					   syssim_schedule_callback,
					   syssim_deschedule_callback,
					   this,
					   0,
					   0);

  __now = 0;
  __next_event = -1;

  __tc = registerTimeBase("1ps");
  link = configureLink("link",  
		       new Event::Handler<sstdisksim>(this,&sstdisksim::handleEvent));
  if ( link == NULL )
  {
    char link_name[256];
    sprintf(link_name, "link%d", id);
    link = configureLink(link_name, 
			 new Event::Handler<sstdisksim>(this,&sstdisksim::handleEvent));
  }


  lockstep = configureSelfLink("lockstep",
			       __tc,
			       new Event::Handler<sstdisksim>(this,&sstdisksim::lockstepEvent));

  DBG("Starting disksim up\n");

  return;
}

/******************************************************************************/
sstdisksim::~sstdisksim()
{
}

/******************************************************************************/
void sstdisksim::setup()  
{
}

/******************************************************************************/
void sstdisksim::finish()
{
  disksim_interface_shutdown(__disksim, __now);

  DBG("Shutting sstdisksim down\n");

}

/******************************************************************************/
void
sstdisksim::handleEvent(Event* event)
{
  SysTime tmp = __now;
  struct disksim_request r;
  sstdisksim_event* ev = static_cast<sstdisksim_event*>(event);

  memset(&r, 0, sizeof(struct disksim_request));

  unsigned long sector;
  unsigned long nbytes;

  sector = ev->pos/SECTOR;
  nbytes = (ev->pos % SECTOR) + ev->count;

  if ( ev->etype == DISKSIMREAD )
      r.flags = DISKSIM_READ;
  else if ( ev->etype == DISKSIMWRITE)
    r.flags = DISKSIM_WRITE;
  else if ( ev->etype == DISKSIMEND)
  {
    lockstep->send(__cycle+100, ev); 
    return;
  }
  else
    abort();

  if ( ev->count <= 0 )
    return;

  r.completed = 0;
  r.start = __now;
  r.devno = ev->devno;
  r.bytecount = ceil((double)ev->count/(double)SECTOR)*SECTOR;
  r.blkno = sector;

  disksim_interface_request_arrive(__disksim, __now, &r);

  while( __next_event >= 0 ) 
  {
    __now = __next_event;
    __next_event = -1;
    disksim_interface_internal_event(__disksim, __now, 0);
  }
  
  if ( ! r.completed ) 
  {
    fprintf(stderr,
	"disksim sim: internal error. Last event not completed\n");
    abort();
    return;
  }

  double cyclespermillisec = 1000000;
  
  __event_total++;
  __cycle += (long)((__now-tmp)*cyclespermillisec);
  lockstep->send(__cycle, ev); 
  
  return;
}

/******************************************************************************/
void
sstdisksim::lockstepEvent(Event* ev)
{
  static int event_count=0;
  static int done_count=100;

  sstdisksim_event* event = static_cast<sstdisksim_event*>(ev);
  event->completed = true;

  Cycle_t now = __tc->convertToCoreTime( getCurrentSimTime(__tc) );

  unsigned long sector;
  unsigned long nbytes;

  sector = event->pos/SECTOR;
  nbytes = (event->pos % SECTOR) + event->count;

#ifdef DISKSIM_DBG
  if ( event->etype == DISKSIMEND )
    DBG( "lockstepEvent called on end event. %lu\n", now );
  if ( event->etype == DISKSIMWRITE )
    DBG( "lockstepEvent called on write event.  %lu\n", now );
  if ( event->etype == DISKSIMREAD )
    DBG( "lockstepEvent called on read event.  %lu\n", now );
#endif
  
  if ( event->etype != DISKSIMEND )
  {
    event_count++;
    event->finishedCall(event, (long long)now);
  }

  __event_total--;

  
  if ( __event_total == 0 )
  {
    if ( ! __done )
    {
      __done = true;
      primaryComponentOKToEndSim();
      print_statistics(&__st, "response time");
      DBG("time: %f milliseconds\n", __now);
      printf("events processed: %d\n", event_count);
    }
  }

  delete(event);
  return;
}

/******************************************************************************/
static Component*
create_sstdisksim(SST::ComponentId_t id, 
                  SST::Params& params)
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
    "sstdisksim serialization",
    components,
  };
}
