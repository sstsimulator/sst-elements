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

#include "sst_config.h"
#include "Job.h"

#include <stdio.h>
#include <string>

#include <iostream> //debug

#include "output.h"
#include "TaskCommInfo.h"

using namespace SST::Scheduler;

static long nextJobNum = 0;  //used setting jobNum

//NetworkSim: added PhaseInfo
Job::Job(unsigned long arrivalTime, int procsNeeded, unsigned long actualRunningTime, unsigned long estRunningTime, CommInfo inComm, PhaseInfo inPhase) : commInfo(inComm), phaseInfo(inPhase)
{
    schedout.init("", 8, 0, Output::STDOUT);
    initialize(arrivalTime, procsNeeded, actualRunningTime, estRunningTime);
}

//NetworkSim: added PhaseInfo
Job::Job(long arrivalTime, int procsNeeded, long actualRunningTime,
         long estRunningTime, std::string ID, CommInfo inComm, PhaseInfo inPhase) : commInfo(inComm) , phaseInfo(inPhase)
{
    initialize(arrivalTime, procsNeeded, actualRunningTime, estRunningTime);
    this -> ID = ID;
}

//copy constructor
Job::Job(const Job &job) : commInfo(job.commInfo)
{
    arrivalTime = job.arrivalTime;
    procsNeeded = job.procsNeeded;
    actualRunningTime = job.actualRunningTime;
    estRunningTime = job.estRunningTime;
    jobNum = job.jobNum;
    ID = job.ID;
    startTime = job.startTime;
    hasRun = job.hasRun;
    started = job.started;  
    if(job.taskCommInfo == NULL){
        taskCommInfo = NULL;
    } else {
        taskCommInfo = new TaskCommInfo(*(job.taskCommInfo));
    }
}

Job::~Job()
{
    if(taskCommInfo != NULL){
        delete taskCommInfo;
    }
}

//Helper for constructors
void Job::initialize(unsigned long arrivalTime, int procsNeeded,
                     unsigned long actualRunningTime, unsigned long estRunningTime) 
{
    //make sure estimate is valid; workload log uses -1 for "no estimate"
    if (estRunningTime < actualRunningTime || (unsigned long)-1 == estRunningTime) {
        estRunningTime = actualRunningTime;
    }

    this -> arrivalTime = arrivalTime;
    this -> procsNeeded = procsNeeded;
    this -> actualRunningTime = actualRunningTime;
    this -> estRunningTime = estRunningTime;

    startTime = -1;

    jobNum = nextJobNum;
    nextJobNum++;
    hasRun = false;
    started = false;
    taskCommInfo = NULL;
}

std::string Job::toString()
{
    char retVal[100];
    snprintf(retVal, 100, "Job #%ld (%ld, %d, %ld, %ld, null)", jobNum,
             arrivalTime, procsNeeded, actualRunningTime, estRunningTime);
    return retVal;
}

//starts a job
void Job::start(unsigned long time) 
{
    if ((unsigned long)-1 != startTime) {
        schedout.fatal(CALL_INFO, 1, "attempt to start an already-running job: %s\n", toString().c_str());
    }
    started = true; 
    startTime = time;
}

void Job::reset()
{
    startTime = -1;
    hasRun = false;
    started = false;
}

void Job::startsAtTime(unsigned long time)
{
    startTime = time;
    hasRun = true; 
    started = true; 
}

