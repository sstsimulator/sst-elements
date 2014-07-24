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

TaskMapInfo::TaskMapInfo(AllocInfo* ai, const Machine & mach) : machine(mach)
{
    allocInfo = ai;
    job = ai->job;
    taskCommInfo = job->taskCommInfo;
    size = job->getProcsNeeded();
    taskToNode = new int[size];
    nodes = new int[mach.getNumNodes()];
    for(int i = 0; i < mach.getNumNodes(); i++){
        nodes[i] = mach.getNumCoresPerNode();
    }
    mappedCount = 0;
    avgHopDist = 0;
}

TaskMapInfo::~TaskMapInfo()
{
    delete allocInfo;
    delete [] taskToNode;
    delete [] nodes;
}

void TaskMapInfo::insert(int taskInd, int nodeInd)
{
    if(nodes[nodeInd] == 0){
        schedout.fatal(CALL_INFO, 1, "Tried to map task %d of Job %ld to node %d with no available cores.", taskInd, job->getJobNum(), nodeInd);
    } else {
        nodes[nodeInd]--;
    }        
    taskToNode[taskInd] = nodeInd;
    mappedCount++;
}

//Current version is not weighted and only checks if there is communication
double TaskMapInfo::getAvgHopDist()
{
    if(avgHopDist == 0) {
        //check if every task is mapped
        if(size > mappedCount){
            schedout.fatal(CALL_INFO, 1, "Task mapping info requested before all tasks are mapped.");
        }

        //Optimize for speed or memory - threshold depends on the available memory
        // speed optimization uses jobSize^2 * sizeof(int) memory
        bool optimizeForSpeed = (size < 10000) ;

        int** commMatrix;
        unsigned long totalHopDist = 0;
        int neighborCount = 0;

        if(optimizeForSpeed) {
            commMatrix = taskCommInfo->getCommMatrix();
        }

        //iterate through all tasks
        bool addDistance;
        for(int taskIter = 0; taskIter < size; taskIter++){
            //iterate through other tasks and add distance for communication
            for(int otherTaskIter = taskIter + 1 ; otherTaskIter < size; otherTaskIter++){
                if(optimizeForSpeed) {
                    addDistance = commMatrix[taskIter][otherTaskIter] != 0
                                  || commMatrix[otherTaskIter][taskIter] != 0;
                } else {
                    addDistance = (taskCommInfo->getCommWeight(taskIter, otherTaskIter) != 0);
                }
                if(addDistance){
                    totalHopDist += machine.getNodeDistance(taskToNode[taskIter], taskToNode[otherTaskIter]);
                    neighborCount++;
                } 
            }
        }

        if(optimizeForSpeed) {
            //delete comm matrix
            for(int i = 0 ; i < size; i++){
                delete [] commMatrix[i];
            }
            delete [] commMatrix;
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

