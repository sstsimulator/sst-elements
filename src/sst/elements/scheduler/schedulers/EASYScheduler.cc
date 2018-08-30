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
 * Schedulers based around priority queues; jobs run in order given by
 * some comparator without any backfilling.
 */

#include "sst_config.h"
#include "EASYScheduler.h"

#include <functional>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>

#include <iostream> //debug

#include "Job.h"
#include "Machine.h"
#include "output.h"

using namespace std;
using namespace SST::Scheduler;

const EASYScheduler::compTableEntry EASYScheduler::compTable[6] = {
    {FIFO, "fifo"},
    {LARGEFIRST, "largefirst"},
    {SMALLFIRST, "smallfirst"},
    {LONGFIRST, "longfirst"},
    {SHORTFIRST, "shortfirst"},
    {BETTERFIT, "betterfit"}
};

const int EASYScheduler::numCompTableEntries = 6;

EASYScheduler::EASYScheduler(JobComparator* comp) 
{ 
    schedout.init("", 10, 0, Output::STDOUT);
    this -> comp = comp;
    toRun = new set<Job*, JobComparator, std::allocator<Job*> >(*comp);
    RunningInfo* RIComp = new RunningInfo();
    running = new multiset<RunningInfo*, RunningInfo>(*RIComp); //don't need to pass comp because compare longs
    delete RIComp;
    compSetupInfo = comp -> toString();
    prevFirstJobNum = -1;
    guaranteedStart = 0;
    lastGuarantee = 0;
}

EASYScheduler::EASYScheduler(EASYScheduler* insched, std::set<Job*, JobComparator>* newtoRun, std::multiset<RunningInfo*, RunningInfo>* newrunning)
{ 
    schedout.init("", 8, 0, Output::STDOUT);
    comp = new JobComparator(insched -> comp);
    toRun = newtoRun;
    running = newrunning;
    compSetupInfo = comp -> toString();
    prevFirstJobNum = insched -> prevFirstJobNum;
    guaranteedStart = insched -> guaranteedStart;
    lastGuarantee = insched -> lastGuarantee;
}

void usage();

string EASYScheduler::getSetupInfo(bool comment) 
{
    string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "EASY Scheduler (" + compSetupInfo + ")"; 
}

//This is called when j arrives; time is current time.
//tryToStart should be called after each job arrives.
void EASYScheduler::jobArrives(Job* j, unsigned long time, const Machine & mach) 
{
    schedout.debug(CALL_INFO, 7, 0, "%ld: Job #%ld arrives\n", time, j -> getJobNum());
    toRun -> insert(j);


    set<Job*, JobComparator>::iterator firstJob = toRun -> begin();
    //if it's the first job give a time guarantee
    if (j -> getJobNum() == (*(firstJob)) -> getJobNum()) {
        giveGuarantee(time, mach);
    }
}

//Remove j from the list of running jobs and update start.
void EASYScheduler::jobFinishes(Job* j, unsigned long time, const Machine & mach)
{
    schedout.debug(CALL_INFO, 7, 0, "%ld: Job #%ld completes\n", time, j -> getJobNum());
    multiset<RunningInfo*, RunningInfo>::iterator it = running -> begin();
    bool success = false;
    while (!success && it != running -> end()) {  
        if ((*it) -> jobNum == j -> getJobNum()) {
            delete *it;
            running -> erase(it);
            success = true;
            break;    
        }
        it++;
    }
    if (!success) schedout.fatal(CALL_INFO, 1, "Could not find finishing job in running list\n%s\n", j -> toString().c_str());
    giveGuarantee(time, mach);
}

Job* EASYScheduler::tryToStart(unsigned long time, const Machine & mach)
{
    schedout.debug(CALL_INFO, 10, 0, "trying to start at %lu\n", time);
    if (!running->empty() && (*(running->begin()))->estComp == time) {
        return NULL;  //don't backfill if another job is about to finish
    }
    if (toRun -> empty()) return NULL;

    set<Job*, JobComparator>::iterator job = toRun -> begin();
    if (time > guaranteedStart) {
        schedout.fatal(CALL_INFO, 1, "Failed to start job #%ld at guaranteed time \nTime: %lu Guarantee: %lu\n", (*job)->getJobNum(), time, guaranteedStart);
    }
    
    bool succeeded = false;  //whether we found a job to allocate
    int availNodes = mach.getNumFreeNodes();
    
    if (availNodes >= ceil(((float)(*job)->getProcsNeeded()) / mach.coresPerNode)) {
        succeeded = true;
    } else {
        job++; 
    }
    
    while (!succeeded && job != toRun->end()) {
        int nodesNeeded = ceil(((float)(*job)->getProcsNeeded()) / mach.coresPerNode);
        if (availNodes >= nodesNeeded) {
            // check if j would delay the first job if it started now
            if (time + (*job)->getEstimatedRunningTime() <= guaranteedStart){
                succeeded = true;
            } else {
                int avail = availNodes;
                multiset<RunningInfo*, RunningInfo>::iterator it  = running->begin();
                while (it != running->end() && (*it)->estComp <= guaranteedStart) {
                    avail += (*it)->numNodes;
                    it++;
                }
                set<Job*, JobComparator>::iterator tempit = toRun->begin();
                if (avail  - nodesNeeded >= ceil(((float)(*tempit)->getProcsNeeded()) / mach.coresPerNode) ){
                    succeeded = true;
                }
            }
        }
        if (!succeeded) job++;
    }
    
    if(succeeded) {
        nextToStart = *job;
        nextToStartTime = time;
    } else {
        nextToStart = NULL;
    }
    
    return nextToStart;
}
    
void EASYScheduler::startNext(unsigned long time, const Machine & mach)
{
    if(nextToStart == NULL){
        schedout.fatal(CALL_INFO, 1, "Called startNext() job from scheduler when there is no available Job at time %lu",
                                      time);
    } else if(nextToStartTime != time){
        schedout.fatal(CALL_INFO, 1, "startNext() and tryToStart() are called at different times for Job #%ld",
                                      nextToStart->getJobNum());
    }
    set<Job*, JobComparator>::iterator jobIt = toRun->begin();
    //whether it is the first job
    bool first = nextToStart->getJobNum() == (*jobIt)->getJobNum();
    
    //find job iterator
    while (jobIt != toRun->end() && (*jobIt)->getJobNum() != nextToStart->getJobNum()) {
        jobIt++;
    }
    if(jobIt == toRun->end()){
        schedout.fatal(CALL_INFO, 1, "Job #%ld is not on toRun list.", nextToStart->getJobNum());
    }
    
    //allocate job
    schedout.debug(CALL_INFO, 7, 0, "%ld: %s starts\n", time, nextToStart->toString().c_str()); 
    RunningInfo* started = new RunningInfo();
    started -> jobNum = nextToStart->getJobNum();
    started -> numNodes = ceil((float)nextToStart->getProcsNeeded() / mach.coresPerNode);
    started -> estComp = time + nextToStart->getEstimatedRunningTime();
    toRun -> erase(jobIt); //remove the job from toRun list
    running -> insert(started); //add to running list       

    if (first) { //update the guarantee if starting the first job
        giveGuarantee(time, mach);      
    }

    multiset<RunningInfo*, RunningInfo>::iterator dit = running -> begin();
    schedout.debug(CALL_INFO, 10, 0, "Currently running jobs: ");
    while(dit != running -> end()) {
        schedout.debug(CALL_INFO, 10, 0, "%ld (%ld)", (*dit) -> jobNum, (*dit) -> estComp);
        ++dit;
    }
    schedout.debug(CALL_INFO, 10, 0, "\n");
    
    nextToStart = NULL;
}

void EASYScheduler::reset() 
{
    toRun -> clear();
    running -> clear();
}

//gives a guaranteed start time for a job.  The first job in the queue cannot
//be delayed past this guarantee
void EASYScheduler::giveGuarantee(unsigned long time, const Machine & mach)
{
    schedout.debug(CALL_INFO, 10, 0, "%ld: giveGuarantee ( ", time);
    set<Job*, JobComparator>::iterator it2 = toRun -> begin();
    while (it2 != toRun -> end()) {
        schedout.debug(CALL_INFO, 10, 0, "%ld ", (*it2) -> getJobNum());
        ++it2;
    }
    schedout.debug(CALL_INFO, 10, 0, ")\n");

    if(toRun -> empty())
        return;

    set<Job*, JobComparator>::iterator firstJob = toRun->begin();
    unsigned long lastGuarantee = guaranteedStart;
    bool succeeded = false;

    int size = ceil(((float)(*firstJob)->getProcsNeeded()) / mach.coresPerNode);
    int free = mach.getNumFreeNodes();

    if (free >= size) {
        guaranteedStart = time;
        succeeded = true;
    }

    int futureFree = free;
    multiset<RunningInfo*, RunningInfo>::iterator it = running -> begin();

    while (!succeeded && it != running->end()) {
        futureFree += (*it)->numNodes;
        if(futureFree >= size) {
            guaranteedStart = (*it) -> estComp;
            succeeded = true;
        }
        it++;
    }
    if (succeeded)
    {
        schedout.debug(CALL_INFO, 7, 0, "%ld: Giving %s guarantee of time %ld\n", time, (*firstJob) -> toString().c_str(), guaranteedStart);
        if ((*firstJob) -> getJobNum() == prevFirstJobNum && lastGuarantee + 1 < guaranteedStart && lastGuarantee > 0) {
            schedout.output("EASY scheduler gave new guarantee worse than previous\n");
            schedout.output("Time: %lu\n", time);
            schedout.output("Running: ");
            for (multiset<RunningInfo*, RunningInfo>::iterator it3 = running -> begin(); it3 != running -> end(); it3++) {
                schedout.output("%ld %d %ld \n", (*it3) -> jobNum, (*it3) -> numNodes, (*it3) -> estComp);
            }
            schedout.output("toRun: ");
            for (set<Job*, JobComparator>::iterator it3 = toRun->begin(); it3 != toRun -> end(); it3++) {
                schedout.output("%ld ", (*it3) -> getJobNum());
            }
            schedout.output("\n");
            schedout.fatal(CALL_INFO, 1, "last guarantee: %ld new guarantee %ld\n EASY scheduler gave new guarantee worse than previous\nFor %s", lastGuarantee, guaranteedStart, (*firstJob)->toString().c_str()); 

        }
        prevFirstJobNum = (*firstJob) -> getJobNum();
    } else {
        schedout.fatal(CALL_INFO, 1, "EASY unable to make reservation for first job (%ld)\n", (*firstJob)->getJobNum());
    }
}

EASYScheduler::JobComparator::JobComparator(ComparatorType type) 
{
    this -> type = type;
}


void EASYScheduler::JobComparator::printComparatorList(ostream& out) 
{
    for (int i=0; i < numCompTableEntries; i++)
        out << "  " << compTable[i].name << endl;
}

EASYScheduler::JobComparator* EASYScheduler::JobComparator::Make(string typeName) 
{
    for (int i=0; i < numCompTableEntries; i++) {
        if (typeName == compTable[i].name) {
            return new EASYScheduler::JobComparator(compTable[i].val);
        }
    }
    return NULL;
}

//for FST, creates an exact copy of the scheduler if running and/or toRun are
//given. inrunning and intoRun contain (deep) copies of the jobs in EASYScheduler's running and
//toRun.  We don't want to mess these jobs up for our simulation, so the copy
//we return will replace all our pointers with pointers to the given deep copies.
//For the case of the EASY scheduler, running does not store a pointer to the
//job, so we ignore the running jobs and just deep copy it manually

EASYScheduler* EASYScheduler::copy(std::vector<Job*>* inrunning, std::vector<Job*>* intoRun)
{
    //we'll pass these versions to the next scheduler
    set<Job*, JobComparator, std::allocator<Job*> >* newtoRun = new set<Job*, JobComparator, std::allocator<Job*> >(*comp);

    RunningInfo* RIComp = new RunningInfo();
    multiset<RunningInfo*, RunningInfo>* newrunning = new multiset<RunningInfo*, RunningInfo>(*RIComp); 
    delete RIComp;

    //copy running
    for (multiset<RunningInfo*, RunningInfo>::iterator it = running -> begin(); it != running -> end(); it++) {
        newrunning -> insert(new RunningInfo(*it));
    }

    //replace pointers in toRun
    for (set<Job*, JobComparator, std::allocator<Job*> >::iterator it = toRun -> begin(); it != toRun -> end(); it++) {
        bool found = false;
        for (vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it2) -> getJobNum() == (*it) -> getJobNum()) {
                newtoRun -> insert(*it2);
                found = true;
            }
        }
        if (!found) schedout.fatal(CALL_INFO, 1, "Could not find deep copy for %s\nwhen copying EASYScheduler for FST\n", (*it) -> toString().c_str());
    } 

    //call the constructor and return
    return new EASYScheduler(this, newtoRun, newrunning); 
}

//for this to work correctly, it returns the reverse of what it would
//in the comparators for the PQscheduler (because this uses sets and maps
//instead of priority queues)
bool EASYScheduler::JobComparator::operator()(Job* const& j1,Job* const& j2) 
{ 
    switch(type) {
    case FIFO:
        if (j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return j2 -> getArrivalTime() > j1 -> getArrivalTime();
        }
        return j2 -> getJobNum() > j1 -> getJobNum();
    case LARGEFIRST:
        //largest job goes first if they are different size
        if (j1 -> getProcsNeeded() != j2 -> getProcsNeeded()) {
            return (j2 -> getProcsNeeded() < j1 -> getProcsNeeded());
        }

        //secondary criteria: earlier arriving job first
        if (j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return j1 -> getArrivalTime() < j2 -> getArrivalTime();
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    case SMALLFIRST:
        //smaller job goes first if they are different size
        if (j1 -> getProcsNeeded() != j2 -> getProcsNeeded()) {
            return (j2 -> getProcsNeeded() > j1 -> getProcsNeeded());
        }

        //secondary criteria: earlier arriving job first
        if (j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return j1 -> getArrivalTime() < j2 -> getArrivalTime();
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    case LONGFIRST:
        //longest job goes first if different lengths
        if (j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime()) {
            return j2 -> getEstimatedRunningTime() < j1 -> getEstimatedRunningTime();
        }

        //secondary criteria: earliest arriving job first
        if (j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return j1 -> getArrivalTime() < j2 -> getArrivalTime();
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    case SHORTFIRST:
        //longest job goes first if different lengths
        if (j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime()) {
            return j2 -> getEstimatedRunningTime() > j1 -> getEstimatedRunningTime();
        }

        //secondary criteria: earliest arriving job first
        if (j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return j1 -> getArrivalTime() < j2 -> getArrivalTime();
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    case BETTERFIT:
        //Primary criteria: Most Processors Required
        if (j1->getProcsNeeded() != j2->getProcsNeeded()) {
            return (j2->getProcsNeeded() < j1->getProcsNeeded());
        }

        //Secondary criteria: Longest Run Time
        if (j1->getEstimatedRunningTime() != j2->getEstimatedRunningTime()) {
            return (j2->getEstimatedRunningTime() < j1->getEstimatedRunningTime());
        }

        //Tertiary criteria: Arrival Time
        if (j1->getArrivalTime() != j2->getArrivalTime()) {
            return  (j2->getArrivalTime() > j1->getArrivalTime());
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    default:
        //internal_error("operator() called on JobComparator w/ invalid type");
        schedout.fatal(CALL_INFO, 1, "operator() called on JobComparator w/ invalid type");
        return true;  //never reach here
    }
}

string EASYScheduler::JobComparator::toString() 
{
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
        schedout.fatal(CALL_INFO, 1, "toString() called on JobComparator w/ invalid type %d", (int)type);
    }
    return "";  //never reach here...
}


