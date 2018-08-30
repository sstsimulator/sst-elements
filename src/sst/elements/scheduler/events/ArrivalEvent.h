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

/*
 * Classes representing system events
 */

#ifndef SST_SCHEDULER_ARRIVALEVENT_H__
#define SST_SCHEDULER_ARRIVALEVENT_H__

#include <sst/core/event.h>

namespace SST {
    namespace Scheduler {

        class Machine;
        class Allocator;
        class Scheduler;
        class Statistics;
        class Job;

        class ArrivalEvent : public SST::Event {
            public:

                ArrivalEvent(unsigned long time, int jobIndex) : SST::Event() {
                    this -> time = time;
                    this -> jobIndex = jobIndex;
                }

                virtual ~ArrivalEvent() {}

                virtual void happen(const Machine & mach, Allocator* alloc, Scheduler* sched,
                                    Statistics* stats, Job* arrivingJob);

                unsigned long getTime() const;

                int getJobIndex() const;

            protected:

                unsigned long time;   //when the event occurs

            private:

                int jobIndex;

                NotSerializable(ArrivalEvent)
        };

    }
}
#endif

