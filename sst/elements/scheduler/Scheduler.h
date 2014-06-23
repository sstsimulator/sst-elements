// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Abstract base class for schedulers
 */

#ifndef SST_SCHEDULER_SCHEDULER_H__
#define SST_SCHEDULER_SCHEDULER_H__

#include <queue>
#include <set>
#include <string> 

namespace SST {
    namespace Scheduler {

        class Job;
        class AllocInfo;
        class Allocator;
        class Machine;
        class Statistics;

        class Scheduler {
            public:
                virtual ~Scheduler() {}

                virtual std::string getSetupInfo(bool comment) = 0;

                //called when j arrives; time is current time
                //tryToStart will be called after announcing all arriving jobs
                virtual void jobArrives(Job* j, unsigned long time, Machine* mach) = 0;

                //called when j finishes; time is current time
                //tryToStart will be called after announcing all arriving jobs
                virtual void jobFinishes(Job* j, unsigned long time, Machine* mach) = 0;

                //allows the scheduler to start a job if desired; time is current time
                //called after calls to jobArrives and jobFinishes
                //returns information on job it started or NULL if none
                //(if not NULL, tryToStart will be called again)
                virtual AllocInfo* tryToStart(Allocator* alloc, unsigned long time, 
                                              Machine* mach) = 0;

                //delete stored state so scheduler can be run on new input
                virtual void reset() {}

                //tell scheduler that simulation is done so it can print information
                virtual void done() {}

                //used for FST, returns an exact copy of the current schedule
                virtual Scheduler* copy(std::vector<Job*>* running, std::vector<Job*>* toRun) = 0;
        };

    }
}
#endif
