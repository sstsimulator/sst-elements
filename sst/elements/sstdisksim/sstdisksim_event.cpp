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

#include "sstdisksim_event.h"
#include "sst/core/element.h"
#include <sst/core/timeConverter.h>

/******************************************************************************/
void 
opFinishedCallback(sstdisksim_event* event, long long now)
{
   unsigned long sector;
   unsigned long nblks;
   unsigned long nbytes;

   sector = event->pos/512;
   nbytes = (event->pos % 512) + event->count;
   nblks = (unsigned long)ceill((double)nbytes/(double)512);
   
   static int ___lockStepEventPrint__ = 0;
   
   if ( !___lockStepEventPrint__ )
   {
     ___lockStepEventPrint__ = 1;
     printf("time\t\t pos\t\t sector\t\t nbytes\t\t nblks\n");
   }   
   
   printf("%ld\t\t%lu\t\t%lu\t\t%lu%\t\t%lu\n", 
	  (long)now,
	  event->pos, 
	  sector,
	  nbytes, 
	  nblks);
}

/******************************************************************************/
sstdisksim_event::sstdisksim_event()
  :SST::Event()
{
  finishedCall = &opFinishedCallback;
}
