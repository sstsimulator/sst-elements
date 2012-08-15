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

#ifndef __PQSCHEDULER_H__
#define __PQSCHEDULER_H__

#include <string>
#include <vector>
#include <functional>
#include "Scheduler.h"
using namespace std;

class PQScheduler : public Scheduler {
  private:

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

    static const compTableEntry compTable[6] ;


    static const int numCompTableEntries;

    string compSetupInfo;
  public:
    virtual ~PQScheduler() { delete toRun;}

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

    PQScheduler(JobComparator* comp);

    //static Scheduler* Make(vector<string>* params);
    //static string getParamHelp();
    string getSetupInfo(bool comment);

    void jobArrives(Job* j, unsigned long time, Machine* mach);

    void jobFinishes(Job* j, unsigned long time, Machine* mach){}

    AllocInfo* tryToStart(Allocator* alloc, unsigned long time, Machine* mach,
        Statistics* stats);

    void reset();
  protected:
    priority_queue<Job*,vector<Job*>,JobComparator>* toRun;  //jobs waiting to run
};

#endif
