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

#ifndef SST_SCHEDULER_EASYSCHEDULER_H__
#define SST_SCHEDULER_EASYSCHEDULER_H__

#include <functional>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <map>
#include <set>
#include <string>
#pragma clang diagnostic pop

#include "Scheduler.h"


namespace SST {
    namespace Scheduler {

        class EASYScheduler : public Scheduler {
            private:

                static const int numCompTableEntries;
                enum ComparatorType {  //to represent type of JobComparator
                    FIFO = 0,
                    LARGEFIRST = 1,
                    SMALLFIRST = 2,
                    LONGFIRST = 3,
                    SHORTFIRST = 4,
                    BETTERFIT = 5
                };

                struct compTableEntry {
                    ComparatorType val;
                    std::string name;
                };

                static const compTableEntry compTable[6];


                std::string compSetupInfo;
                void giveGuarantee(unsigned long time, const Machine & mach);
                unsigned long lastGuarantee;
                unsigned long guaranteedStart;
                long prevFirstJobNum;

            public:

                class JobComparator : public std::binary_function<Job*,Job*,bool> {
                    public:
                        static JobComparator* Make(std::string typeName);  //return NULL if name is invalid
                        static void printComparatorList(std::ostream& out);  //print list of possible comparators
                        bool operator()(Job*& j1, Job*& j2);
                        bool operator()(Job* const& j1, Job* const& j2);
                        std::string toString();
                        JobComparator(JobComparator* incomp) { 
                           type = incomp -> type;
                        }
                    private:
                        JobComparator(ComparatorType type);
                        ComparatorType type;
                };


                class RunningInfo : public std::binary_function<RunningInfo*, RunningInfo*,bool> {
                    public:
                        long jobNum;
                        int numNodes;
                        unsigned long estComp;
                        RunningInfo(RunningInfo* inRI) 
                        {
                            jobNum = inRI -> jobNum;
                            numNodes = inRI -> numNodes;
                            estComp = inRI -> estComp;
                        }
                        RunningInfo() 
                        {
                        } 
                        bool operator()(RunningInfo* r1, RunningInfo* r2)
                        {
                            if(r1->estComp != r2->estComp) {
                                return r1->estComp < r2->estComp;
                            } else {
                                return r1->jobNum < r2->jobNum;
                            }
                        }
                };

                virtual ~EASYScheduler() {
                    delete toRun;
                    delete running;
                    delete comp;
                }

                EASYScheduler(JobComparator* comp);
                EASYScheduler(EASYScheduler* insched, std::set<Job*, JobComparator>* newrunning, std::multiset<RunningInfo*, RunningInfo>* newtoRun);

                std::string getSetupInfo(bool comment);

                void jobArrives(Job* j, unsigned long time, const Machine & mach);
                void jobFinishes(Job* j, unsigned long time, const Machine & mach);

                Job* tryToStart(unsigned long time, const Machine & mach);
                void startNext(unsigned long time, const Machine & mach);

                void reset();

                EASYScheduler* copy(std::vector<Job*>* running, std::vector<Job*>* toRun);

            protected:
                //need to use a set instead of a priority queue to suppport iteration
                std::set<Job*, JobComparator>* toRun;  //jobs waiting to run
                JobComparator * comp;
                
                //keeps track of running jobs in order based on their estimated
                //completion time.  Must use multi in case jobs end at same
                //time (careful not to erase jobs by key = finishing time)
                std::multiset<RunningInfo*, RunningInfo>* running; 
        };
    }
}
#endif
