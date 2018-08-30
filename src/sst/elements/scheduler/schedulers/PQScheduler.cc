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
#include "PQScheduler.h"

#include <functional>
#include <string>
#include <vector>
#include <cmath>

#include "Job.h"
#include "Machine.h"
#include "misc.h"
#include "output.h"

using namespace std;

using namespace SST::Scheduler;


const PQScheduler::compTableEntry PQScheduler::compTable[] = {
    { FIFO, "fifo"},
    { LARGEFIRST, "largefirst"},
    { SMALLFIRST, "smallfirst"},
    { LONGFIRST, "longfirst"},
    { SHORTFIRST, "shortfirst"},
    { BETTERFIT, "betterfit"}
};

const int PQScheduler::numCompTableEntries = 6;

PQScheduler::PQScheduler(JobComparator* comp) 
{
    schedout.init("", 8, ~0, Output::STDOUT);
    toRun = new priority_queue<Job*, vector<Job*>, JobComparator>(*comp);
    compSetupInfo = comp -> toString();
    origcomp = comp;
}

//constructor only used in copy()
PQScheduler::PQScheduler(PQScheduler* insched, priority_queue<Job*,std::vector<Job*>,JobComparator>* intoRun) 
{
    schedout.init("", 8, ~0, Output::STDOUT);
    toRun = intoRun;
    compSetupInfo = insched -> compSetupInfo;
    origcomp = insched -> origcomp;
}

//copy for FST
//there is no running so we ignore it.
//Have to deep copy toRun to match FST's pointers given in intoRun
PQScheduler* PQScheduler::copy(std::vector<Job*>* inrunning, std::vector<Job*>* intoRun)
{
    //the toRun with correct pointers for our new scheduler
    priority_queue<Job*,std::vector<Job*>,JobComparator>* newtoRun = new priority_queue<Job*, vector<Job*>, JobComparator>(*origcomp);

    //to copy toRun, we have to pop each element off individually, then add them to copyToRun. We must add them
    //back to toRun as we pop them off copyToRun
    priority_queue<Job*,std::vector<Job*>,JobComparator>* copyToRun = new priority_queue<Job*, vector<Job*>, JobComparator>(*origcomp); 
    while(!toRun -> empty()) {
        copyToRun -> push(toRun -> top());
        toRun -> pop();
    }

    int notfound = 0;
    while (!copyToRun -> empty()) {
        bool found = false;
        Job* it = copyToRun -> top();
        toRun -> push(it); //add the element back to toRun
        copyToRun -> pop();
        for (vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it2) -> getJobNum() == it -> getJobNum()) {
                newtoRun -> push(*it2);
                found = true;
            }
        }
        if (!found) {
            schedout.debug(CALL_INFO, 7, 0, "Cannot find %s in toRun\n", it -> toString().c_str());
            notfound++;
        }
    }
    //we can not find one element, as if the schedule is relaxed the
    //scheduler will have the job we're calculating the FST for already in
    //toRun.  However, FST won't tell the scheduler about the job until all
    //other jobs are started.
    if (notfound > 1) {
        schedout.output("toRun:\n");
        for (vector<Job*>::iterator it2 = intoRun -> begin(); it2 != intoRun -> end(); it2++) {
            schedout.output("%s\n", (*it2) -> toString().c_str());
        }
        schedout.fatal(CALL_INFO, 1, "Could not find %d jobs when copying toRun for FST\n", notfound);
    }
    delete copyToRun;

    //make the new scheduler
    return new PQScheduler(this, newtoRun); 
}


void usage();

string PQScheduler::getSetupInfo(bool comment) 
{
    string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Priority Queue Scheduler\n" + com + 
        "Comparator: " + compSetupInfo; 
}

//called when j arrives; time is current time
//tryToStart should be called after each job arrives
void PQScheduler::jobArrives(Job* j, unsigned long time, const Machine & mach) 
{
    toRun -> push(j);
    schedout.debug(CALL_INFO, 7, 0, "%s arrives\n", j -> toString().c_str());
}

//allows the scheduler to start a job if desired; time is current time
//called after calls to jobArrives and jobFinishes
//(either after each call or after each call occuring at same time)
//returns first job to start, NULL if none
//(if not NULL, should call tryToStart again)
Job* PQScheduler::tryToStart(unsigned long time, const Machine & mach) 
{
    if (0 == toRun->size()) return NULL;
    Job* job = toRun->top();
    int nodesNeeded = ceil(((float) job->getProcsNeeded()) / mach.coresPerNode);
    if (mach.getNumFreeNodes() >= nodesNeeded) {
        nextToStart = job;
        nextToStartTime = time;
    } else {
        nextToStart = NULL;
    }
    return nextToStart;
}

void PQScheduler::startNext(unsigned long time, const Machine & mach)
{
    if(nextToStart == NULL){
        schedout.fatal(CALL_INFO, 1, "Called startNext() job from scheduler when there is no available Job at time %lu",
                                      time);
    } else if(nextToStartTime != time){
        schedout.fatal(CALL_INFO, 1, "startNext() and tryToStart() are called at different times for Job #%ld",
                                      nextToStart->getJobNum());
    }
    toRun->pop(); //remove the job
    return;
}

void PQScheduler::reset() 
{
    while(!toRun -> empty()) {
        toRun -> pop();
    }
}


PQScheduler::JobComparator::JobComparator(ComparatorType type) 
{
    this -> type = type;
}

void PQScheduler::JobComparator::printComparatorList(ostream& out) 
{
    for (int i = 0; i < numCompTableEntries; i++) {
        out << "  " << compTable[i].name << endl;
    }
}

PQScheduler::JobComparator* PQScheduler::JobComparator::Make(string typeName) 
{
    for (int i = 0; i < numCompTableEntries; i++) {
        if (typeName == compTable[i].name) {
            return new JobComparator(compTable[i].val);
        }
    }
    return NULL;
}

//void internal_error(string mesg);

bool PQScheduler::JobComparator::operator()(Job*& j1, Job*& j2) 
{
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
        if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
            return (j2 -> getProcsNeeded() > j1 -> getProcsNeeded());

        //Secondary criteria: Longest Run Time
        if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime()) {
            return (j2 -> getEstimatedRunningTime() > j1->getEstimatedRunningTime());
        }

        //Tertiary criteria: Arrival Time
        if(j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return  (j2 -> getArrivalTime() < j1 -> getArrivalTime());
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() < j1 -> getJobNum();

    default:
        //internal_error("operator() called on JobComparator w/ invalid type");
        schedout.fatal(CALL_INFO, 1, "operator() called on JobComparator w/ invalid type");
        return true;  //never reach here
    }
}

string PQScheduler::JobComparator::toString() 
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
        schedout.fatal(CALL_INFO, 1, "toString() called on JobComparator w/ invalid type");
    }
    return "";  //never reach here...
}


