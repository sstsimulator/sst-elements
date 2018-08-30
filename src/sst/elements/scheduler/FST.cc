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
 * Computes the FST for each job that comes in
 */

#include "sst_config.h"
#include "FST.h"

#include <vector>
#include <map>

#include "AllocInfo.h"
#include "Job.h"
#include "output.h"
#include "Scheduler.h"
#include "allocators/SimpleAllocator.h"
#include "SimpleMachine.h"
#include "TaskMapInfo.h"
#include "taskMappers/SimpleTaskMapper.h"

using namespace SST::Scheduler;
using namespace std;

FST::FST(int inrelaxed)
{
    //keeps track of job copies so we have a pointer when they actually start
    running = new vector<Job*>;
    toRun = new vector<Job*>;
    schedout.init("", 8, 0, Output::STDOUT);

    if (inrelaxed == 1) {
        relaxed = false;
    } else if (inrelaxed == 2) {
        relaxed = true;
    } else {
        schedout.fatal(CALL_INFO, 1, "Passed %d to FST constructor; should be 1 or 2", inrelaxed);
    }
}

//This would normally be a part of the constructor but we need the number of
//jobs, so schedComponent calls it later. 
void FST::setup(int innumjobs)
{
    numjobs = innumjobs;
    jobFST = new unsigned long[numjobs]; 
}

//used as a comparator to make sure our simulation considers events in the
//right order.  We assume that events that finish at the same time arrive in
//order of event number (this also matches the Java), which corresponds to the
//start time of the job 
bool compevents(Job* j1, Job* j2) {
    if (j1 -> getStartTime() + j1 -> getActualTime() == j2 -> getStartTime() + j2 -> getActualTime()) {
        //return j1 -> getJobNum() < j2 -> getJobNum();
        return j1 -> getStartTime() < j2 -> getStartTime();
    }
    return j1 -> getStartTime() + j1 -> getActualTime() < j2 -> getStartTime() + j2 -> getActualTime();
}

//When a job arrives, we need to calculate its FST value
void FST::jobArrives(Job *inj, Scheduler* insched, Machine* inmach)
{ 
    schedout.debug(CALL_INFO, 7, 0, "%s arriving to FST\n", inj -> toString().c_str());
    
    Job *j = new Job(*inj); //must copy the job because they keep track of when they each start

    //if the schedule is not relaxed the job has already been added; otherwise
    //we want to add it later (but only once)
    bool alreadyadded = !relaxed;

    if (!relaxed) {
        toRun -> push_back(j); //keep the pointer for the future so we can add it to running
        //(if relaxed we'll add it later, after all the other jobs complete)
    }

    //if there are no running jobs, no need for simulation
    if (running -> empty()) {
        schedout.debug(CALL_INFO, 7, 0, "no need for simulation\n");
        jobFST[j -> getJobNum()] = j -> getArrivalTime();
        schedout.debug(CALL_INFO, 7, 0, "Assigning FST of %lu to Job %ld\n", jobFST[j -> getJobNum()], j -> getJobNum());
        if (relaxed) {
            toRun -> push_back(j); //keep the pointer for the future so we can add it to running
        }
        return;
    }

    //create copies of the scheduler, machine, and allocator for our simulation
    Scheduler* sched = insched -> copy(running, toRun);
    Machine* mach = new SimpleMachine(inmach->numNodes, true, inmach->coresPerNode, NULL);
    Allocator* alloc = new SimpleAllocator((SimpleMachine*) mach);
    TaskMapper* taskMap = new SimpleTaskMapper(*mach);
    string nullstr = "";
    char nullcstr[] = "";
    map<Job*, TaskMapInfo*>* jobToAi = new map<Job*, TaskMapInfo*>();

    for (unsigned int x = 0; x < running -> size(); x++) {
        AllocInfo* ai = alloc -> allocate(running -> at(x));
        if (NULL == ai){
            schedout.fatal(CALL_INFO, 1, "in FST could not allocate running job\nMachine had %d processors for %s", mach -> getNumFreeNodes(), running -> at(x) -> toString().c_str());
        }
        TaskMapInfo* tmi = taskMap->mapTasks(ai);
        mach -> allocate(tmi);
        jobToAi -> insert(pair<Job*, TaskMapInfo*>(running -> at(x), tmi));
        delete tmi;
    }

    Statistics* stats = new Statistics(mach, sched, alloc, taskMap, nullstr.c_str(), nullcstr, true, this);

    //have to keep track of each job's end time
    bool(*compeventsptr)(Job*, Job*) = compevents;
    std::multimap<Job*, unsigned long, bool(*)(Job*, Job*)>* endtimes = 
        new std::multimap<Job*, unsigned long, bool(*)(Job*, Job*)>(compeventsptr); 

    for(unsigned int x = 0; x < running -> size(); x++) {
        endtimes -> insert(pair<Job*, unsigned long>(running -> at(x), running -> at(x) -> getStartTime() + running -> at(x) -> getActualTime()));
    }

    //the scheduler will delete jobs once they finish; we'll still need them
    //for the next simulation though
    std::vector<Job*>* readdtorunning = new vector<Job*>; 
    std::vector<Job*>* readdtorun = new vector<Job*>; 

    //step through each waiting job, including j.  Tell the scheduler that it
    //arrives.  In the meantime, keep an eye on endtimes to make sure we tell
    //the scheduler about any job that's finishing.
    unsigned long time = j -> getArrivalTime();
    bool success = false;
    //see if we can start now
    if (toRun -> empty() && !alreadyadded) {
        toRun -> push_back(j);
        sched -> jobArrives(j, j -> getArrivalTime(), *mach);
        alreadyadded = true;
    } 
    success = FSTstart(endtimes, jobToAi, j, sched, alloc, mach, stats, time);

    if (!alreadyadded) {
        bool allothersstarted = true;
        for (unsigned int x = 0; allothersstarted && x < toRun -> size(); x++) {
            if (!toRun -> at(x) -> hasStarted()) {
                allothersstarted = false;
            }
        }
        if (allothersstarted) {
            toRun -> push_back(j);
            sched -> jobArrives(j, time, *mach);
            stats -> jobArrives(time);
            alreadyadded = true;
            //try to start again in case we can start the job right away
            success = FSTstart(endtimes, jobToAi, j, sched, alloc, mach, stats, time);
        }
    }


    //step through each running job.  When it finishes  we tell the scheduler
    //it finishes, then try to start a new job.  If the new job that gets
    //started is j, we assign that as the FST value of j
    while (!success && !endtimes -> empty()) { 
        time = endtimes -> begin() -> second;
        bool found = false;

        //find our job in toRun or running and erase it, but keep track of the pointer for later
        for (unsigned int x = 0; !found && x < running -> size(); x++) {
            if (running -> at(x) == endtimes -> begin() -> first) {
                running -> erase(running -> begin() + x);
                readdtorunning -> push_back(new Job(*(endtimes -> begin() -> first)));
                found = true;
            }
        }
        for (unsigned int x = 0; !found && x < toRun -> size(); x++) {
            if (toRun -> at(x) == endtimes -> begin() -> first) {
                toRun -> erase(toRun -> begin() + x);
                readdtorun-> push_back(new Job(*(endtimes -> begin() -> first)));
                found = true;
            }
        } 
        if (!found) schedout.fatal(CALL_INFO, 1, "Could not find %s in running or toRun", endtimes->begin()->first->toString().c_str());

        if(jobToAi -> find(endtimes -> begin() -> first) == jobToAi -> end()) schedout.fatal(CALL_INFO, 1, "couldn't find %s in jobToAi", endtimes ->begin() -> first -> toString().c_str());

        //tell our simulated scheduler etc that the job completed
        mach -> deallocate(jobToAi -> find(endtimes -> begin() -> first) -> second);
        alloc -> deallocate(jobToAi -> find(endtimes -> begin() -> first) -> second -> allocInfo);
        sched -> jobFinishes(endtimes -> begin() -> first, time, *mach);
        endtimes -> erase(endtimes -> begin());

        //this check is because we don't want to start until all jobs that have completed at the same time to finish
        //(may cause a larger job to be able to start as more processors become available)
        if (endtimes -> begin() -> second != time) {
            success = FSTstart(endtimes, jobToAi, j, sched, alloc, mach, stats, time);

            //now that a new job is started, see if we can add ours
            if (relaxed && !alreadyadded) {
                bool allothersstarted = true;
                for (unsigned int x = 0; allothersstarted && x < toRun -> size(); x++) {
                    if (!toRun -> at(x) -> hasStarted()) {
                        allothersstarted = false;
                    }
                }
                if (allothersstarted) {
                    toRun -> push_back(j);
                    sched -> jobArrives(j, time, *mach);
                    stats -> jobArrives(time);
                    alreadyadded = true;
                    //try to start again in case we can start the job right away
                    success = FSTstart(endtimes, jobToAi, j, sched, alloc, mach, stats, time);
                }
            }
        } 
    }
    if (!success) schedout.fatal(CALL_INFO, 1, "Could not find time for %s in FST\n", j -> toString().c_str());

    //clean up
    delete sched;
    delete mach;
    delete alloc;
    delete taskMap;
    delete stats;
    delete endtimes;
    for (map<Job*, TaskMapInfo*>::iterator it = jobToAi->begin(); it != jobToAi -> end(); it++) {
        delete it -> second;
    }
    jobToAi -> clear();
    delete jobToAi;
    while (!readdtorunning -> empty()) {
        running -> push_back(readdtorunning -> back());
        readdtorunning -> pop_back();
    }
    while (!readdtorun-> empty()) {
        toRun -> push_back(readdtorun -> back());
        readdtorun -> pop_back();
    }
    //some of these jobs may have been started during our simulation; reset them
    for (unsigned int x = 0; x < toRun -> size(); x++) {
        toRun -> at(x) -> reset();
    }
    delete readdtorunning;
    delete readdtorun;
}

//helper function; tells the scheduler to start a job
bool FST::FSTstart(std::multimap<Job*, unsigned long, bool(*)(Job*, Job*)>* endtimes,
                   std::map<Job*, TaskMapInfo*>* jobToAi, Job* j, Scheduler* sched, 
                   Allocator* alloc, Machine* mach, Statistics* stats, unsigned long time) 
{
    AllocInfo* ai;
    Job* newJob;
    SimpleTaskMapper taskMapper = SimpleTaskMapper(*mach);
    TaskMapInfo* tmi;
    do {
        newJob = sched->tryToStart(time, *mach);
        sched->startNext(time, *mach);
        ai = alloc->allocate(newJob);
        if (ai != NULL) {
            tmi = taskMapper.mapTasks(ai);
            if (ai -> job == j) {
                //our job has been scheduled!  record the time
                jobFST[j -> getJobNum()] = time;
                schedout.debug(CALL_INFO, 7, 0, "Assigning FST of %lu to Job %ld\n", jobFST[j -> getJobNum()], j -> getJobNum());
                j -> reset();
                return true;
            }
            else {
                schedout.debug(CALL_INFO, 7, 0, "%lu: FST starting %s\n", time, ai -> job -> toString().c_str());
                endtimes -> insert(pair<Job*, unsigned long>(ai -> job, time + ai -> job -> getActualTime()));
                jobToAi -> insert(pair<Job*, TaskMapInfo*>(ai -> job, tmi));
            }
            stats -> jobStarts(tmi, time);
        }
    } while (ai != NULL); 
    return false; 
}

//when a job finishes, we just remove it from running
void FST::jobCompletes(Job* j)
{
    schedout.debug(CALL_INFO, 7, 0, "%s completing in FST\n", j -> toString().c_str());
    for(vector<Job*>::iterator it = running -> begin(); it != running -> end(); it++) {
        if ((*it) -> getJobNum() == j -> getJobNum()) {
            delete *it;
            running -> erase(it);
            return;
        }
    }
    schedout.fatal(CALL_INFO, 1, "FST could not find completing job in its currently-running list\n");
}

//when a job starts, we must move it to running from toRun
void FST::jobStarts(Job* j, unsigned long time) 
{
    //Need to add the job to running when it actually starts.  But, we want to
    //add the copied version, not the actual version (or this simulation kills
    //our schedule)
    for(unsigned int x = 0; x < toRun -> size(); x++) {
        if(toRun -> at(x) -> getJobNum() == j -> getJobNum()) {
            Job* startingjob = toRun -> at(x);
            startingjob -> startsAtTime(time);
            running -> push_back(startingjob); 
            toRun -> erase (toRun -> begin() + x);
            return;
        } 
    }
    schedout.fatal(CALL_INFO, 1, "Job %ld started before FST was aware it arrived\n", j -> getJobNum());
}

//returns the FST value for job num
unsigned long FST::getFST(int num)
{
    if(num < 0 || num >= numjobs) schedout.fatal(CALL_INFO, 1, "trying to get FST value out of range: %d", num);
    return jobFST[num];
}


/*    
      for (unsigned int simjob = 0; simjob < running -> size(); simjob++) {
//tell the scheduler about all jobs that finish before the next running job starts
while (!endtimes -> empty() && endtimes -> begin() -> first <= running -> at(simjob) -> getArrivalTime()) {
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("%d arriving at %lu\nstart time:%lu\n", endtimes -> begin() -> second ->job -> getJobNum(), endtimes -> begin() -> first, endtimes ->begin()->second -> job ->getStartTime());
if (!jinserted && endtimes -> begin() -> first > j -> getArrivalTime()) {
//tell scheduler about our job 
unsigned long time = j -> getArrivalTime()3
4 processors free
;
sched -> jobArrives(j, j -> getArrivalTime(), mach);
AllocInfo* ai;
do {
ai = sched -> tryToStart(alloc, time, mach, stats);
if (ai -> job == j) {
//our job has been scheduled!  record the time
//schedout.debug(CALL_INFO, 7, 0, "success");
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("setting FST of %d to %lu\n", j -> getJobNum(), time);
jobFST[j -> getJobNum()] = time;

delete endtimes;
//running -> push_back(j);
return;
}
endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
} while (ai != NULL);
jinserted = true;
}
//job finishes
unsigned long time = endtimes -> begin() -> first;
AllocInfo* ai = endtimes -> begin() -> second;
mach -> deallocate(ai);
alloc -> deallocate(ai);
sched -> jobFinishes(ai -> job, time, mach);
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("deleting %d from endtimes\n",endtimes->begin()->second->job->getJobNum());
endtimes -> erase (endtimes -> begin());
delete ai;
//see if we can start any jobs
do {
ai = sched -> tryToStart(alloc, time, mach, stats);
if (ai != NULL) {
if (ai -> job == j) {
schedout.debug(CALL_INFO, 7, 0, "success2\n");
//our job has been scheduled!  record the time
jobFST[j -> getJobNum()] = time;

delete endtimes;
//running -> push_back(j);
return;
}
endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
}
} while (ai != NULL); 
} 
//if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("outside while loop\n");
//don't think this ever happens
if (!jinserted && running -> at(simjob) -> getArrivalTime() > j -> getArrivalTime()) {
//tell scheduler about our job 
sched -> jobArrives(j, j -> getArrivalTime(), mach);
jinserted = true;
AllocInfo* ai;
unsigned long time = j -> getArrivalTime();
do {
ai = sched -> tryToStart(alloc, time, mach, stats);
if (ai != NULL) {
if (ai -> job == j) {
//our job has been scheduled!  record the time
jobFST[j -> getJobNum()] = time;

delete endtimes;
//running -> push_back(j);
return;
}
endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
}
} while (ai != NULL);

}
//if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("before next while loop\n");
unsigned long time = running -> at(simjob) -> getArrivalTime();
//tell scheduler about the next job
sched -> jobArrives(running -> at(simjob), time, mach);
//see if we can start any jobs
AllocInfo* ai;
do {
    ai = sched -> tryToStart(alloc, time, mach, stats);
    if (ai != NULL) {
        if (ai -> job == j) {
            //our job has been scheduled!  record the time
            //schedout.fatal(CALL_INFO, 1, "success!");
            jobFST[j -> getJobNum()] = time;

            delete endtimes;
            //running -> push_back(j);
            return;
        }
        endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
        if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
    }
} while (ai != NULL);
}
if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("outside big for loop\n");
schedout.output("Running after big for: ");
for(int x = 0; x < running -> size(); x++)
schedout.output(" %s", running -> at(x) -> toString().c_str());
schedout.output("\n");
//schedout.output("must walk through remaining jobs:\n");

//no more jobs arriving, must walk through jobs finishing until j can be scheduled

schedout.output("endtimes: ");
for(std::multimap<unsigned long, AllocInfo*>::iterator it = endtimes -> begin(); it != endtimes -> end(); it++) {
    schedout.output("%ld (%ld) ", it -> second -> job -> getJobNum(), it -> first);
}
schedout.output("\n");

while (!endtimes -> empty()) { 
    if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("popping %s off endtimes at %lu\nstart time:%lu\nfreeprocs left:%d\n", endtimes -> begin() -> second -> job -> toString().c_str(), endtimes->begin()->first, endtimes ->begin()->second -> job ->getStartTime(), mach->getNumFreeNodes()); 
    if (!jinserted && endtimes -> begin() -> first > j -> getArrivalTime()) {
        //schedout.output("found time for job %s\n", j ->
        //toString().c_str()); tell scheduler about our job 
        unsigned long time = j -> getArrivalTime(); 
        sched -> jobArrives(j, time, mach);
        jinserted = true;
        //see if we can start any jobs

        AllocInfo* ai; 
        do { 
            ai = sched -> tryToStart(alloc, time, mach, stats); 
            if (ai != NULL) { 
                if (ai -> job == j) { 
                    //our job has been scheduled!  record the time
                    //schedout.debug(CALL_INFO, 7, 0, "found FST for %s :
                    //%lu", j -> toString().c_str(), time);
                    jobFST[j -> getJobNum()] = time;

                    delete endtimes;
                    //running -> push_back(j);
                    return;
                }
                endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
                if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
            }
        } while (ai != NULL);

    }
    unsigned long time = endtimes -> begin() -> first;
    AllocInfo* ai = endtimes -> begin() -> second;
    mach -> deallocate(ai);
    alloc -> deallocate(ai);
    sched -> jobFinishes(ai -> job, time, mach);
    if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("deleting %d from endtimes\n", endtimes->begin()->second->job->getJobNum());
    endtimes -> erase (endtimes -> begin());
    delete ai;
    //see if we can start any jobs
    //schedout.output("before trying to start loop\n");
    do {
        schedout.output("fp before TTS: %d", mach->getNumFreeNodes());
        ai = sched -> tryToStart(alloc, time, mach, stats);
        if (ai != NULL) {
            if (ai -> job == j) {
                if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("scheduling in last loop at %lu, fp: %d\n", time, mach->getNumFreeNodes());
                schedout.output("Running when sched in last loop: ");
                for(int x = 0; x < running -> size(); x++)
                    schedout.output(" %s", running -> at(x) -> toString().c_str());
                schedout.output("\n");
                //our job has been scheduled!  record the time
                //schedout.output("FSTfound time for job");
                jobFST[j -> getJobNum()] = time;

                delete endtimes;
                //running -> push_back(j);
                return;
            } else {
                schedout.output("started %s instead at %ld\n", ai -> job ->toString().c_str(), ai -> job -> getStartTime());
            }
            endtimes -> insert(std::pair<unsigned long, AllocInfo*>(time + ai -> job -> getActualTime(), ai));
            if (j -> getJobNum() == 37827 || j -> getJobNum() == 37828) schedout.output("inserting %d to endtimes\n", ai ->job ->getJobNum());
        } 
    } while (ai != NULL);
}
schedout.fatal(CALL_INFO, 1, "Could not get FST for %s", j -> toString().c_str() );

*/

