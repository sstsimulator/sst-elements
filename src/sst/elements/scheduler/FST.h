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
 * Computes the FST for each job that comes in
 */

#ifndef SST_SCHEDULER_FST_H__
#define SST_SCHEDULER_FST_H__

#include <vector>
#include <map>

namespace SST {
    namespace Scheduler {
        class Scheduler;
        class Machine;
        class Allocator;
        class Job;
        class Statistics;
        class TaskMapInfo;

        class FST {
            private:
                std::vector<Job*>* running;
                std::vector<Job*>* toRun;
                int numjobs;
                unsigned long* jobFST; //array to hold the FST values for jobs 1....numjobs
                bool relaxed;

            public:
                void jobArrives(Job* j, Scheduler* insched, Machine* inmach);
                void jobCompletes(Job* j);
                void jobStarts(Job* j, unsigned long time);
                FST(int inrelaxed); 
                bool FSTstart(std::multimap<Job*, unsigned long, bool(*)(Job*, Job*)>* endtimes, 
                              std::map<Job*, TaskMapInfo*>* jobToAi, Job* j, Scheduler* sched,
                              Allocator* alloc, Machine* mach, Statistics* stats, unsigned long time);
                void setup(int numjobs);
                unsigned long getFST(int num);
        };

    }
}
#endif
