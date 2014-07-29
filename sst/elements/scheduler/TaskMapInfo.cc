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
#include "MeshMachine.h"
#include "output.h"
#include "TaskCommInfo.h"

using namespace SST::Scheduler;

TaskMapInfo::TaskMapInfo(AllocInfo* ai)
{
    allocInfo = ai;
    job = ai->job;
    taskCommInfo = job->taskCommInfo;
    size = job->getProcsNeeded();
    taskToNode = new int[size];
    mappedCount = 0;
    avgHopDist = 0;
}

TaskMapInfo::~TaskMapInfo()
{
    delete allocInfo;
    delete [] taskToNode;
}

void TaskMapInfo::insert(int taskInd, int nodeInd)
{
    taskToNode[taskInd] = nodeInd;
    mappedCount++;
}

double TaskMapInfo::getAvgHopDist(const MeshMachine & machine)
{
    if(avgHopDist == 0) {
        //check if every task is mapped
        if(size > mappedCount){
            schedout.fatal(CALL_INFO, 1, "Task mapping info requested before all tasks are mapped.");
        }

        unsigned long totalHopDist = 0;
        int neighborCount = 0;

        //iterate through all tasks
        for(int taskIter = 0; taskIter < size; taskIter++){
            std::vector<std::vector<int> >* commVec = taskCommInfo->getCommOfTask(taskIter);
            //iterate through communicating tasks and add distance for communication
            for(unsigned int otherTaskIter = 0; otherTaskIter < commVec->at(0).size(); otherTaskIter++){
                totalHopDist += machine.getNodeDistance(taskToNode[taskIter], taskToNode[commVec->at(0)[otherTaskIter]])
                                * commVec->at(1)[otherTaskIter]; // multiply hop distance with communication weight
                neighborCount++;
            }
            delete commVec;
        }

        //distance per neighbor
        //two-way distances and uncounted neighbors cancel each other
        avgHopDist = totalHopDist;
        if(neighborCount != 0){
            avgHopDist /= neighborCount;
        }
    }

    return avgHopDist;
}

