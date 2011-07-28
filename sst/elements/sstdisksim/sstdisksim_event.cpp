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
  //static int ___lockStepEventPrint__ = 0;
  
  unsigned long sector;
  
  sector = event->pos/512;

  static long long before = 0;

  
  //if ( !___lockStepEventPrint__ )
  //{
  //   ___lockStepEventPrint__ = 1;
  //   printf("time\t\t pos\t\t sector\t\t nbytes\n");
  //}   
  
  static int ___x = 0;
  printf("%d\t\t%lld\t\t%lu\t\t%llu\t\t%d\n", 
	 ___x,
	 now-before,
	 sector,
	 event->pos, 
	 event->count);
  ___x++;
  before = now;
}

/******************************************************************************/
sstdisksim_event::sstdisksim_event()
  :SST::Event()
{
  finishedCall = &opFinishedCallback;
}
