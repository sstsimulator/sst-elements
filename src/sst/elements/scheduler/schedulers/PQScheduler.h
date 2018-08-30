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

#ifndef SST_SCHEDULER_PQSCHEDULER_H__
#define SST_SCHEDULER_PQSCHEDULER_H__

#include <string>
#include <vector>

#include "Scheduler.h"


namespace SST {
    namespace Scheduler {
        class Job;

        class PQScheduler : public Scheduler {
            private:

                enum ComparatorType{  //to represent type of JobComparator
                    FIFO = 0,
                    LARGEFIRST = 1,
                    SMALLFIRST = 2,
                    LONGFIRST = 3,
                    SHORTFIRST = 4,
                    BETTERFIT = 5
                };

                struct compTableEntry{
                    ComparatorType val;
                    std::string name;
                };

                static const compTableEntry compTable[6];


                static const int numCompTableEntries;

                std::string compSetupInfo;


            public:
                virtual ~PQScheduler() 
                {
                    delete toRun;
                }
                PQScheduler* copy(std::vector<Job*>* running, std::vector<Job*>* toRun);

                class JobComparator : public std::binary_function<Job*,Job*,bool> {
                public:
                    static JobComparator* Make(std::string typeName);  //return NULL if name is invalid
                    static void printComparatorList(std::ostream& out);  //print list of possible comparators
                    bool operator()(Job*& j1, Job*& j2);
                    bool operator()(Job* const& j1, Job* const& j2);
                    std::string toString();

                private:
                    JobComparator(ComparatorType type);
                    ComparatorType type;
                };
                JobComparator* origcomp;

                PQScheduler(JobComparator* comp);
                PQScheduler(PQScheduler* insched, std::priority_queue<Job*,std::vector<Job*>,JobComparator>* intoRun);

                std::string getSetupInfo(bool comment);

                void jobArrives(Job* j, unsigned long time, const Machine & mach);

                void jobFinishes(Job* j, unsigned long time, const Machine & mach){ }

                Job* tryToStart(unsigned long time, const Machine & mach);
                void startNext(unsigned long time, const Machine & mach);

                void reset();

            protected:
                std::priority_queue<Job*,std::vector<Job*>,JobComparator>* toRun;  //jobs waiting to run
        };

    }
}
#endif
