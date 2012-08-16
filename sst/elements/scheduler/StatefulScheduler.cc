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
#include <set>
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;
#include "sst/core/serialization/element.h"
#include "StatefulScheduler.h"
#include "Job.h"
#include "Allocator.h"
#include "misc.h"
#include "Machine.h"

const StatefulScheduler::compTableEntry StatefulScheduler::compTable[6] = {
  { FIFO, "fifo"},
  { LARGEFIRST, "largefirst"},
  { SMALLFIRST, "smallfirst"},
  { LONGFIRST, "longfirst"},
  { SHORTFIRST, "shortfirst"},
  {BETTERFIT, "betterfit"}};
const int StatefulScheduler::numCompTableEntries = 6;

//each manager has a different constructor for stateful scheduler
StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, bool dummy){
  //"dummy" variable distinguishes  between constructors; does not do anything and its value does not matter
  SCComparator *sccomp = new SCComparator();
  numProcs = numprocs;
  freeProcs = numprocs;
  estSched = new set<SchedChange*, SCComparator>(*sccomp);
  jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
  heart = new ConservativeManager(this);
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, int filltimes){
  SCComparator *sccomp = new SCComparator();
  numProcs = numprocs;
  freeProcs = numprocs;
  estSched = new set<SchedChange*, SCComparator>(*sccomp);
  jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
  heart = new PrioritizeCompressionManager(this, comp, filltimes);
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp){
  SCComparator *sccomp = new SCComparator();
  numProcs = numprocs;
  freeProcs = numprocs;
  estSched = new set<SchedChange*, SCComparator>(*sccomp);
  jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
  heart = new DelayedCompressionManager(this, comp); 
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, int filltimes, bool dummy){
  SCComparator *sccomp = new SCComparator();
  numProcs = numprocs;
  freeProcs = numprocs;
  estSched = new set<SchedChange*, SCComparator>(*sccomp);
  jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
  heart = new EvenLessManager(this, comp, filltimes);
}


void usage();

string StatefulScheduler::getSetupInfo(bool comment) {
  string com;
  if(comment)
    com="# ";
  else
    com="";
  string heartstring = heart->getString();
  return com + heartstring + com +
    "Comparator: " + compSetupInfo; 
}
string StatefulScheduler::DelayedCompressionManager::getString(){return "DelayedCompression Less Conservative Scheduler\n"; }
string StatefulScheduler::ConservativeManager::getString(){return "Conservative Scheduler\n"; }
string StatefulScheduler::PrioritizeCompressionManager::getString(){return "PrioritizeCompression Scheduler\n"; }
string StatefulScheduler::EvenLessManager::getString(){return "Even Less Conservative Scheduler\n"; }

void StatefulScheduler::jobArrives(Job* j, unsigned long time, Machine* mach) {
  scheduleJob(j, time);
  heart->arrival(j, time);
}

unsigned long StatefulScheduler::scheduleJob(Job* j, unsigned long time){
  //inserts a job into the estimated schedule as early as possible without causing conflicts
  //updates jobToEvents accordingly
  unsigned long starttime;
  starttime = findTime(estSched, j, time);
  SchedChange* endChange = new SchedChange(starttime + j->getEstimatedRunningTime(), j, true, NULL);
  SchedChange* startChange = new SchedChange(starttime, j, false, endChange);
  estSched->insert(endChange);
  estSched->insert(startChange);
  jobToEvents->erase(j); //unnecessary for the most part, but if a job is reinserted can wind up with bad pointers
  jobToEvents->insert(pair<Job*,SchedChange*>(j, startChange));

  return starttime;
}

unsigned long StatefulScheduler::findTime(set<SchedChange*, SCComparator>* sched, Job *j, unsigned long time) {
  unsigned long intime = time;
  if (j->getEstimatedRunningTime() == 0)
    return zeroCase(sched, j, intime);

  if(intime < j->getArrivalTime())
    intime = j->getArrivalTime();

  set<SchedChange*, SCComparator>::iterator it = sched->begin();
  //to traverse schedule
  int currentFree = freeProcs;  //number of procs free at time being considered
  bool done = false;    //whether we've found working anchor point
  unsigned long anchorTime = intime;  //anchor point; possible starting time for job
  SchedChange *sc = NULL;   //first change we haven't looked at yet

  while (!done) {
    //will exit because anchor point at end of schedule must work

    if (it != sched->end()){
      sc = *(it);
      unsigned long scTime = sc->getTime();
      if (scTime <= anchorTime)
      {
        currentFree += sc->freeProcChange();
      }
      else {  //advanced to anchor point; now test it

        bool skip = false;    //to check if current procs falls below required amount
        while (!done && (currentFree >= j->getProcsNeeded())) {
          //process this change and any occuring at the same time
          currentFree += sc->freeProcChange();
          if (sc->j->getEstimatedRunningTime() == 0
              && currentFree < j->getProcsNeeded())
            skip = true;
          ++it; //in java we make sure that the NEXT element exists and satisfies the time constraints, so in C++ we increment now and dec at end of loop
          while (it != sched->end() &&  (*it)->getTime() == scTime) 
          {
            sc = *(it);
            currentFree += sc->freeProcChange();
            if (sc->j->getEstimatedRunningTime() == 0 && currentFree < j->getProcsNeeded())
              skip = true;
            ++it;
          }
          --it;

          if (skip) {
            ++it;
            if (it != sched->end() && anchorTime == scTime)
              sc = *(it);
            else
            {
              --it;
              currentFree -= sc->freeProcChange();
            }
            scTime = sc->getTime();
            break;
          }

          //check if we've gotten to where the job would end
          ++it;
          if ((scTime >= anchorTime + j->getEstimatedRunningTime()) || it == sched->end()) 
          {
            --it;
            done = true;    //yes; use the anchor point
          }
          else
          {
            sc = *(it); //no; advance the time we're looking at
          }
          scTime = sc->getTime();
        }
        if (!done) {    //not enough procs; advance anchorTime
          anchorTime = sc->getTime();
          currentFree += sc->freeProcChange();
        }
      }
      ++it;
    } else {  //ran out of changes before anchor point so can use it
      if (currentFree != numProcs)
        error("Conservative got to end of estimated schedule w/o all processors being free");
      done = true;
    }
  }
  return anchorTime;
}

unsigned long StatefulScheduler::zeroCase(set<SchedChange*, SCComparator> *sched, Job* filler, unsigned long time) {

  //iterate through event list to find first time where
  //there are enough avaiable procs
  int avaProcs = freeProcs;
  unsigned long lookAtTime = time;//to keep track of time being checked
  if ( avaProcs >= filler->getProcsNeeded()) 
    return time;

  //if can't run right away iterate
  set<SchedChange*, SCComparator>::reverse_iterator tour = sched->rend();
  //to traverse schedule
  SchedChange* sc;//event at current time
  int procsChangeDueToStart = 0;
  int procsChangeDueToZero = 0;

  while (tour != sched->rbegin())
  {
    sc = *(--tour);
    if (sc->getTime() > lookAtTime) {
      lookAtTime = sc->getTime();
      avaProcs += procsChangeDueToStart; //make sure to take change due to starting jobs in to consideration now
      procsChangeDueToStart = 0;
      procsChangeDueToZero = 0;
    }

    if (sc->getTime() == lookAtTime) {
      //if event occured at current time,
      //change number of free procs
      if (sc->j->getEstimatedRunningTime() != 0) {
        if (sc->isEnd)
          avaProcs += sc->freeProcChange();
        else
          procsChangeDueToStart += sc->freeProcChange();//ignore jobs that start at current time for now
      }else
        procsChangeDueToZero += sc->freeProcChange();
      bool keeplooping = true; //ugly hack because c++ iterators can't peek
      while (tour != sched->rbegin() && keeplooping)
      {
        tour--;
        if((*tour)->getTime() == lookAtTime) {
          sc = (*tour);
          if (sc->j->getEstimatedRunningTime() != 0) {
            if (sc->isEnd)
              avaProcs += sc->freeProcChange();
            else
              procsChangeDueToStart += sc->freeProcChange();//ignore jobs that start at current time for now
          } 
          else
            procsChangeDueToZero += sc->freeProcChange();
        }
        else
        {
          keeplooping = false;
          tour++;
        }
      }
    }
    if (sc->getTime() < lookAtTime) {//if occured before, count
      avaProcs += sc->freeProcChange();
    }

    //check and either end or increment
    if ( avaProcs >= filler->getProcsNeeded()) {
      //enough procs to run right away
      return lookAtTime;
    }
  }

  if (avaProcs >= filler->getProcsNeeded()) {//check if enough procs
    return lookAtTime;
  } else {
    //got to end of list but still not enough procs
    avaProcs += procsChangeDueToZero;
    if (avaProcs != numProcs){
      printPlan();
      error("Scheduler got to end of estimated schedule w/o all processors being free");
    }
  }
  return lookAtTime;
}

void StatefulScheduler::jobFinishes(Job* j, unsigned long time, Machine* mach){
  //remove j from the list of running jobs and update start

  set<SchedChange*, SCComparator>::reverse_iterator it = estSched->rend();
  bool success = false;
  SchedChange* sc ;
  while(it != estSched->rbegin()&& !success )
  {  
    sc = *(--it);
    unsigned long scTime = sc->getTime();
    if(scTime < time)
    {
      printPlan();
      error("expecting events in the past");
    }
    if(sc->j->getJobNum() == j->getJobNum())
    {
      if(!sc->isEnd)
        error("Job finished before scheduler started it");
      success = true;
      freeProcs += j->getProcsNeeded();
      estSched->erase(sc);
      jobToEvents->erase(j);
      if(time == scTime)
      {
        heart->onTimeFinish(j,time);
        delete sc;//job's done, don't need schedchange for it anymore
        return; //job ended exactly as scheduled so no compression
      }
    }
  }
  heart->earlyFinish(j,time);
  if(!success)
    error("Could not find finishing job in running list");
  delete sc;
}

void StatefulScheduler::printPlan() {
  //for debugging; prints planned schedule to stderr
  bool first = true;
  int procs = freeProcs;
  printf("Planned Schedule:\n");
  for(set<SchedChange*, SCComparator>::iterator it = estSched->begin(); it != estSched->end(); ++it)
  {
    SchedChange* sc = *it;
    printf("%d procs free so far|", procs);
    sc->print();
    first = false;
  }
  heart->printPlan();
}

void StatefulScheduler::removeJob(Job* j, unsigned long time)
{
  //removes job from schedule
  set<SchedChange*, SCComparator>::iterator it  = estSched->begin();
  while(it != estSched->end())
  {
    SchedChange* sc = *it;
    if(sc->j->getJobNum() == j->getJobNum())
    {
      estSched->erase(it);
      delete sc;
    }
    it++;
  }
}

AllocInfo* StatefulScheduler::tryToStart(Allocator* alloc, unsigned long time, Machine* mach, Statistics* stats) {
  //allows the scheduler to start a job if desired; time is current time
  //called after calls to jobArrives and jobFinishes
  //(either after each call or after each call occuring at same time)
  //returns first job to start, NULL if none
  //(if not NULL, should call tryToStart again)
  heart->tryToStart(time);
  set<SchedChange*, SCComparator>::iterator it = estSched->begin();
  while(it != estSched->end())
  {
    SchedChange* sc = *(it); 
    unsigned long scTime = sc->getTime();
    if(scTime < time)
    {
      printPlan();
      error("Expecting events in the past");
    }
    if(scTime > time)
    {
      return NULL;
    }

    if(!sc->isEnd)
    {
      AllocInfo* allocInfo = alloc->allocate(sc->j);
      if(allocInfo == NULL)
      {
        return NULL;
      }
      freeProcs -= sc->j->getProcsNeeded();
      estSched->erase(sc);
      jobToEvents->erase(sc->j);
      heart->start(sc->j, time);
      sc->j->start(time, mach, allocInfo, stats); //necessary for SST (and the allocator/machine) to actually get the job
      delete sc; //once a job is started we don't need its schedchange anymore
      return allocInfo;
    }
    it++;
  }
  return NULL;
}

void StatefulScheduler::reset() {
}
//comparator functions:
StatefulScheduler::JobComparator::JobComparator(ComparatorType type) {
  this -> type = type;
}


void StatefulScheduler::JobComparator::printComparatorList(ostream& out) {
  for(int i=0; i<numCompTableEntries; i++)
    out << "  " << compTable[i].name << endl;
}

StatefulScheduler::JobComparator* StatefulScheduler::JobComparator::Make(string typeName) {
  for(int i=0; i<numCompTableEntries; i++)
    if(typeName == compTable[i].name)
      return new StatefulScheduler::JobComparator(compTable[i].val);
  return NULL;
}

void internal_error(string mesg);

//manager functions:
void StatefulScheduler::Manager::compress(unsigned long time)
{
  //compresses the schedule so jobs start as quickly as possible
  //it is up to the manager to make sure this does not conflict with its guarantees
  set<SchedChange*, SCComparator> *oldEstSched = new set<SchedChange*, SCComparator>(*(scheduler->estSched));
  SCComparator* sccomp = new SCComparator();
  delete scheduler->estSched;
  scheduler->estSched = new set<SchedChange*, SCComparator>(*sccomp );

  //first pass; pick up unmatched ends
  //this must be done first so they appear when jobs are added
  for(set<SchedChange*, SCComparator>::iterator it = oldEstSched->begin(); it != oldEstSched->end(); it++)
  {
    SchedChange* sc = *it;
    if(!sc->isEnd)
      sc->getPartner()->j = NULL; //so we ignore its partner
    else
      if(sc->j != NULL) //if null we already saw its partner, otherwise it is the end of a running job so just copy it over
        scheduler->estSched->insert(sc);
  }

  //second pass; add the jobs whose starts appeared
  for(set<SchedChange*, SCComparator>::iterator it = oldEstSched->begin(); it != oldEstSched->end(); it++)
  {
    SchedChange* sc = *it;
    if(!sc->isEnd)
    {
      unsigned long scTime = sc->getTime();
      unsigned long newStartTime = scheduler->scheduleJob(sc->j, time);
      if(newStartTime > scTime)
        error("Attempt to delay estimated start of Job");
      delete sc; 
    }
    else if(sc->j == NULL) //these were duplicated; if not null it was copied directly and we don't want to delete it
      delete sc; //we may or may not have added the element to the array; either way we no longer need it.  oldEstSched is going to be filled with lots of bad pointers but the next thing we do is clear it anyway.
  }

  oldEstSched->clear();
  delete oldEstSched;
  delete sccomp;
}

StatefulScheduler::PrioritizeCompressionManager::PrioritizeCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp, int infillTimes){
  scheduler = inscheduler;
  backfill = new set<Job*, JobComparator>(*comp);
  fillTimes = infillTimes;
  numSBF = new int[fillTimes + 1];
  for(int x = 0; x < fillTimes+1; x++)
    numSBF[x] = 0;
}

void StatefulScheduler::PrioritizeCompressionManager::reset(){
  for(int x = 0; x < fillTimes + 1; x++)
    numSBF[x] = 0;
}

void StatefulScheduler::PrioritizeCompressionManager::printPlan(){}
void StatefulScheduler::PrioritizeCompressionManager::done(){
  for(int i = 0; i < fillTimes; i++)
    if(numSBF[i] != 0)
      printf("backfilled successfully %d times in a row %d times\n", i, numSBF[i]);
}

void StatefulScheduler::PrioritizeCompressionManager::earlyFinish(Job* j, unsigned long time){
  //backfills and compresses as necessary
  int times;
  bool exit = true;
  if (fillTimes == 0) {
    compress(time);
    return;
  }
  for (times = 0; times < fillTimes; times++) {
    //only backfill a certain number of times
    for (set<Job*, JobComparator>::iterator it = backfill->begin(); it != backfill->end(); it++){//iterate through backfill queue
      SchedChange* oldStartTime = (scheduler->jobToEvents->find(*it)->second);//store old start time

      //remove from current scheduler
      scheduler->estSched->erase(oldStartTime);
      scheduler->estSched->erase(oldStartTime->getPartner());

      unsigned long oldTime = oldStartTime->getTime();
      unsigned long newTime = oldTime;
      scheduler->jobToEvents->erase(*it); // otherwise there are multiple pais for the job and the wrong one may be used
      //try to backfill job
      newTime = scheduler->scheduleJob(*it, time);
      if (newTime > oldTime) { //not supposed to happen
        for( set<SchedChange*, SCComparator>::iterator sc = scheduler->estSched->begin(); sc != scheduler->estSched->end(); sc++)
          (*sc)->print();
        printf("old: %ld, new:%ld\nbackfilling: %s\n", oldTime, newTime, (*it)->toString().c_str());
        oldStartTime->print();
        error("PrioritizeCompression Backfilling gave a new reservation that was later than previous one");
      }
      delete oldStartTime;

      if (newTime < oldTime) {//backfill successful
        exit = false;
        break;
      } else {
        exit = true;
      }

    }
    if (exit)
      break;
  }

  //record results
  numSBF[times]++;

  if (!exit) //backfilling was cut off so we should compress
    compress(time);
}



void StatefulScheduler::PrioritizeCompressionManager::tryToStart(unsigned long time){
}

void StatefulScheduler::PrioritizeCompressionManager::removeJob(Job* j, unsigned long time){
}

void StatefulScheduler::PrioritizeCompressionManager::onTimeFinish(Job* j, unsigned long time){
}

StatefulScheduler::DelayedCompressionManager::DelayedCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp){
  scheduler = inscheduler;
  backfill = new set<Job*, JobComparator>(*comp);
  results = 0;
}

void StatefulScheduler::DelayedCompressionManager::reset(){
  results = 0;
  backfill->clear();
}

void StatefulScheduler::DelayedCompressionManager::arrival(Job* j, unsigned long time){
  SchedChange* newJobStart = scheduler->jobToEvents->find(j)->second;
  SchedChange* newJobEnd = newJobStart->getPartner();

  scheduler->estSched->erase(newJobStart);
  scheduler->estSched->erase(newJobEnd);

  bool moved = false;
  backfill->insert(j);


  //stop hole from being filled by newly arrived job

  //other more effecictive methods go here
  unsigned long start = newJobStart->getTime();

  //check if any existing job can fill that spot
  for (set<Job*, JobComparator>::iterator it = backfill->begin(); it != backfill->end(); it++){//iterate through backfill queue
    //remove from schedule

    SchedChange *oldStart = scheduler->jobToEvents->find(*it)->second;
    SchedChange *oldEnd = oldStart->getPartner();
    if((*it)->getJobNum() != j->getJobNum()){
      scheduler->estSched->erase(oldStart);
      scheduler->estSched->erase(oldEnd);
    }
    //check where would fit in schedule
    unsigned long newTime = scheduler->findTime(scheduler->estSched, *it, time);

    //if if can move it up
    if (newTime <= (start + j->getEstimatedRunningTime()) &&
        newTime != oldStart->getTime()) {

      scheduler->scheduleJob(*it, time);
      if(((*it)->getJobNum() == j->getJobNum()))
        moved = true;
    } else {
      //else put back in old place
      if(!((*it)->getJobNum() == j->getJobNum())){
        scheduler->estSched->insert(oldStart);
        scheduler->estSched->insert(oldEnd);
      }
    }
  }
  if(!moved)
    scheduler->scheduleJob(j, time); //reschedule new job
  delete newJobStart;
  delete newJobEnd;
}





void StatefulScheduler::DelayedCompressionManager::printPlan(){
  printf(" backfilling queue:\n");
  for(set<Job*, JobComparator>::iterator it = backfill->begin(); it != backfill->end(); it++)
    printf("%s\n", (*it)->toString().c_str());
}


void StatefulScheduler::DelayedCompressionManager::done(){
  printf("Backfilled %d times\n", results);
}

void StatefulScheduler::DelayedCompressionManager::earlyFinish(Job* j, unsigned long time){
  fill(time);
}

void StatefulScheduler::DelayedCompressionManager::tryToStart(unsigned long time){
  fill(time);
}

void StatefulScheduler::DelayedCompressionManager::fill(unsigned long time){
  //backfills
  for(set<Job*, JobComparator>::iterator it = backfill->begin(); it != backfill->end(); it++)
  {
    SchedChange* OldStart = scheduler->jobToEvents->find(*it)->second;
    SchedChange* OldEnd = OldStart->getPartner(); 

    scheduler->estSched->erase(OldStart);
    scheduler->estSched->erase(OldEnd);

    //check if readding to scheduler allows Job to start now
    if(time == scheduler->findTime(scheduler->estSched, *it, time)) {
      //if so reschedule
      scheduler->scheduleJob(*it, time);
      results++;
      delete OldStart;
      delete OldEnd;
    }
    else {
      scheduler->estSched->insert(OldStart);
      scheduler->estSched->insert(OldEnd);
    }
  }
}

void StatefulScheduler::DelayedCompressionManager::removeJob(Job* j, unsigned long time){ }

void StatefulScheduler::DelayedCompressionManager::onTimeFinish(Job* j, unsigned long time){ } 

StatefulScheduler::EvenLessManager::EvenLessManager(StatefulScheduler* inscheduler, JobComparator* comp, int fillTimes){
  SCComparator* sccomp = new SCComparator();
  scheduler = inscheduler;
  backfill = new set<Job*, JobComparator>(*comp);
  guarantee = new set<SchedChange*, SCComparator>(*sccomp);
  guarJobToEvents = new map<Job*, SchedChange*, JobComparator>(*comp);
  bftimes = fillTimes;
}

void StatefulScheduler::EvenLessManager::deepCopy(set<SchedChange*, SCComparator> *from, set<SchedChange*, SCComparator> *to, map<Job*, SchedChange*, JobComparator> *toJ) {
  // this does not actually delete elements: to->clear();
  set<SchedChange*, SCComparator>::iterator sc2 = to->begin();
  while(!to->empty())
  {
    sc2 = to->begin();
    SchedChange* sc = *sc2;
    to->erase(sc2);
    delete sc;
  }
  //the elements of toJ are of type pair<> (not a pointer) so can be cleared normally.
  toJ->clear();
  for(set<SchedChange*, SCComparator>::iterator sc = from->begin(); sc != from->end(); sc++){
    if (!((*sc)->isEnd)) {
      SchedChange* je = new SchedChange((*sc)->getTime()+(*sc)->j->getEstimatedRunningTime(), (*sc)->j, true, NULL);
      SchedChange* js = new SchedChange((*sc)->getTime(),(*sc)->j,false, je);
      to->insert(js);
      to->insert(je);
      toJ->insert(pair<Job*, SchedChange*>(js->j,js));
    } else if ((*sc)->isEnd) {
      if (toJ->find((*sc)->j) == toJ->end()) {
        SchedChange* je = new SchedChange((*sc)->getTime(), (*sc)->j, true, NULL);
        to->insert(je);
      }
    }
  }

}

void StatefulScheduler::EvenLessManager::backfillfunc(unsigned long time){
  for(int i = 0; i < bftimes; i++){
    for(set<Job*, JobComparator>::iterator it = backfill->begin(); it != backfill->end(); it++){
      SchedChange* js = (scheduler->jobToEvents->find(*it)->second);
      //DEBUG:
      if(!js)
        printf("null pointer %p is js\n", js);
      if(!js->getPartner())
        printf("null pointer  %p is jspartner\n", js->getPartner());
      set<SchedChange*,SCComparator>::iterator lower = scheduler->estSched->lower_bound(js->getPartner());
      set<SchedChange*,SCComparator>::iterator upper =  scheduler->estSched->upper_bound(js->getPartner());
      --upper;
      if(lower != upper)
      {
        printf("failed equality test:\n");  
        printf("%p |", *lower);
        (*lower)->print();
        printf("%p |", *upper);
        (*upper)->print();
      }
      //***
      scheduler->estSched->erase(js);
      scheduler->estSched->erase(js->getPartner());
      scheduler->jobToEvents->erase(*it);

      unsigned long old = js->getTime();
      unsigned long start = scheduler->findTime(scheduler->estSched,*it,time);
      SchedChange* je = new SchedChange(start+(*it)->getEstimatedRunningTime(),(*it), true, NULL);
      SchedChange* js2 = new SchedChange(start,(*it),false, je);
      scheduler->estSched->insert(js2);
      scheduler->estSched->insert(je);
      scheduler->jobToEvents->insert(pair<Job*,SchedChange*>((*it),js2));

      if (start==time && old != time) {
        //We are trying to run something now, check if it would kill the guaranteed
        //schedule

        SchedChange* gjs = guarJobToEvents->find((*it))->second;
        if(!gjs)
          printf("null pointer is gjs\n");
        SchedChange* gje = gjs->getPartner();

        guarantee->erase(gjs);
        guarantee->erase(gje);

        //Try adding the new start time to the guarantee
        //schedule and check if it is broken.
        guarantee->insert(je);
        guarantee->insert(js2);

        int freeprocs = scheduler->freeProcs;
        //We keep track of the last time seen so that the order of
        //jobs at the same time does not cause a negative value
        unsigned long last = (unsigned long)-1;
        bool destroyed = false;
        //String reason = "";
        for (set<SchedChange*,SCComparator>::iterator sc = guarantee->begin(); sc != guarantee->end(); sc++) {
          //If this is a new time from the last time
          if (last!=(*sc)->getTime() && freeprocs<0) {
            destroyed = true;
            printf("Bad at job |" );
            (*sc)->print();
            printf("\nwhen inserting ");
            je->print();
            int tempfreeprocs = scheduler->freeProcs;
            printf("\nguarantee:\n");
            for (set<SchedChange*,SCComparator>::iterator sc3 = guarantee->begin(); sc3 != guarantee->end(); sc3++) 
            {
              tempfreeprocs+= (*sc3)->freeProcChange();
              printf("%d |",tempfreeprocs);
              (*sc3)->print();
            }
            tempfreeprocs = scheduler->freeProcs;
            printf("\nestSched:\n");
            for (set<SchedChange*,SCComparator>::iterator sc3 = scheduler->estSched->begin(); sc3 != scheduler->estSched->end(); sc3++) 
            {
              tempfreeprocs+= (*sc3)->freeProcChange();
              printf("%d |",tempfreeprocs);
              (*sc3)->print();
            }
          }

          last = (*sc)->getTime();
          freeprocs += (*sc)->freeProcChange();
        }
        if (destroyed || freeprocs<0 || freeprocs != scheduler->numProcs) {
          //The schedule is impossible.

          printf(": backfilling of  destroys schedule ()\n");

          //Use the estimated schedule instead.

          //if we don't take these out, they are in both estSched and guar,
          //and deepcopy deletes and then copies them
          guarantee->erase(je);
          guarantee->erase(js2);
          deepCopy(scheduler->estSched,guarantee,guarJobToEvents);
        } else {
          //Guaranteed schedule looks OK.
          //Remove the temporary changes.
          guarantee->erase(je);
          guarantee->erase(js2);
          guarantee->insert(gje);
          guarantee->insert(gjs);
        }
      }
      delete js->getPartner();
      delete js;//new ones are made to replace them (je and js2)

      if (start < old) {
        //Backfilled to earlier time.
        break;
      } else if (start > old) {
        error(": Backfilling error, plan:");
        scheduler->printPlan();
        error("ELC gave a worse start time to . Old: , New: ");
      } else {
        //error(": Unable to backfill ");
      }
    }
  }
  compress(time);
}



void StatefulScheduler::EvenLessManager::reset(){
}

void StatefulScheduler::EvenLessManager::arrival(Job* j, unsigned long time){

  unsigned long gtime = scheduler->findTime(guarantee,j,time);
  SchedChange* je = new SchedChange(gtime+j->getEstimatedRunningTime(),j,true,NULL);
  SchedChange* js = new SchedChange(gtime,j,false,je);
  guarantee->insert(js);
  guarantee->insert(je);
  guarJobToEvents->insert(pair<Job*,SchedChange*>(j,js));
  backfill->insert(j);
  deepCopy(guarantee, scheduler->estSched, scheduler->jobToEvents);
  backfillfunc(time);
}

void StatefulScheduler::EvenLessManager::printPlan(){
  int free = scheduler->freeProcs;
  for( set<SchedChange*, SCComparator>::iterator sc = guarantee->begin(); sc != guarantee->end(); sc++)
  {
    free+=(*sc)->freeProcChange();
    printf("\t %ld, %d", (*sc)->j->getJobNum() , free);
  }
  printf("\n");
}


void StatefulScheduler::EvenLessManager::earlyFinish(Job* j, unsigned long time){
  for( set<SchedChange*, SCComparator>::iterator sc = guarantee->begin(); sc != guarantee->end(); sc++)
    if((*sc)->j->getJobNum() == j->getJobNum())
    {
      guarantee->erase(*sc);
      break;
    }

  deepCopy(guarantee, scheduler->estSched, scheduler->jobToEvents);
  backfillfunc(time);
}


void StatefulScheduler::EvenLessManager::fill(unsigned long time){
}

void StatefulScheduler::EvenLessManager::removeJob(Job* j, unsigned long time){
}

void StatefulScheduler::EvenLessManager::onTimeFinish(Job* j, unsigned long time){ 

  for( set<SchedChange*, SCComparator>::iterator sc = guarantee->begin(); sc != guarantee->end(); sc++)
    if((*sc)->j->getJobNum() == j->getJobNum())
    {
      guarantee->erase(sc);
      break;
    }
} 

void StatefulScheduler::EvenLessManager::start(Job* j, unsigned long time){
  //Remove the job's current guarantees and add a guaranteed end time
  SchedChange* js = guarJobToEvents->find(j)->second;
  guarJobToEvents->erase(j);
  guarantee->erase(js);
  set<SchedChange*, SCComparator>::iterator temp2 = guarantee->find(js->getPartner());
  guarantee->erase(js->getPartner());
  backfill->erase(j);
  SchedChange* temp = new SchedChange(time+j->getEstimatedRunningTime(),j, true);
  guarantee->insert(temp);
  int freeprocs = scheduler->freeProcs;

  //We keep track of the last time seen so that the order of
  //jobs at the same time does not cause a negative value
  unsigned long last = (unsigned long)-1;
  bool destroyed = false;
  for(set<SchedChange*, SCComparator>::iterator sc = guarantee->begin(); sc != guarantee->end(); )
  {
    //If this is a new time from the last time
    if (last!=(*sc)->getTime() && freeprocs<0) { //the last test is because a negative (temporarily) is OK if they're all at the same time; and any needed processors are immediately released.
      destroyed = true;
      printf("Bad at point %ld | %d \n", (*sc)->getTime(), freeprocs);

    }
    freeprocs += (*sc)->freeProcChange();
    last = (*sc)->getTime();
    sc++;
  }
  if (destroyed || freeprocs<0 || freeprocs != scheduler->numProcs) {
    //The schedule is impossible.
    if (freeprocs<0)
      printf( "Negative procs at end | \n");
    if (freeprocs!=scheduler->numProcs)
      printf(  "All procs not freed\n");

    printf("the schedule is impossible, using estimated schedule instead\n");

    //Use the estimated schedule instead.
    deepCopy(scheduler->estSched,guarantee,guarJobToEvents);
  } else {
    //deepCopy(guarantee,scheduler->estSched,scheduler->jobToEvents);
    //(this was commented out in the Java as well)
  }
}

//JobComparator operator
bool StatefulScheduler::JobComparator::operator()(Job* const& j1,Job* const& j2){ 
  //true means j1 comes before j2
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

    case BETTERFIT:
      //Primary criteria: Most Processors Required
      if(j1->getProcsNeeded() != j2->getProcsNeeded())
        return (j2->getProcsNeeded() < j1->getProcsNeeded());

      //Secondary criteria: Longest Run Time
      if(j1->getEstimatedRunningTime() != j2->getEstimatedRunningTime()) {
        return (j2->getEstimatedRunningTime() < j1->getEstimatedRunningTime());
      }

      //Tertiary criteria: Arrival Time
      if(j1->getArrivalTime() != j2->getArrivalTime()) {
        return  (j2->getArrivalTime() > j1->getArrivalTime());
      }

      //break ties so different jobs are never equal:
      return j2 -> getJobNum() > j1 -> getJobNum();

    default:
      internal_error("operator() called on JobComparator w/ invalid type");
      return true;  //never reach here
  }
}


string StatefulScheduler::JobComparator::toString() {
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
      return "BestFitComparator";
    default:
      internal_error("toString() called on JobComparator w/ invalid type");
  }
  return "";  //never reach here...
}


