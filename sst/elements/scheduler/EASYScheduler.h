// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __EASYSCHEDULER_H__
#define __EASYSCHEDULER_H__

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include "Scheduler.h"

using namespace std;

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
                    string name;
                };

                static const compTableEntry compTable[6];


                string compSetupInfo;
                void giveGuarantee(unsigned long time, Machine* mach);
                unsigned long lastGuarantee;
                unsigned long guaranteedStart;
                long prevFirstJobNum;
                AllocInfo* doesntDisturbFirst(Allocator* alloc, Job* j, Machine* mach, unsigned long time);

            public:

                class JobComparator : public binary_function<Job*,Job*,bool> {
                    public:
                        static JobComparator* Make(string typeName);  //return NULL if name is invalid
                        static void printComparatorList(ostream& out);  //print list of possible comparators
                        bool operator()(Job*& j1, Job*& j2);
                        bool operator()(Job* const& j1, Job* const& j2);
                        string toString();
                    private:
                        JobComparator(ComparatorType type);
                        ComparatorType type;
                };
                EASYScheduler(JobComparator* comp);

                virtual ~EASYScheduler() {
                    delete toRun;
                    delete running;
                    delete comp;
                }

                class RunningInfo : public binary_function<RunningInfo*, RunningInfo*,bool> {
                    public:
                        long jobNum;
                        int numProcs;
                        unsigned long estComp;
                        bool operator()(RunningInfo* r1, RunningInfo* r2){
                            if(r1->estComp != r2->estComp) {
                                return r1->estComp < r2->estComp;
                            } else {
                                return r1->jobNum < r2->jobNum;
                            }
                        }
                };

                string getSetupInfo(bool comment);

                void jobArrives(Job* j, unsigned long time, Machine* mach);
                void jobFinishes(Job* j, unsigned long time, Machine* mach);

                AllocInfo* tryToStart(Allocator* alloc, unsigned long time, Machine* mach,
                                      Statistics* stats);

                void reset();

            protected:
                //need to use a set instead of a priority queue to suppport iteration
                set<Job*, JobComparator>* toRun;  //jobs waiting to run
                JobComparator * comp;
                multiset<RunningInfo*, RunningInfo>* running; //keeps track of running jobs in order based on their estimated completion time.  Must use multi in case jobs end at same time (careful not to erase jobs by key = finishing time)


        };

    }
}
#endif
