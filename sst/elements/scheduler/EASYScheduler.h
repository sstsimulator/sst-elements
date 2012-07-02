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

#ifndef __EASYSCHEDULER_H__
#define __EASYSCHEDULER_H__

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include "Scheduler.h"

using namespace std;

class EASYScheduler : public Scheduler {
  private:

    static const int numCompTableEntries;
    enum ComparatorType {  //to represent type of JobComparator
      FIFO = 0,
      LARGEFIRST = 1,
      SMALLFIRST = 2,
      LONGFIRST = 3,
      SHORTFIRST = 4
    };

    struct compTableEntry {
      ComparatorType val;
      string name;
    };

    static const compTableEntry compTable[5];


    string compSetupInfo;
    void giveGuarantee(long time, Machine* mach);
    long lastGuarantee;
    long guaranteedStart;
    long prevFirstJobNum;
    AllocInfo* doesntDisturbFirst(Allocator* alloc, Job* j, Machine* mach, long time);

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
    }

    string getSetupInfo(bool comment);

    void jobArrives(Job* j, long time, Machine* mach);
    void jobFinishes(Job* j, long time, Machine* mach);

    AllocInfo* tryToStart(Allocator* alloc, long time, Machine* mach,
        Statistics* stats);

    void reset();

  protected:
    //need to use a set instead of a priority queue to suppport iteration
    set<Job*, JobComparator>* toRun;  //jobs waiting to run
    multimap<long, Job*>* running; //keeps track of running jobs in order based on their estimated completion time.  Must use multi in case jobs end at same time (careful not to erase jobs by key = finishing time)


};

#endif
