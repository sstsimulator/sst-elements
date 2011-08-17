// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                      
         
#include "sst/core/serialization/element.h"

#include "ArrivalEvent.h"
#include "Machine.h"
#include "Allocator.h"
#include "AllocInfo.h"
#include "Scheduler.h"
#include "Statistics.h"
#include "Job.h"

long ArrivalEvent::getTime() const {
  return time;
}

int ArrivalEvent::getJobIndex() const {
  return jobIndex;
}

void ArrivalEvent::happen(Machine* mach, Allocator* alloc, Scheduler* sched,
			  Statistics* stats, Job* arrivingJob) {
  sched -> jobArrives(arrivingJob, time);
  stats -> jobArrives(time);

  //tries to start job                                                                    
  AllocInfo* allocInfo;
  do {
    allocInfo = sched -> tryToStart(alloc, time, mach, stats);
  } while(allocInfo != NULL);
}
