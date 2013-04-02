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

/*
 * Schedulers based around priority queues; jobs run in order given by
 * some comparator without any backfilling.
 */

#include <functional>
#include <string>
#include <queue>
#include <vector>
using namespace std;

#include "sst/core/serialization/element.h"

#include "PQScheduler.h"
//#include "Factory.h"
#include "Allocator.h"
#include "Job.h"
#include "misc.h"

using namespace SST::Scheduler;


const PQScheduler::compTableEntry PQScheduler::compTable[] = {
  { FIFO, "fifo"},
  { LARGEFIRST, "largefirst"},
  { SMALLFIRST, "smallfirst"},
  { LONGFIRST, "longfirst"},
  { SHORTFIRST, "shortfirst"},
  {BETTERFIT, "betterfit"}};

const int PQScheduler::numCompTableEntries = 6;

PQScheduler::PQScheduler(JobComparator* comp) {
  toRun = new priority_queue<Job*,vector<Job*>,JobComparator>(*comp);
  compSetupInfo = comp -> toString();
}

void usage();

/*
   Scheduler* PQScheduler::Make(vector<string>* params) {
   argsAtLeast(0, params);
   argsAtMost(1, params);

   if(params -> size() == 1)
   return new PQScheduler(JobComparator::Make("fifo"));
   else {
   JobComparator* comp = JobComparator::Make((*params)[1]);
   if(comp == NULL)
   usage();
   return new PQScheduler(comp);
   }
   }

   string PQScheduler::getParamHelp(){
   return "[<opt_comp>]\n\topt_comp: Comparator to use, defaults to fifo";
   }
 */

string PQScheduler::getSetupInfo(bool comment) {
  string com;
  if(comment)
    com="# ";
  else
    com="";
  return com + "Priority Queue Scheduler\n" + com +
    "Comparator: " + compSetupInfo; 
}

void PQScheduler::jobArrives(Job* j, unsigned long time, Machine* mach) {
  //called when j arrives; time is current time
  //tryToStart should be called after each job arrives

  toRun -> push(j);
}

AllocInfo* PQScheduler::tryToStart(Allocator* alloc, unsigned long time,
    Machine* mach,
    Statistics* stats) {
  //allows the scheduler to start a job if desired; time is current time
  //called after calls to jobArrives and jobFinishes
  //(either after each call or after each call occuring at same time)
  //returns first job to start, NULL if none
  //(if not NULL, should call tryToStart again)

  if(toRun -> size() == 0) 
    return NULL;

  AllocInfo* allocInfo = NULL;
  Job* job = toRun -> top();
  if (alloc -> canAllocate(job)) 
    allocInfo = alloc -> allocate(job);
  if(allocInfo != NULL) {
    toRun -> pop();  //remove the job we just allocated
    job -> start(time, mach, allocInfo, stats);
  }
  return allocInfo;
}

  void PQScheduler::reset() {
    while(!toRun -> empty())
      toRun -> pop();
  }


PQScheduler::JobComparator::JobComparator(ComparatorType type) {
  this -> type = type;
}
void PQScheduler::JobComparator::printComparatorList(ostream& out) {
  for(int i=0; i<numCompTableEntries; i++)
    out << "  " << compTable[i].name << endl;
}

PQScheduler::JobComparator* PQScheduler::JobComparator::Make(string typeName) {
  for(int i=0; i<numCompTableEntries; i++)
    if(typeName == compTable[i].name)
      return new JobComparator(compTable[i].val);
  return NULL;
}

void internal_error(string mesg);

bool PQScheduler::JobComparator::operator()(Job*& j1, Job*& j2) {
  switch(type) {
    case FIFO:
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j2 -> getArrivalTime() < j1 -> getArrivalTime();
      return j2 -> getJobNum() < j1 -> getJobNum();
    case LARGEFIRST:
      //largest job goes first if they are different size
      if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
        return (j2 -> getProcsNeeded() > j1 -> getProcsNeeded());

      //secondary criteria: earlier arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() > j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() < j1 -> getJobNum();
    case SMALLFIRST:
      //smaller job goes first if they are different size
      if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
        return (j2 -> getProcsNeeded() < j1 -> getProcsNeeded());

      //secondary criteria: earlier arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() > j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() < j1 -> getJobNum();
    case LONGFIRST:
      //longest job goes first if different lengths
      if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime())
        return j2 -> getEstimatedRunningTime() > j1 -> getEstimatedRunningTime();

      //secondary criteria: earliest arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() > j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() < j1 -> getJobNum();
    case SHORTFIRST:
      //longest job goes first if different lengths
      if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime())
        return j2 -> getEstimatedRunningTime() < j1 -> getEstimatedRunningTime();

      //secondary criteria: earliest arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() > j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() < j1 -> getJobNum();

    case BETTERFIT:
      //Primary criteria: Most Processors Required
      if(j1->getProcsNeeded() != j2->getProcsNeeded())
        return (j2->getProcsNeeded() > j1->getProcsNeeded());

      //Secondary criteria: Longest Run Time
      if(j1->getEstimatedRunningTime() != j2->getEstimatedRunningTime()) {
        return (j2->getEstimatedRunningTime() > j1->getEstimatedRunningTime());
      }

      //Tertiary criteria: Arrival Time
      if(j1->getArrivalTime() != j2->getArrivalTime()) {
        return  (j2->getArrivalTime() < j1->getArrivalTime());
      }

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() < j1 -> getJobNum();

    default:
      internal_error("operator() called on JobComparator w/ invalid type");
      return true;  //never reach here
  }
}

string PQScheduler::JobComparator::toString() {
  switch(type){
    case FIFO:
      return "FIFOComparator";
    case LARGEFIRST:
      return "LargestFirstComparator";
    case SMALLFIRST:
      return "SmallestFirstComparator";
    case LONGFIRST:
      return "LongestFirstComparator";
    case SHORTFIRST:
      return "ShortestFirstComparator";
    case BETTERFIT:
      return "BetterFitComparator";     
    default:
      internal_error("toString() called on JobComparator w/ invalid type");
  }
      return "";  //never reach here...
}


