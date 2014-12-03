// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "TaskMapInfo.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "TaskCommInfo.h"

using namespace SST::Scheduler;

TaskMapInfo::TaskMapInfo(AllocInfo* ai, const Machine & inMach) : machine(inMach)
{
    allocInfo = ai;
    job = ai->job;
    taskCommInfo = job->taskCommInfo;
    size = job->getProcsNeeded();
    taskToNode = std::vector<long int>(size, -1);
    mappedCount = 0;
    avgHopDist = 0;
    numAvailCores = std::map<long int, int>();
}

TaskMapInfo::~TaskMapInfo()
{
    delete allocInfo;
}

void TaskMapInfo::insert(int taskInd, long int nodeInd)
{
    if(taskToNode[taskInd] != -1){
        schedout.fatal(CALL_INFO, 1, "Attempted to map task %d of job %ld multiple times.\n", taskInd, job->getJobNum());
    }
    if(numAvailCores.count(nodeInd) == 0){
        numAvailCores[nodeInd] = machine.coresPerNode - 1;
    } else {
        numAvailCores[nodeInd]--;
        if(numAvailCores[nodeInd] < 0){
            schedout.fatal(CALL_INFO, 1, "Task %d of job %ld is mapped to node %ld, where there is no core available.\n", taskInd, job->getJobNum(), nodeInd);
        }
    }
    taskToNode[taskInd] = nodeInd;
    mappedCount++;
}

double TaskMapInfo::getAvgHopDist()
{
    if(avgHopDist == 0) {
        //check if every task is mapped
        if(size > mappedCount){
            schedout.fatal(CALL_INFO, 1, "Task mapping info requested before all tasks are mapped.");
        }

        unsigned long totalHopDist = 0;
        int neighborCount = 0;    

        //iterate through all tasks
        std::vector<std::map<int,int> >* commInfo = taskCommInfo->getCommInfo();
        for(int taskIter = 0; taskIter < size; taskIter++){
            //iterate through communicating tasks and add distance for communication
            for(std::map<int, int>::iterator it = commInfo->at(taskIter).begin(); it != commInfo->at(taskIter).end(); it++){
                totalHopDist += machine.getNodeDistance(taskToNode[taskIter], taskToNode[it->first])
                                * it->second; // multiply hop distance with communication weight
                neighborCount++;
            }
        }
        delete commInfo;

        //distance per neighbor
        //two-way distances and uncounted neighbors cancel each other
        avgHopDist = totalHopDist;
        if(neighborCount != 0){
            avgHopDist /= neighborCount;
        }
    }

    return avgHopDist;
}

