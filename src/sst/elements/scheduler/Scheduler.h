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
                Scheduler() { nextToStart = NULL; }
            
                virtual ~Scheduler() {}

                virtual std::string getSetupInfo(bool comment) = 0;

                //called when j arrives; time is current time
                //tryToStart will be called after announcing all arriving jobs
                virtual void jobArrives(Job* j, unsigned long time, const Machine & mach) = 0;

                //called when j finishes; time is current time
                //tryToStart will be called after announcing all arriving jobs
                virtual void jobFinishes(Job* j, unsigned long time, const Machine & mach) = 0;

                //allows the scheduler to start a job if desired; time is current time
                //called after calls to jobArrives and jobFinishes
                //returns the job that can be started or NULL if none
                //(if not NULL, tryToStart should be called again)
                virtual Job* tryToStart(unsigned long time, const Machine & mach) = 0;
                
                //starts the next available job
                //should be called after tryToStart at the same simulation time
                virtual void startNext(unsigned long time, const Machine & mach) = 0;

                //delete stored state so scheduler can be run on new input
                virtual void reset() {}

                //tell scheduler that simulation is done so it can print information
                virtual void done() {}

                //used for FST, returns an exact copy of the current schedule
                virtual Scheduler* copy(std::vector<Job*>* running, std::vector<Job*>* toRun) = 0;
            
            protected:
                Job* nextToStart; //next ready job - used to give feedback to schedComponent
                unsigned long nextToStartTime; //used to validate tryToStart and startNext are called at the same time
        };

    }
}
#endif
