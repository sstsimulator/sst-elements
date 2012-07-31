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
#include <set>
#include <iostream>
using namespace std;

#include "sst/core/serialization/element.h"

#include "EASYScheduler.h"
//#include "Factory.h"
#include "Allocator.h"
#include "Job.h"
#include "misc.h"
#include "Machine.h"

const EASYScheduler::compTableEntry EASYScheduler::compTable[5] = {
  { FIFO, "fifo"},
  { LARGEFIRST, "largefirst"},
  { SMALLFIRST, "smallfirst"},
  { LONGFIRST, "longfirst"},
  { SHORTFIRST, "shortfirst"}};
const int EASYScheduler::numCompTableEntries = 5;

EASYScheduler::EASYScheduler(JobComparator* comp) { 
  toRun = new set< Job*,JobComparator, std::allocator<Job*> >(*comp);
  running = new multimap<long, Job*>(); //don't need to pass comp because compare longs
  compSetupInfo = comp -> toString();
  prevFirstJobNum = -1;
  guaranteedStart = 0;
  lastGuarantee = 0;
}

void usage();

string EASYScheduler::getSetupInfo(bool comment) {
  string com;
  if(comment)
    com="# ";
  else
    com="";
  return com + "EASY Scheduler\n" + com +
    "Comparator: " + compSetupInfo; 
}

void EASYScheduler::jobArrives(Job* j, long time, Machine* mach) {
  //called when j arrives; time is current time
  //tryToStart should be called after each job arrives
  toRun -> insert(j);


  set<Job*, JobComparator>::iterator firstJob = toRun->begin();
  //if it's the first job give a time guarantee
  if(j->getJobNum() == (*(firstJob))->getJobNum())
  {
    giveGuarantee(time, mach);
  }
}

void EASYScheduler::jobFinishes(Job* j, long time, Machine* mach){
  //remove j from the list of running jobs and update start

  multimap<long, Job*>::iterator it = running->begin();
  bool success = false;
  while(!success && it != running->end())
  {  
    if(it->second == j)
    {
      running->erase(it);
      success = true;
    }
    it++;
  }
  if(!success)
    error("Could not find finishing job in running list");
  giveGuarantee(time, mach);
}

AllocInfo* EASYScheduler::tryToStart(Allocator* alloc, long time,
    Machine* mach,
    Statistics* stats) {
  //allows the scheduler to start a job if desired; time is current time
  //called after calls to jobArrives and jobFinishes
  //(either after each call or after each call occuring at same time)
  //returns first job to start, NULL if none
  //(if not NULL, should call tryToStart again)
  if(!running->empty() && running->begin()->first == time)
    return NULL;  //don't backfill if another job is about to finish

  if(toRun->empty()) 
    return NULL;

  bool succeeded = false;  //whether we found a job to allocate
  bool first = false; //whether it was the first job

  AllocInfo* allocInfo = NULL;
  set<Job*, JobComparator>::iterator job = toRun -> begin();
  if(time > guaranteedStart)
  {
    char errorstring[55];
    sprintf(errorstring, "Failed to start job #%ld at guaranteed time", (*job)->getJobNum());
    error(errorstring);
  }
  if (alloc -> canAllocate(*job)) 
  {
    succeeded = true;
    first = true;
  }
  else
  {
    job++; 
  }

  while(!succeeded && job != toRun->end())
  {
    if(alloc->canAllocate(*job))
    {
      //need to make sure first job can still start at guaranteed time
      allocInfo = doesntDisturbFirst(alloc,*job,mach,time);
      if(allocInfo != NULL)
      {
        succeeded = true;
      }
    }
    if(!succeeded)
      job++;
  }
  //allocate job if found
  if(succeeded)
  {
    if(allocInfo == NULL)
    {
      allocInfo = alloc->allocate(*job);
    }
    toRun->erase(job); //remove the job from toRun list
    pair<long, Job*> temp = pair<long, Job*>(time + (*job)->getEstimatedRunningTime(),*job);
    multimap<long, Job*>::iterator justinserted = running->insert(temp); //add to running list       
    (justinserted->second)->start(time,mach,allocInfo,stats);
    if(first) //update the guarantee if starting the first job
      giveGuarantee(time, mach);      
    return allocInfo;
  }
  return NULL;
}

void EASYScheduler::reset() {
  toRun -> clear();
}

void EASYScheduler::giveGuarantee(long time, Machine* mach)
{
  //gives a guaranteed start time for a job.  The first job in the queue cannot
  //be delayed past this guarantee
  if(toRun->empty())
    return;

  set<Job*, JobComparator>::iterator firstJob = toRun->begin();
  long lastGuarantee = guaranteedStart;
  bool succeeded = false;

  int size = (*firstJob)->getProcsNeeded();
  int free = mach->getNumFreeProcessors();

  if(free >= size)
  {
    guaranteedStart = time;
    succeeded = true;
  }

  int futureFree = free;
  multimap<long, Job*>::iterator it = running->begin();

  while(!succeeded && it != running->end())
  {
    futureFree += it->second->getProcsNeeded();
    if(futureFree >= size)
    {
      guaranteedStart = it->first;
      succeeded = true;
    }
    it++;
  }
  if(succeeded)
  {
    if((*firstJob)->getJobNum() == prevFirstJobNum && lastGuarantee < guaranteedStart && lastGuarantee > 0)
    {

      error("EASY scheduler gave new guarantee worse than previous\n");
    }
    prevFirstJobNum = (*firstJob)->getJobNum();
  }
  else{
    char errorstring[70];
    sprintf(errorstring, "EASY unable to make reservation for first job (%ld)",(*firstJob)->getJobNum());
    error(errorstring); 
  }
}

AllocInfo* EASYScheduler::doesntDisturbFirst(Allocator* alloc, Job* j, Machine* mach, long time){

  //returns if j would delay the first job by starting now
  if(!alloc->canAllocate(j))
    return NULL;

  AllocInfo* retVal = alloc->allocate(j);
  if(time + j->getEstimatedRunningTime() <= guaranteedStart)
    return retVal;

  int avail = mach->getNumFreeProcessors();
  multimap<long,Job*>::iterator it  = running->begin();
  while(it != running->end() && it->first <= guaranteedStart)
  {
    avail += it->second->getProcsNeeded();
    it++;
  }
  set<Job*, JobComparator>::iterator tempit = toRun->begin();
  if(avail - j->getProcsNeeded() >= (*tempit)->getProcsNeeded())
    return retVal;

  //if we made it this far it disturbs the first job
  alloc->deallocate(retVal);
  return NULL; 
}

EASYScheduler::JobComparator::JobComparator(ComparatorType type) {
  this -> type = type;
}


void EASYScheduler::JobComparator::printComparatorList(ostream& out) {
  for(int i=0; i<numCompTableEntries; i++)
    out << "  " << compTable[i].name << endl;
}

EASYScheduler::JobComparator* EASYScheduler::JobComparator::Make(string typeName) {
  for(int i=0; i<numCompTableEntries; i++)
    if(typeName == compTable[i].name)
      return new EASYScheduler::JobComparator(compTable[i].val);
  return NULL;
}

void internal_error(string mesg);

bool EASYScheduler::JobComparator::operator()(Job* const& j1,Job* const& j2) { 
  //for this to work correctly, it returns the reverse of what it would
  //in the comparators for the PQscheduler (because this uses sets and maps
  //instead of priority queues)
  switch(type) {
    case FIFO:
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j2 -> getArrivalTime() > j1 -> getArrivalTime();
      return j2 -> getJobNum() > j1 -> getJobNum();
    case LARGEFIRST:
      //largest job goes first if they are different size
      if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
        return (j2 -> getProcsNeeded() < j1 -> getProcsNeeded());

      //secondary criteria: earlier arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() < j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() > j1 -> getJobNum();
    case SMALLFIRST:
      //smaller job goes first if they are different size
      if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
        return (j2 -> getProcsNeeded() > j1 -> getProcsNeeded());

      //secondary criteria: earlier arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() < j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() > j1 -> getJobNum();
    case LONGFIRST:
      //longest job goes first if different lengths
      if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime())
        return j2 -> getEstimatedRunningTime() < j1 -> getEstimatedRunningTime();

      //secondary criteria: earliest arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() < j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() > j1 -> getJobNum();
    case SHORTFIRST:
      //longest job goes first if different lengths
      if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime())
        return j2 -> getEstimatedRunningTime() > j1 -> getEstimatedRunningTime();

      //secondary criteria: earliest arriving job first
      if(j1 -> getArrivalTime() != j2 -> getArrivalTime())
        return j1 -> getArrivalTime() < j2 -> getArrivalTime();

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() > j1 -> getJobNum();
    default:
      internal_error("operator() called on JobComparator w/ invalid type");
      return true;  //never reach here
  }
}

string EASYScheduler::JobComparator::toString() {
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
    default:
      internal_error("toString() called on JobComparator w/ invalid type");
  }
      return "";  //never reach here...
}


