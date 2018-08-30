// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "ArrivalEvent.h"

#include "Allocator.h"
#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "Scheduler.h"
#include "Statistics.h"

using namespace SST::Scheduler;

unsigned long ArrivalEvent::getTime() const 
{
    return time;
}

int ArrivalEvent::getJobIndex() const 
{
    return jobIndex;
}

void ArrivalEvent::happen(const Machine & mach, Allocator* alloc, Scheduler* sched,
                          Statistics* stats, Job* arrivingJob) 
{
    sched -> jobArrives(arrivingJob, time, mach);
    stats -> jobArrives(time);

    /* no longer tries to start job as we only try to start once all events that
       happen at the same time have been received and handled successfully.
       Only schedComponent knows this, so it is responsible for calling tryToStart
       for both arrival and completion events

       AllocInfo* allocInfo;
       do {
       allocInfo = sched -> tryToStart(alloc, time, mach, stats);
       } while(allocInfo != NULL);
       */
}
