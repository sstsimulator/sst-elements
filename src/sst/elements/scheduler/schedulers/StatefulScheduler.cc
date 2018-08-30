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
#include "StatefulScheduler.h"

#include <functional>
#include <set>
#include <cstdio>
#include <string>
#include <cmath>

#include "Job.h"
#include "Machine.h"
#include "misc.h"
#include "output.h"

using namespace std;
using namespace SST::Scheduler;

const StatefulScheduler::compTableEntry StatefulScheduler::compTable[6] = {
    { FIFO, "fifo"},
    { LARGEFIRST, "largefirst"},
    { SMALLFIRST, "smallfirst"},
    { LONGFIRST, "longfirst"},
    { SHORTFIRST, "shortfirst"},
    {BETTERFIT, "betterfit"}};
const int StatefulScheduler::numCompTableEntries = 6;

//each manager has a different constructor for stateful scheduler
StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, bool dummy, const Machine & inmach) : mach(inmach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    //"dummy" variable distinguishes between constructors; does not do anything and its value does not matter
    SCComparator *sccomp = new SCComparator();
    numProcs = numprocs;
    freeProcs = numprocs;
    estSched = new set<SchedChange*, SCComparator>(*sccomp);
    jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
    origcomp = comp;
    heart = new ConservativeManager(this, mach);
    sc = NULL;
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, int filltimes, const Machine & inmach) : mach(inmach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    SCComparator *sccomp = new SCComparator();
    numProcs = numprocs;
    freeProcs = numprocs;
    estSched = new set<SchedChange*, SCComparator>(*sccomp);
    jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
    origcomp = comp;
    heart = new PrioritizeCompressionManager(this, comp, filltimes, mach);
    sc = NULL;
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, const Machine & inmach) : mach(inmach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    SCComparator *sccomp = new SCComparator();
    numProcs = numprocs;
    freeProcs = numprocs;
    estSched = new set<SchedChange*, SCComparator>(*sccomp);
    jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
    origcomp = comp;
    heart = new DelayedCompressionManager(this, comp, mach); 
    sc = NULL;
}

StatefulScheduler::StatefulScheduler(int numprocs, StatefulScheduler::JobComparator* comp, int filltimes, bool dummy, const Machine & inmach) : mach(inmach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    SCComparator *sccomp = new SCComparator();
    numProcs = numprocs;
    freeProcs = numprocs;
    estSched = new set<SchedChange*, SCComparator>(*sccomp);
    jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*comp);
    origcomp = comp;
    heart = new EvenLessManager(this, comp, filltimes, mach);
    sc = NULL;
}

//copy constructor for copy() (which is, in turn, for FST)
StatefulScheduler::StatefulScheduler(StatefulScheduler* insched, set<SchedChange*, SCComparator>* inestSched, Manager* inheart, map<Job*, SchedChange*, StatefulScheduler::JobComparator>* inJobToEvents, const Machine & inmach) : mach(inmach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    numProcs = insched -> numProcs;
    freeProcs = insched -> freeProcs;
    //if (NULL == inestSched) {
    //SCComparator *sccomp = new SCComparator();
    //estSched = new set<SchedChange*, SCComparator>(*sccomp);
    //} else {
    estSched = inestSched;
    //}
    //if (NULL == inJobToEvents) {
    //jobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*(inheart -> origcomp));
    //} else {
    jobToEvents = inJobToEvents;
    //}
    //if (NULL == inheart) {
    //heart = insched -> heart -> copy(NULL);
    //} else {
    heart = inheart; 
    heart -> scheduler = this;
    //}
    sc = NULL;
}

//for FST, creates an exact copy of the scheduler if running and/or toRun are
//given. inrunning and intoRun contain (deep) copies of the jobs in
//StatefulScheduler's estSched and toRun (i.e. heart -> backfill if it exists),
//respectively.  We don't want to mess these jobs up for our simulation, so the
//copy we return will replace all our pointers with pointers to the given deep
//copies. We also must change the pointers in jobToEvents, for both jobs and
//schedchanges (as both are deleted once the job ends) 
StatefulScheduler* StatefulScheduler::copy(std::vector<Job*>* inrunning, std::vector<Job*>* intoRun)
{
    //copy heart -> backfill if it exists (as well as anything else with job
    //pointers)

    SCComparator sccomp; 
    std::set<SchedChange*, SCComparator>* newestSched = new std::set<SchedChange*, SCComparator>(sccomp);
    map<Job*, SchedChange*, StatefulScheduler::JobComparator>* newJobToEvents = new map<Job*, SchedChange*, StatefulScheduler::JobComparator>(*(origcomp));

    for (set<SchedChange*, SCComparator>::iterator it = estSched -> begin(); it != estSched -> end(); it++) {
        bool found = false;
        //jobs in estSched can be either running or can be waiting to be
        //scheduled, so we must check in both inrunning and intoRun
        for (vector<Job*>::iterator it2 = inrunning -> begin(); !found && it2 != inrunning -> end(); it2++) {
            if ((*it2) -> getJobNum() == (*it) -> j -> getJobNum()) {
                SchedChange* tempsc = new SchedChange(*it, mach);
                tempsc -> j = *it2;
                newestSched -> insert(tempsc);
                found = true;
            }
        }
        for (vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it2) -> getJobNum() == (*it) -> j -> getJobNum()) {
                SchedChange* tempsc = new SchedChange(*it, mach);
                tempsc -> j = *it2;
                newestSched -> insert(tempsc);
                found = true;
            }
        }
        if (!found) {
            printPlan();    
            schedout.output("toRun:\n");
            for (vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
                schedout.output("%s\n", (*it2) -> toString().c_str());
            }
            schedout.output("running:\n");
            for (vector<Job*>::iterator it2 = inrunning -> begin(); !found && it2 != inrunning -> end(); it2++) {
                schedout.output("%s\n", (*it2) -> toString().c_str());
            } 
            schedout.fatal(CALL_INFO, 1, "Could not find deep copy for %s\nwhen copying StatefulScheduler estSched for FST\n", (*it) -> j -> toString().c_str());
        }
    } 
    //now the SchedChanges in newEstSched are not pointing to their correct partner (they're pointing to the old version)
    //we must iterate through newEstSched and fix that
    for (set<SchedChange*, SCComparator>::iterator it = newestSched -> begin(); it != newestSched -> end(); it++) {
        if (!(*it) -> isEnd) {
            //find the corresponding SchedChange
            bool found = false;
            for (set<SchedChange*, SCComparator>::iterator it2 = it; !found && it2 != newestSched -> end(); it2++) {
                if ((*it2) -> isEnd && (*it) -> j -> getJobNum() == (*it2) -> j -> getJobNum()) {
                    found = true;
                    (*it) -> partner = *it2;
                }
            }
            if (!found) {
                schedout.output("plan: \n");
                for (set<SchedChange*, SCComparator>::iterator it3 = newestSched -> begin(); it3 != newestSched -> end(); it3++) {
                    (*it3) -> print();
                } 
                schedout.output("===========================================\n");
                (*it)->print();
                schedout.fatal(CALL_INFO, 1, "Was not able to find partner\n"); 
            }
        }
    }
    

    for (map<Job*, SchedChange*, StatefulScheduler::JobComparator>::iterator it = jobToEvents -> begin(); it != jobToEvents -> end(); it++) {
        bool found = false;
        for (vector<Job*>::iterator it2 = inrunning -> begin(); !found && it2 != inrunning -> end(); it2++) {
            //schedout.output("it2jte %p\n", *it2);
            if ((*it2) -> getJobNum() == it -> first -> getJobNum()) {
                //make a new pair; same schedchange but different job number
                for (set<SchedChange*, SCComparator>::iterator it3 = newestSched -> begin(); it3 != newestSched -> end(); it3++) {
                    if ((*it3) -> j -> getJobNum() == (*it2) -> getJobNum()) {
                        newJobToEvents -> insert(pair<Job*,SchedChange*>(*it2, *it3));
                        found = true;
                    }
                }
            }
        }
        for (vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            //schedout.output("it2jte2 %p\n", *it2);
            //fflush(stdout);
            if ((*it2) -> getJobNum() == it -> first -> getJobNum()) {
                //newJobToEvents -> insert(pair<Job*,SchedChange*>(*it2, new SchedChange(it -> second)));
                for (set<SchedChange*, SCComparator>::iterator it3 = newestSched -> begin(); it3 != newestSched -> end(); it3++) {
                    if ((*it3) -> j -> getJobNum() == (*it2) -> getJobNum()) {
                        newJobToEvents -> insert(pair<Job*,SchedChange*>(*it2, *it3));
                        found = true;
                    }
                }
                found = true;
            }
        }
        if (!found) {
            printPlan();
            schedout.fatal(CALL_INFO, 1, "Could not find deep copy for %s\nwhen copying StatefulScheduler jobToEvents for FST\n", it -> first -> toString().c_str());
        }
    }

    Manager* newheart = heart -> copy(inrunning, intoRun);
    StatefulScheduler* ret = new StatefulScheduler(this, newestSched, newheart, newJobToEvents, mach);
    return ret;
} 

int SchedChange::freeProcChange() 
{
    return isEnd ? getNodesNeeded(*j) : - getNodesNeeded(*j);
}

char* SchedChange::toString()
{
    char* retval = new char[100];
    sprintf(retval, "%ld : start Job %ld", time, j -> getJobNum());
    return retval;
}

void SchedChange::print()
{
    if (!isEnd) {
        schedout.debug(CALL_INFO, 7, 0, "%s Scheduled from %ld to %ld\n", j -> toString().c_str(), time, partner -> getTime());
    } else {
        schedout.debug(CALL_INFO, 7, 0, "%s Scheduled until %ld\n", j -> toString().c_str(), time);
    }
}

SchedChange::SchedChange(unsigned long intime, Job* inj, bool end, const Machine & inmach, SchedChange* inpartner) : mach(inmach)
{
    if (NULL == inpartner && !end) schedout.fatal(CALL_INFO, 1, "Schedchange beginning not given partner");
    partner = inpartner;
    time = intime;
    j = inj;
    isEnd = end;
}

//copy constructor (used in StatefulScheduler:: copy())
SchedChange::SchedChange(SchedChange* insc, const Machine & inmach) : mach(inmach)
{
    partner = insc -> partner;
    time = insc -> time;
    j = insc -> j;
    isEnd = insc -> isEnd;
}

void usage();

bool SCComparator::operator()(SchedChange* const& first, SchedChange* const& second)
{
    //needs to be checked for right direction
    if(first -> getTime() != second -> getTime()) {
        return first -> getTime() < second -> getTime(); 
    }
    if(first -> isEnd && !second -> isEnd) {
        return false;
    }
    if(!first -> isEnd && second -> isEnd) {
        return true;
    }
    if(first -> j -> getEstimatedRunningTime() != second -> j -> getEstimatedRunningTime()) {
        return first -> j -> getEstimatedRunningTime() < second -> j -> getEstimatedRunningTime();
    }
    return first -> j -> getJobNum() < second -> j -> getJobNum();
}


string StatefulScheduler::getSetupInfo(bool comment) 
{
    string com;
    if(comment)
        com="# ";
    else
        com="";
    string heartstring = heart->getString();
    return com + heartstring + com +
        "Comparator: " + compSetupInfo; 
}

string StatefulScheduler::DelayedCompressionManager::getString()
{
    return "DelayedCompression Less Conservative Scheduler\n"; 
}

string StatefulScheduler::ConservativeManager::getString()
{
    return "Conservative Scheduler\n"; 
}

string StatefulScheduler::PrioritizeCompressionManager::getString()
{
    return "PrioritizeCompression Scheduler\n"; 
}

string StatefulScheduler::EvenLessManager::getString()
{
    return "Even Less Conservative Scheduler\n"; 
}

void StatefulScheduler::jobArrives(Job* j, unsigned long time, const Machine & mach) 
{
    schedout.debug(CALL_INFO, 7, 0, "%ld: Job #%ld arrives\n", time, j -> getJobNum());
    scheduleJob(j, time);
    heart -> arrival(j, time);
}

//inserts a job into the estimated schedule as early as possible without causing conflicts
//updates jobToEvents accordingly
unsigned long StatefulScheduler::scheduleJob(Job* j, unsigned long time)
{
    unsigned long starttime;
    starttime = findTime(estSched, j, time);
    SchedChange* endChange = new SchedChange(starttime + j -> getEstimatedRunningTime(), j, true, mach, NULL);
    SchedChange* startChange = new SchedChange(starttime, j, false, mach, endChange);
    estSched -> insert(endChange);
    estSched -> insert(startChange);
    //the following is unnecessary for the most part, but if a job is reinserted can wind up with bad pointers
    jobToEvents -> erase(j); 
    jobToEvents -> insert(pair<Job*,SchedChange*>(j, startChange));
    return starttime;
}

//finds the earliest time when j can be scheduled
unsigned long StatefulScheduler::findTime(set<SchedChange*, SCComparator>* sched, Job *j, unsigned long time) 
{
    unsigned long intime = time;
    if (j -> getEstimatedRunningTime() == 0) return zeroCase(sched, j, intime);

    if(intime < j -> getArrivalTime()) intime = j -> getArrivalTime();

    set<SchedChange*, SCComparator>::iterator it = sched -> begin();
    //to traverse schedule
    int currentFree = freeProcs; //number of procs free at time being considered
    bool done = false;  //whether we've found working anchor point
    unsigned long anchorTime = intime; //anchor point; possible starting time for job
    SchedChange *sc = NULL;  //first change we haven't looked at yet

    while (!done) {
        //will exit because anchor point at end of schedule must work

        if (it != sched -> end()) {
            sc = *(it);
            unsigned long scTime = sc -> getTime();
            if (scTime <= anchorTime) {
                currentFree += sc -> freeProcChange();
            }
            else { //advanced to anchor point; now test it

                bool skip = false;  //to check if current procs falls below required amount
                while (!done && (currentFree >= getNodesNeeded(*j))) {
                    //process this change and any occuring at the same time
                    currentFree += sc -> freeProcChange();
                    if (sc -> j -> getEstimatedRunningTime() == 0
                        && currentFree < getNodesNeeded(*j)) {
                        skip = true;sc = NULL;
                    }
                    ++it; //in java we make sure that the NEXT element exists and satisfies the time constraints, so in C++ we increment now and dec at end of loop
                    while (it != sched -> end() && (*it) -> getTime() == scTime) {
                        sc = *(it);
                        currentFree += sc -> freeProcChange();
                        if (sc -> j -> getEstimatedRunningTime() == 0 && currentFree < getNodesNeeded(*j)) {
                            skip = true;
                        }
                        ++it;
                    }
                    --it;

                    if (skip) {
                        ++it;
                        if (it != sched -> end() && anchorTime == scTime) {
                            sc = *(it);
                        } else {
                            --it;
                            currentFree -= sc -> freeProcChange();
                        }
                        scTime = sc -> getTime();
                        break;
                    }

                    //check if we've gotten to where the job would end
                    ++it;
                    if ((scTime >= anchorTime + j->getEstimatedRunningTime()) || it == sched->end()) {
                        --it;
                        done = true;  //yes; use the anchor point
                    } else {
                        sc = *(it); //no; advance the time we're looking at
                    }
                    scTime = sc -> getTime();
                }
                if (!done) {  //not enough procs; advance anchorTime
                    anchorTime = sc -> getTime();
                    currentFree += sc -> freeProcChange();
                }
            }
            ++it;
        } else { //ran out of changes before anchor point so can use it
            if (currentFree != numProcs) {
                printPlan();
                schedout.fatal(CALL_INFO, 1, "Scheduler got to end of estimated schedule with %d processors free out of %d total\n", currentFree, numProcs);
            }
            done = true;
        }
    }
    return anchorTime;
}

//Finds the earliest time when a job with zero running time can be scheduled
//(requires some unusual considerations).
unsigned long StatefulScheduler::zeroCase(set<SchedChange*, SCComparator> *sched, Job* filler, unsigned long time) 
{
    //iterate through event list to find first time where
    //there are enough avaiable procs
    int avaProcs = freeProcs;
    unsigned long lookAtTime = time;//to keep track of time being checked
    if (avaProcs >= getNodesNeeded(*filler) ) {
        return time;
    }

    //if can't run right away iterate
    set<SchedChange*, SCComparator>::reverse_iterator tour = sched -> rend();
    //to traverse schedule
    SchedChange* sc;//event at current time
    int procsChangeDueToStart = 0;
    int procsChangeDueToZero = 0;

    while (tour != sched->rbegin()) {
        sc = *(--tour);
        if (sc->getTime() > lookAtTime) {
            lookAtTime = sc -> getTime();
            avaProcs += procsChangeDueToStart; //make sure to take change due to starting jobs in to consideration now
            procsChangeDueToStart = 0;
            procsChangeDueToZero = 0;
        }

        if (sc -> getTime() == lookAtTime) {
            //if event occured at current time,
            //change number of free procs
            if (sc -> j -> getEstimatedRunningTime() != 0) {
                if (sc -> isEnd) {
                    avaProcs += sc -> freeProcChange();
                } else {
                    procsChangeDueToStart += sc -> freeProcChange();//ignore jobs that start at current time for now
                }
            } else {
                procsChangeDueToZero += sc -> freeProcChange();
            }
            bool keeplooping = true; //ugly hack because c++ iterators can't peek
            while (tour != sched -> rbegin() && keeplooping) {
                tour--;
                if ((*tour) -> getTime() == lookAtTime) {
                    sc = (*tour);
                    if (sc -> j -> getEstimatedRunningTime() != 0) {
                        if (sc -> isEnd) {
                            avaProcs += sc->freeProcChange();
                        } else {
                            procsChangeDueToStart += sc -> freeProcChange();//ignore jobs that start at current time for now
                        }
                    } else {
                        procsChangeDueToZero += sc -> freeProcChange();
                    }
                } else {
                    keeplooping = false;
                    tour++;
                }
            }
        }
        if (sc -> getTime() < lookAtTime) {//if occured before, count
            avaProcs += sc -> freeProcChange();
        }

        //check and either end or increment
        if (avaProcs >= getNodesNeeded(*filler)) {
            //enough procs to run right away
            return lookAtTime;
        }
    }

    if (avaProcs >= getNodesNeeded(*filler)) {//check if enough procs
        return lookAtTime;
    } else {
        //got to end of list but still not enough procs
        avaProcs += procsChangeDueToZero;
        if (avaProcs != numProcs) {
            printPlan();
            schedout.fatal(CALL_INFO, 1, "Scheduler got to end of estimated schedule w/o all processors being free");
        }
    }
    return lookAtTime;
}

//Removes j from the list of running jobs and update start
void StatefulScheduler::jobFinishes(Job* j, unsigned long time, const Machine & mach)
{ 
    schedout.debug(CALL_INFO, 7, 0, "%s finishing at %lu\n", j -> toString().c_str(), time);
    set<SchedChange*, SCComparator>::reverse_iterator it = estSched -> rend();
    bool success = false;
    SchedChange* sc ;
    while (it != estSched -> rbegin()&& !success ) { 
        sc = *(--it);
        unsigned long scTime = sc -> getTime();
        if (scTime < time) {
            printPlan();
            schedout.fatal(CALL_INFO, 1, "expecting events in the past");
        }
        if (sc -> j -> getJobNum() == j -> getJobNum()) {
            if (!sc -> isEnd) schedout.fatal(CALL_INFO, 1, "Job finished before scheduler started it");
            success = true;
            freeProcs += getNodesNeeded(*j);
            estSched -> erase(sc);
            jobToEvents -> erase(j);
            if (time == scTime) {
                heart -> onTimeFinish(j,time);
                delete sc;//job's done, don't need schedchange for it anymore
                return; //job ended exactly as scheduled so no compression
            }
        }
    }
    heart -> earlyFinish(j,time);
    if (!success) schedout.fatal(CALL_INFO, 1, "Could not find finishing job in running list");
    delete sc;
}

//for debugging; prints planned schedule to stderr
void StatefulScheduler::printPlan() 
{
    //bool first = true;
    int procs = freeProcs;
    schedout.debug(CALL_INFO, 7, 0, "Planned Schedule: \n");
    for (set<SchedChange*, SCComparator>::iterator it = estSched -> begin(); it != estSched -> end(); ++it) {
        SchedChange* sc = *it;
        procs += sc -> freeProcChange();
        schedout.debug(CALL_INFO, 7, 0, "%d procs free so far|", procs);
        sc -> print();
        //first = false;
    }
    if (estSched -> empty())
        schedout.debug(CALL_INFO, 7, 0, "No jobs in estimated schedule\n");
    heart -> printPlan();
}

//removes job from schedule
void StatefulScheduler::removeJob(Job* j, unsigned long time)
{
    set<SchedChange*, SCComparator>::iterator it = estSched -> begin();
    while(it != estSched->end()) {
        SchedChange* sc = *it;
        if(sc -> j -> getJobNum() == j -> getJobNum()) {
            estSched -> erase(it);
            delete sc;
        }
        it++;
    }
}

//allows the scheduler to start a job if desired; time is current time
//called after calls to jobArrives and jobFinishes
//(either after each call or after each call occuring at same time)
//returns first job to start, NULL if none
//(if not NULL, should call tryToStart again)
Job* StatefulScheduler::tryToStart(unsigned long time, const Machine & mach) 
{
    heart -> tryToStart(time);
    set<SchedChange*, SCComparator>::iterator it = estSched -> begin();
    while (it != estSched -> end()) {
        SchedChange* sc = *(it); 
        unsigned long scTime = sc->getTime();
        if (scTime < time) {
            printPlan();
            schedout.fatal(CALL_INFO, 1, "Expecting events in the past");
        }
        if (scTime > time) {
            schedout.debug(CALL_INFO, 7, 0, "returning null\n");
            return NULL;
        }

        if (!sc -> isEnd) {
            if (mach.getNumFreeNodes() < getNodesNeeded(*(sc->j))) {
                nextToStart = NULL;
            } else {
                nextToStartTime = time;
                nextToStart = sc->j;
            }
            return nextToStart;
        }
        it++;
    }
    return NULL;
}

void StatefulScheduler::startNext(unsigned long time, const Machine & mach)
{
    if(nextToStart == NULL){
        schedout.fatal(CALL_INFO, 1, "Called startNext() job from scheduler when there is no available Job at time %lu",
                                      time);
    } else if(nextToStartTime != time){
        schedout.fatal(CALL_INFO, 1, "startNext() and tryToStart() are called at different times for Job #%ld",
                                      nextToStart->getJobNum());
    }
    
    freeProcs -= getNodesNeeded(*(sc->j));
    estSched -> erase(sc);
    jobToEvents -> erase(sc -> j);
    heart -> start(sc -> j, time);
    schedout.debug(CALL_INFO, 7, 0, "starting %s\n", sc -> j -> toString().c_str());
    sc -> print();
    delete sc; //once a job is started we don't need its schedchange anymore
}

void StatefulScheduler::reset() { }

//comparator functions:
StatefulScheduler::JobComparator::JobComparator(ComparatorType type) 
{
    this -> type = type;
}


void StatefulScheduler::JobComparator::printComparatorList(ostream& out) 
{
    for (int i = 0; i < numCompTableEntries; i++) {
        out << " " << compTable[i].name << endl;
        //schedout.verbose(CALL_INFO, 0, 0,  " %s\n", compTable[i].name.c_str());
    }
}

StatefulScheduler::JobComparator* StatefulScheduler::JobComparator::Make(string typeName) 
{
    for (int i = 0; i < numCompTableEntries; i++) {
        if (typeName == compTable[i].name) {
            return new StatefulScheduler::JobComparator(compTable[i].val);
        }
    }
    schedout.fatal(CALL_INFO, 1, "Cannot find the comparator named:%s", typeName.c_str());
    return NULL;
}

//manager functions:


//compresses the schedule so jobs start as quickly as possible
//it is up to the manager to make sure this does not conflict with its guarantees
void StatefulScheduler::Manager::compress(unsigned long time)
{
    set<SchedChange*, SCComparator> *oldEstSched = new set<SchedChange*, SCComparator>(*(scheduler -> estSched));
    SCComparator* sccomp = new SCComparator();
    delete scheduler -> estSched;
    scheduler -> estSched = new set<SchedChange*, SCComparator>(*sccomp );
    //schedout.output("in compress, old eS: %p, new eS: %p\n", oldEstSched, scheduler -> estSched);
    //fflush(stdout);

    //first pass; pick up unmatched ends
    //this must be done first so they appear when jobs are added
    for (set<SchedChange*, SCComparator>::iterator it = oldEstSched -> begin(); it != oldEstSched -> end(); it++) {
        SchedChange* sc = *it;
        if (!sc -> isEnd) {
            //schedout.output("deleting pointer from %p to %s\n", sc->getPartner(), sc->getPartner()->j->toString().c_str());
            sc->getPartner()->j = NULL; //so we ignore its partner
        } else {
            //if null we already saw its partner, otherwise it is the end of a running job so just copy it over
            if(sc -> j != NULL) scheduler -> estSched -> insert(sc);
        }
    }

    //second pass; add the jobs whose starts appeared (in other words the jobs that are not yet running)
    for (set<SchedChange*, SCComparator>::iterator it = oldEstSched -> begin(); it != oldEstSched -> end(); it++) {
        SchedChange* sc = *it;
        if (!sc -> isEnd) {
            unsigned long scTime = sc -> getTime();
            //scheduleJob will create two new SchedChanges and add them to EstSched
            unsigned long newStartTime = scheduler -> scheduleJob(sc -> j, time);
            if (newStartTime > scTime) schedout.fatal(CALL_INFO, 1, "Attempt to delay estimated start of Job");
            delete sc; 
        } else if (NULL == sc -> j) {
            //these were duplicated; if not null it was copied directly and we don't want to delete it
            delete sc; //we may or may not have added the element to the array; either way we no longer need it. oldEstSched is going to be filled with lots of bad pointers but the next thing we do is clear it anyway.
        }
    }

    fflush(stdout);
    oldEstSched -> clear();
    delete oldEstSched;
    delete sccomp;
}

StatefulScheduler::PrioritizeCompressionManager::PrioritizeCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp, int infillTimes, const Machine & inmach) : Manager(inmach)
{
    scheduler = inscheduler;
    origcomp = comp;
    backfill = new set<Job*, JobComparator>(*comp);
    fillTimes = infillTimes;
    numSBF = new int[fillTimes + 1];
    for (int x = 0; x < fillTimes + 1; x++) {
        numSBF[x] = 0;
    }
}

StatefulScheduler::PrioritizeCompressionManager::PrioritizeCompressionManager(PrioritizeCompressionManager* inmanager, set<Job*, JobComparator>* inbackfill, const Machine & inmach) : Manager(inmach)
{
    origcomp = inmanager -> origcomp;
    scheduler = inmanager -> scheduler;
    backfill = inbackfill;
    fillTimes = inmanager -> fillTimes;
    numSBF = new int[fillTimes + 1];
    for (int x = 0; x < fillTimes + 1; x++) {
        numSBF[x] = inmanager -> numSBF[x];
    }
}

StatefulScheduler::PrioritizeCompressionManager* StatefulScheduler::PrioritizeCompressionManager::copy(std::vector<Job*>* running, std::vector<Job*>* intoRun)
{
    set<Job*, JobComparator>* newbackfill = new set<Job*, JobComparator>(*origcomp);
    int notfound = 0;
    for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
        bool found = false;
        for (std::vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it) -> getJobNum() == (*it2) -> getJobNum()) {
                newbackfill -> insert(*it2);
                found = true;
            }
        }
        if (!found) notfound++;
    }
    if (notfound > 1) schedout.fatal(CALL_INFO, 1, "Prioritize Compression Manager could not find two jobs for its new backfill in copy()");
    return new PrioritizeCompressionManager(this, newbackfill, mach);
}

void StatefulScheduler::PrioritizeCompressionManager::reset()
{
    for (int x = 0; x < fillTimes + 1; x++) {
        numSBF[x] = 0;
    }
}

void StatefulScheduler::PrioritizeCompressionManager::printPlan() {}

void StatefulScheduler::PrioritizeCompressionManager::done()
{
    for(int i = 0; i < fillTimes; i++) {
        if(numSBF[i] != 0) schedout.verbose(CALL_INFO, 4, 0, "backfilled successfully %d times in a row %d times\n", i, numSBF[i]);
    }
}

//backfills and compresses as necessary
void StatefulScheduler::PrioritizeCompressionManager::earlyFinish(Job* j, unsigned long time)
{
    int times;
    bool exit = true;
    if (0 == fillTimes) {
        compress(time);
        return;
    }
    for (times = 0; times < fillTimes; times++) {
        //only backfill a certain number of times
        //iterate through backfill queue
        for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
            //store old start time
            SchedChange* oldStartTime = (scheduler -> jobToEvents -> find(*it) -> second);

            //remove from current scheduler
            scheduler -> estSched -> erase(oldStartTime);
            scheduler -> estSched -> erase(oldStartTime -> getPartner());

            unsigned long oldTime = oldStartTime -> getTime();
            unsigned long newTime = oldTime;
            scheduler -> jobToEvents -> erase(*it); // otherwise there are multiple pais for the job and the wrong one may be used
            //try to backfill job
            newTime = scheduler -> scheduleJob(*it, time);
            if (newTime > oldTime) { //not supposed to happen
                for( set<SchedChange*, SCComparator>::iterator sc = scheduler->estSched->begin(); sc != scheduler->estSched->end(); sc++) {
                    (*sc) -> print();
                }
                schedout.verbose(CALL_INFO, 4, 0, "old: %ld, new:%ld\nbackfilling: %s\n", oldTime, newTime, (*it) -> toString().c_str());
                oldStartTime -> print();
                schedout.fatal(CALL_INFO, 1, "PrioritizeCompression Backfilling gave a new reservation that was later than previous one");
            }
            delete oldStartTime;

            if (newTime < oldTime) {//backfill successful
                exit = false;
                break;
            } else {
                exit = true;
            }

        }
        if (exit) break;
    }

    //record results
    numSBF[times]++;

    //backfilling was cut off so we should compress
    if (!exit) compress(time);
}

StatefulScheduler::DelayedCompressionManager::DelayedCompressionManager(StatefulScheduler* inscheduler, JobComparator* comp, const Machine & inmach) : Manager(inmach)
{
    scheduler = inscheduler;
    backfill = new set<Job*, JobComparator>(*comp);
    results = 0;
    origcomp = comp;
}

StatefulScheduler::DelayedCompressionManager::DelayedCompressionManager(DelayedCompressionManager* inmanager, set<Job*, JobComparator>* inbackfill, const Machine & inmach) : Manager(inmach)
{
    scheduler = inmanager -> scheduler;
    backfill = inbackfill;
    results = inmanager -> results;
    origcomp = inmanager -> origcomp;
}

StatefulScheduler::DelayedCompressionManager* StatefulScheduler::DelayedCompressionManager::copy(std::vector<Job*>* running, std::vector<Job*>* intoRun)
{
    set<Job*, JobComparator>* newbackfill = new set<Job*, JobComparator>(*origcomp);
    int notfound = 0;
    for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
        bool found = false;
        for (std::vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it) -> getJobNum() == (*it2) -> getJobNum()) {
                newbackfill -> insert(*it2);
                found = true;
            }
        }
        if (!found) notfound++;
    }
    if (notfound > 1) schedout.fatal(CALL_INFO, 1, "Delayed Compression Manager could not find two jobs for its new backfill in copy()");
    return new DelayedCompressionManager(this, newbackfill, mach);
}

void StatefulScheduler::DelayedCompressionManager::reset(){
    results = 0;
    backfill -> clear();
}

void StatefulScheduler::DelayedCompressionManager::arrival(Job* j, unsigned long time)
{
    SchedChange* newJobStart = scheduler -> jobToEvents -> find(j) -> second;
    SchedChange* newJobEnd = newJobStart -> getPartner();

    scheduler -> estSched -> erase(newJobStart);
    scheduler -> estSched -> erase(newJobEnd);

    bool moved = false;
    backfill -> insert(j);


    //stop hole from being filled by newly arrived job

    //other more effecictive methods go here
    unsigned long start = newJobStart -> getTime();

    //check if any existing job can fill that spot
    for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {//iterate through backfill queue
        //remove from schedule

        SchedChange *oldStart = scheduler -> jobToEvents -> find(*it) -> second;
        SchedChange *oldEnd = oldStart -> getPartner();
        if ((*it) -> getJobNum() != j -> getJobNum()) {
            scheduler -> estSched -> erase(oldStart);
            scheduler -> estSched -> erase(oldEnd);
        }

        //check where would fit in schedule
        unsigned long newTime = scheduler -> findTime(scheduler -> estSched, *it, time);

        //if if can move it up
        if (newTime <= (start + j -> getEstimatedRunningTime()) &&
            newTime != oldStart -> getTime()) {

            scheduler -> scheduleJob(*it, time);
            if (((*it) -> getJobNum() == j -> getJobNum())) {
                moved = true;
            }
        } else {
            //else put back in old place
            if (!((*it) -> getJobNum() == j -> getJobNum())) {
                scheduler -> estSched -> insert(oldStart);
                scheduler -> estSched -> insert(oldEnd);
            }
        }
    }
    if (!moved) scheduler -> scheduleJob(j, time); //reschedule new job
    delete newJobStart;
    delete newJobEnd;
}


void StatefulScheduler::DelayedCompressionManager::printPlan()
{
    schedout.debug(CALL_INFO, 7, 0, " backfilling queue:\n");
    for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
        schedout.debug(CALL_INFO, 7, 0, "%s\n", (*it) -> toString().c_str());
    }
}


void StatefulScheduler::DelayedCompressionManager::done()
{
    schedout.debug(CALL_INFO, 7, 0, "Backfilled %d times\n", results);
}

void StatefulScheduler::DelayedCompressionManager::earlyFinish(Job* j, unsigned long time)
{
    fill(time);
}

void StatefulScheduler::DelayedCompressionManager::tryToStart(unsigned long time)
{
    fill(time);
}

void StatefulScheduler::DelayedCompressionManager::fill(unsigned long time)
{
    //backfills
    for(set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
        SchedChange* OldStart = scheduler -> jobToEvents -> find(*it) -> second;
        SchedChange* OldEnd = OldStart -> getPartner(); 

        scheduler -> estSched -> erase(OldStart);
        scheduler -> estSched -> erase(OldEnd);

        //check if readding to scheduler allows Job to start now
        if (time == scheduler -> findTime(scheduler -> estSched, *it, time)) {
            //if so reschedule
            scheduler -> scheduleJob(*it, time);
            results++;
            delete OldStart;
            delete OldEnd;
        } else {
            scheduler -> estSched -> insert(OldStart);
            scheduler -> estSched -> insert(OldEnd);
        }
    }
}

StatefulScheduler::EvenLessManager::EvenLessManager(StatefulScheduler* inscheduler, JobComparator* comp, int fillTimes, const Machine & inmach) : Manager(inmach)
{
    SCComparator* sccomp = new SCComparator();
    scheduler = inscheduler;
    backfill = new set<Job*, JobComparator>(*comp);
    guarantee = new set<SchedChange*, SCComparator>(*sccomp);
    delete sccomp;
    guarJobToEvents = new map<Job*, SchedChange*, JobComparator>(*comp);
    origcomp = comp;
    bftimes = fillTimes;
}

StatefulScheduler::EvenLessManager::EvenLessManager(EvenLessManager* inmanager, set<Job*, JobComparator>* inbackfill, const Machine & inmach) : Manager(inmach)
{ 
    //    set<Job*, JobComparator>* newbackfill = new set<Job*, JobComparator>(*origcomp);
    //   int notfound = 0;
    //   for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
    //       bool found = false;
    //       for (std::vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
    //           if ((*it) -> getJobNum() == (*it2) -> getJobNum()) {
    //               newbackfill -> insert(*it2);
    //               found = true;
    //           }
    //       }
    //       if (!found) notfound++;
    //   }
    //   if (notfound > 1) schedout.fatal(CALL_INFO, 1, "Prioritize Compression Manager could not find two jobs for its new backfill in copy()");
    //   return new PrioritizeCompressionManager(this, newbackfill);

    scheduler = inmanager -> scheduler;
    origcomp = inmanager -> origcomp;
    backfill = inbackfill;
    SCComparator* sccomp = new SCComparator();
    guarantee = new set<SchedChange*, SCComparator>(*sccomp);
    delete sccomp;
    guarJobToEvents = new map<Job*, SchedChange*, JobComparator>(*origcomp);
    bftimes = inmanager -> bftimes;
}

StatefulScheduler::EvenLessManager* StatefulScheduler::EvenLessManager::copy(std::vector<Job*>* running, std::vector<Job*>* intoRun)
{
    set<Job*, JobComparator>* newbackfill = new set<Job*, JobComparator>(*origcomp);
    int notfound = 0; 
    for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
        bool found = false;
        for (std::vector<Job*>::iterator it2 = intoRun -> begin(); !found && it2 != intoRun -> end(); it2++) {
            if ((*it) -> getJobNum() == (*it2) -> getJobNum()) {
                newbackfill -> insert(*it2);
                found = true;
            }
        }
        if (!found) notfound++;
    }
    if (notfound > 1) schedout.fatal(CALL_INFO, 1, "Even Less Conservative Manager could not find two jobs for its new backfill in copy()");
    EvenLessManager* ret = new EvenLessManager(this, newbackfill, mach);
    //must match pointers in gJTE and backfill because we will later index into
    //gJTE using the job pointer from backfill; also fill in guarantee 
    map<Job*, SchedChange*, JobComparator>* newguarJobToEvents = ret -> guarJobToEvents;
    set<SchedChange*, SCComparator>* newguarantee = ret -> guarantee;
    for (map<Job*, SchedChange*, JobComparator>::iterator it = guarJobToEvents -> begin(); it != guarJobToEvents -> end(); it++)
    {
        bool found = false;
        //find the job in newbackfill
        for (set<Job*, JobComparator>::iterator it2 = newbackfill -> begin(); !found && it2 != newbackfill -> end(); it2++) {
            if ((*it2) -> getJobNum() == it -> first -> getJobNum()) {
                found = true;
                SchedChange* tempsched = new SchedChange (it -> second, mach);
                tempsched -> j = *it2;
                newguarJobToEvents -> insert(pair<Job*, SchedChange*>(*it2, tempsched)); 
                newguarantee -> insert(tempsched);
                tempsched -> print();
            }
        }
        //if it's not there it has to be in running
        for (vector<Job*>::iterator it2 = running -> begin(); !found && it2 != running -> end(); it2++) {
            if ((*it2) -> getJobNum() == it -> first -> getJobNum()) {
                found = true;
                SchedChange* tempsched = new SchedChange (it -> second, mach);
                tempsched -> j = *it2;
                newguarJobToEvents -> insert(pair<Job*, SchedChange*>(*it2, tempsched)); 
                newguarantee -> insert(tempsched);
                tempsched -> print();
            }
        }
        if (!found) schedout.fatal(CALL_INFO, 1, "Could not find %s in new backfill\n", it -> first -> toString().c_str());
    } 
    printPlan();
    ret -> printPlan();
    return ret;
}

void StatefulScheduler::EvenLessManager::deepCopy(set<SchedChange*, SCComparator> *from, set<SchedChange*, SCComparator> *to, map<Job*, SchedChange*, JobComparator> *toJ) 
{
    // to->clear() does not actually delete elements so we use a manual loop
    set<SchedChange*, SCComparator>::iterator sc2 = to -> begin();
    while(!to -> empty())
    {
        sc2 = to -> begin();
        SchedChange* sc = *sc2;
        to -> erase(sc2);
        delete sc;
    }

    //the elements of toJ are of type pair<> (not a pointer) so can be cleared normally.
    toJ -> clear();

    for (set<SchedChange*, SCComparator>::iterator sc = from -> begin(); sc != from -> end(); sc++) {
        if (!((*sc) -> isEnd)) {
            SchedChange* je = new SchedChange((*sc) -> getTime()+(*sc) -> j -> getEstimatedRunningTime(), (*sc) -> j, true, mach, NULL);
            SchedChange* js = new SchedChange((*sc) -> getTime(),(*sc) -> j,false, mach, je);
            to -> insert(js);
            to -> insert(je);
            toJ -> insert(pair<Job*, SchedChange*>(js -> j,js));
        } else if ((*sc) -> isEnd) {
            if (toJ -> find((*sc) -> j) == toJ -> end()) { //there is no entry for j in toJ; the job is running
                SchedChange* je = new SchedChange((*sc) -> getTime(), (*sc) -> j, true, mach, NULL);
                to -> insert(je);
                toJ -> insert(pair<Job*, SchedChange*>(je -> j,je));
            }
        }
    }
}

void StatefulScheduler::EvenLessManager::backfillfunc(unsigned long time)
{
    for (int i = 0; i < bftimes; i++) {
        for (set<Job*, JobComparator>::iterator it = backfill -> begin(); it != backfill -> end(); it++) {
            SchedChange* js = (scheduler -> jobToEvents -> find(*it) -> second);

            //set<SchedChange*,SCComparator>::iterator lower = scheduler -> estSched -> lower_bound(js -> getPartner());
            set<SchedChange*,SCComparator>::iterator upper = scheduler -> estSched -> upper_bound(js -> getPartner());
            --upper;

            scheduler -> estSched -> erase(js);
            scheduler -> estSched -> erase(js -> getPartner());
            scheduler -> jobToEvents -> erase(*it);

            unsigned long old = js -> getTime();
            unsigned long start = scheduler -> findTime(scheduler -> estSched,*it,time);
            SchedChange* je = new SchedChange(start+(*it) -> getEstimatedRunningTime(),(*it), true, mach, NULL);
            SchedChange* js2 = new SchedChange(start,(*it), false, mach, je);
            scheduler -> estSched -> insert(js2);
            scheduler -> estSched -> insert(je);
            scheduler -> jobToEvents -> insert(pair<Job*,SchedChange*>((*it),js2));

            if (start == time && old != time) {
                //We are trying to run something now, check if it would kill the guaranteed
                //schedule

                SchedChange* gjs = guarJobToEvents -> find((*it)) -> second;
                SchedChange* gje = gjs -> getPartner();

                guarantee -> erase(gjs);
                guarantee -> erase(gje);

                //Try adding the new start time to the guarantee
                //schedule and check if it is broken.
                guarantee -> insert(je);
                guarantee -> insert(js2);

                int freeprocs = scheduler -> freeProcs;
                //We keep track of the last time seen so that the order of
                //jobs at the same time does not cause a negative value
                unsigned long last = (unsigned long)-1;
                bool destroyed = false;
                //String reason = "";
                for (set<SchedChange*,SCComparator>::iterator sc = guarantee -> begin(); sc != guarantee -> end(); sc++) {
                    //If this is a new time from the last time

                    if (last!=(*sc) -> getTime() && freeprocs < 0) {
                        destroyed = true;
                        schedout.debug(CALL_INFO, 4, 0, "Bad at job |" );
                        (*sc) -> print();
                        schedout.debug(CALL_INFO, 4, 0, "\nwhen inserting ");
                        je -> print();
                        int tempfreeprocs = scheduler -> freeProcs;
                        schedout.debug(CALL_INFO, 4, 0, "\nguarantee:\n");
                        for (set<SchedChange*,SCComparator>::iterator sc3 = guarantee -> begin(); sc3 != guarantee -> end(); sc3++) {
                            tempfreeprocs+= (*sc3) -> freeProcChange();
                            schedout.debug(CALL_INFO, 4, 0, "%d |",tempfreeprocs);
                            (*sc3) -> print();
                        }
                        tempfreeprocs = scheduler -> freeProcs;
                        schedout.debug(CALL_INFO, 4, 0, "\nestSched:\n");
                        for (set<SchedChange*,SCComparator>::iterator sc3 = scheduler -> estSched -> begin(); sc3 != scheduler -> estSched -> end(); sc3++) {
                            tempfreeprocs+= (*sc3) -> freeProcChange();
                            schedout.debug(CALL_INFO, 4, 0, "%d |",tempfreeprocs);
                            (*sc3) -> print();
                        }
                    }

                    last = (*sc) -> getTime();
                    freeprocs += (*sc) -> freeProcChange();
                }
                if (destroyed || freeprocs<0 || freeprocs != scheduler -> numProcs) {
                    //The schedule is impossible.

                    schedout.debug(CALL_INFO, 4, 0, ": backfilling of destroys schedule ()\n");

                    //Use the estimated schedule instead.

                    //if we don't take these out, they are in both estSched and guar,
                    //and deepcopy deletes and then copies them
                    guarantee -> erase(je);
                    guarantee -> erase(js2);
                    deepCopy(scheduler -> estSched, guarantee, guarJobToEvents);
                } else {
                    //Guaranteed schedule looks OK.
                    //Remove the temporary changes.
                    guarantee -> erase(je);
                    guarantee -> erase(js2);
                    guarantee -> insert(gje);
                    guarantee -> insert(gjs);
                }
            }
            delete js -> getPartner();
            delete js;//new ones are made to replace them (je and js2)

            if (start < old) {
                //Backfilled to earlier time.
                schedout.debug(CALL_INFO, 7, 0, "%lu: backfilled %s to %lu from %lu\n", time, (*it)->toString().c_str(), start, old);
                break;
            } else if (start > old) {
                schedout.output("%lu: Backfilling error, plan:\n", time);
                scheduler -> printPlan();
                schedout.fatal(CALL_INFO, 1, "ELC gave a worse start time to %s. Old: %lu, New: %lu\n", (*it)->toString().c_str(), old, start);
            } else {
                scheduler -> printPlan();
                schedout.debug(CALL_INFO, 7, 0, "Unable to backfill\n");
            }
        }
    }
    compress(time);
}



void StatefulScheduler::EvenLessManager::reset(){ }

void StatefulScheduler::EvenLessManager::arrival(Job* j, unsigned long time)
{
    unsigned long gtime = scheduler -> findTime(guarantee,j,time);
    SchedChange* je = new SchedChange(gtime+j -> getEstimatedRunningTime(),j,true,mach,NULL);
    SchedChange* js = new SchedChange(gtime,j,false,mach,je);
    guarantee -> insert(js);
    guarantee -> insert(je);
    guarJobToEvents -> insert(pair<Job*,SchedChange*>(j,js));
    backfill -> insert(j);
    deepCopy(guarantee, scheduler -> estSched, scheduler -> jobToEvents);
    backfillfunc(time);
}

void StatefulScheduler::EvenLessManager::printPlan()
{
    int free = scheduler -> freeProcs;
    schedout.debug(CALL_INFO, 7, 0, "Guaranteed plan:\n");
    for( set<SchedChange*, SCComparator>::iterator sc = guarantee -> begin(); sc != guarantee -> end(); sc++)
    {
        free+=(*sc) -> freeProcChange();
        (*sc) -> print(),
        schedout.debug(CALL_INFO, 7, 0, "\t %d",  free);
    }
    schedout.debug(CALL_INFO, 7, 0, "\n");
}


void StatefulScheduler::EvenLessManager::earlyFinish(Job* j, unsigned long time)
{
    schedout.debug(CALL_INFO, 7, 0, "Early Finish\n");
    SchedChange* sc = guarJobToEvents -> find(j) -> second;
    guarantee -> erase(sc);
    guarJobToEvents -> erase(j);

    deepCopy(guarantee, scheduler -> estSched, scheduler -> jobToEvents);
    backfillfunc(time);
}

void StatefulScheduler::EvenLessManager::onTimeFinish(Job* j, unsigned long time)
{ 
    schedout.debug(CALL_INFO, 7, 0, "On Time Finish\n");
    if (guarJobToEvents -> find(j) == guarJobToEvents -> end()) {
        for (map<Job*, SchedChange*, JobComparator>::iterator it = guarJobToEvents -> begin(); it != guarJobToEvents -> end(); it++) {
            it -> second -> print();
        }
        schedout.fatal(CALL_INFO, 1, "Could not find %s in guarJobToEvents\n", j -> toString().c_str());
    }
    SchedChange* sc = guarJobToEvents -> find(j) -> second;
    if (sc == NULL) schedout.fatal(CALL_INFO, 1, "SchedChange for %s is %p", j -> toString().c_str(), sc);
    guarantee -> erase(sc);
    guarJobToEvents -> erase(j);
} 

void StatefulScheduler::EvenLessManager::start(Job* j, unsigned long time)
{
    //Remove the job's current guarantees and add a guaranteed end time
    SchedChange* js = guarJobToEvents -> find(j) -> second;
    //guarJobToEvents -> erase(j);
    guarantee -> erase(js);
    //set<SchedChange*, SCComparator>::iterator temp2 = guarantee -> find(js -> getPartner());
    guarantee -> erase(js -> getPartner());
    backfill -> erase(j);
    SchedChange* temp = new SchedChange(time + j -> getEstimatedRunningTime(), j, true, mach);
    guarantee -> insert(temp);
    guarJobToEvents -> find(j) -> second = temp;
    //guarJobToEvents -> insert(pair<Job*,SchedChange*>(j,temp));
    int freeprocs = scheduler -> freeProcs;

    //We keep track of the last time seen so that the order of
    //jobs at the same time does not cause a negative value
    unsigned long last = (unsigned long)-1;
    bool destroyed = false;
    for (set<SchedChange*, SCComparator>::iterator sc = guarantee -> begin(); sc != guarantee -> end(); ) {
        //If this is a new time from the last time
        if (last != (*sc) -> getTime() && freeprocs < 0) { //the last test is because a negative (temporarily) is OK if they're all at the same time; and any needed processors are immediately released.
            destroyed = true;
            schedout.debug(CALL_INFO, 4, 0, "Bad at point %ld | %d \n", (*sc) -> getTime(), freeprocs);

        }
        freeprocs += (*sc) -> freeProcChange();
        last = (*sc) -> getTime();
        sc++;
    }
    if (destroyed || freeprocs < 0 || freeprocs != scheduler -> numProcs) {
        //The schedule is impossible.
        if (freeprocs < 0) schedout.debug(CALL_INFO, 4, 0, "Negative procs at end | \n");
        if (freeprocs!=scheduler -> numProcs) schedout.debug(CALL_INFO, 4, 0, "All procs not freed\n");
        scheduler -> printPlan();

        schedout.debug(CALL_INFO, 4, 0, "the schedule is impossible, using estimated schedule instead\n");

        //Use the estimated schedule instead.
        deepCopy(scheduler -> estSched,guarantee,guarJobToEvents);
        scheduler -> printPlan();
    } else {
        //deepCopy(guarantee,scheduler -> estSched,scheduler -> jobToEvents);
        //(this was commented out in the Java as well)
    }
}

//JobComparator operator
bool StatefulScheduler::JobComparator::operator()(Job* const& j1,Job* const& j2) const
{ 
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
        if(j1 -> getProcsNeeded() != j2 -> getProcsNeeded())
            return (j2 -> getProcsNeeded() < j1 -> getProcsNeeded());

        //Secondary criteria: Longest Run Time
        if(j1 -> getEstimatedRunningTime() != j2 -> getEstimatedRunningTime()) {
            return (j2 -> getEstimatedRunningTime() < j1 -> getEstimatedRunningTime());
        }

        //Tertiary criteria: Arrival Time
        if(j1 -> getArrivalTime() != j2 -> getArrivalTime()) {
            return (j2 -> getArrivalTime() > j1 -> getArrivalTime());
        }

        //break ties so different jobs are never equal:
        return j2 -> getJobNum() > j1 -> getJobNum();

    default:
        schedout.fatal(CALL_INFO, 1, "operator() called on JobComparator w/ invalid type");
        return true; //never reach here
    }
}


string StatefulScheduler::JobComparator::toString() 
{
    switch (type) {
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
        schedout.fatal(CALL_INFO, 1, "toString() called on JobComparator w/ invalid type");
    }
    return ""; //never reach here...
}

int StatefulScheduler::getNodesNeeded(const Job & j) const
{
    return ceil(((float)j.getProcsNeeded()) / mach.coresPerNode);
}

int SchedChange::getNodesNeeded(const Job & j) const
{
    return ceil(((float)j.getProcsNeeded()) / mach.coresPerNode);
}

