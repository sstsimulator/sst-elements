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
sstdisksim::sstdisksim( ComponentId_t id,  Params_t& params ) :
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

  __now = 0;
  __next_event = -1;

  __tc = registerTimeBase("1ps");
  link = configureLink("link",  
		       new Event::Handler<sstdisksim>(this,&sstdisksim::handleEvent));
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
int
sstdisksim::Setup()
{
  return 0;
}

/******************************************************************************/
int 
sstdisksim::Finish()
{
  disksim_interface_shutdown(__disksim, __now);

  DBG("Shutting sstdisksim down\n");

  return 0;
}

/******************************************************************************/
long
sstdisksim::sstdisksim_process_event(sstdisksim_event* ev)
{
  SysTime tmp = __now;
  struct disksim_request r;
  memset(&r, 0, sizeof(struct disksim_request));

  unsigned long sector;
  unsigned long nblks;
  unsigned long nbytes;

  sector = ev->pos/SECTOR;
  nbytes = (ev->pos % SECTOR) + ev->count;
  nblks = (unsigned long)ceill((double)nbytes/(double)SECTOR);

  if ( ev->etype == DISKSIMREAD )
    r.flags = DISKSIM_READ;
  else if ( ev->etype == DISKSIMWRITE)
    r.flags = DISKSIM_WRITE;
  else
    abort();

  r.start = __now;
  r.devno = ev->devno;
  r.bytecount = nblks*SECTOR;
  r.blkno = sector;

  r.completed = 0;
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
    return -1;
  }

  tmp = __now-tmp; /* milliseconds */
  long cyclespermillisec = 1000000;

  
  __cycle += (long)(tmp*cyclespermillisec);
  lockstep->Send(__cycle, ev);

  return tmp;
}

/******************************************************************************/
void
sstdisksim::lockstepEvent(Event* ev)
{
  static int event_count=0;

  sstdisksim_event* event = static_cast<sstdisksim_event*>(ev);
  event->completed = true;

  Cycle_t now = __tc->convertToCoreTime( getCurrentSimTime(__tc) );

#ifdef DISKSIM_DBG
  if ( event->etype == DISKSIMEND )
    DBG( "lockstepEvent called on end event. %lu\n", now );
  if ( event->etype == DISKSIMWRITE )
    DBG( "lockstepEvent called on write event.  %lu\n", now );
  if ( event->etype == DISKSIMREAD )
    DBG( "lockstepEvent called on read event.  %lu\n", now );
#endif

  if ( (event->etype == DISKSIMEND && !__done) ||
       event->etype != DISKSIMEND )
  {
    __event_total--;
  }

  if ( __event_total == 0 )
  {
    if ( ! __done )
    {
      __done = true;
      unregisterExit();
      print_statistics(&__st, "response time");
      DBG("time: %f milliseconds\n", __now);

      printf("events processed: %d\n", event_count);
    }

    delete(event);
    return;
  }

  event_count++;
  event->finishedCall();
  delete(event);
}

/******************************************************************************/
void
sstdisksim::handleEvent(Event* ev)
{
  sstdisksim_event* event = static_cast<sstdisksim_event*>(ev);

  /* This sstdisksim process has been shutdown; 
     behave accordingly */
  if (event->etype == DISKSIMEND )
  {
    lockstep->Send(0, ev);
  }
  else
  {
    __event_total++;
    sstdisksim_process_event(event);
  }

  return;
}

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
    "sstdisksim serialization",
    components,
  };
}
