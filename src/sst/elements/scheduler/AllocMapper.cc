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

#include "AllocMapper.h"

#include "AllocInfo.h"
#include "Job.h"
#include "TaskMapInfo.h"

#include "output.h"

using namespace std;
using namespace SST::Scheduler;

//set aside memory for mappings
std::map<long int, std::vector<int>*> AllocMapper::mappings = std::map<long int, std::vector<int>*>();

AllocInfo* AllocMapper::allocate(Job* job)
{
    if (!canAllocate(*job)){
        return NULL;
    }
    AllocInfo *ai = new AllocInfo(job, mach);
    int nodesNeeded = ai->getNodesNeeded();
    int jobSize = job->getProcsNeeded();

    //create mapping data
    vector<int>taskToNode(jobSize, -1);
    vector<long int>usedNodes(nodesNeeded, -1);

    //get free node info
    isFree = mach.freeNodeList();

    allocMap(*ai, usedNodes, taskToNode);

    //fill the allocInfo
    for(int i = 0; i < nodesNeeded; i++){
        ai->nodeIndices[i] = usedNodes[i];
    }

    //store mapping if required
    if(allocateAndMap){
        std::vector<int> *mapping = new std::vector<int>(taskToNode);
        AllocMapper::mappings[job->getJobNum()] = mapping;
    }

    //clear memory
    delete isFree;

    return ai;
}

TaskMapInfo* AllocMapper::mapTasks(AllocInfo* allocInfo)
{
    long int jobNum = allocInfo->job->getJobNum();
    int nodesNeeded = allocInfo->getNodesNeeded();
    int jobSize = allocInfo->job->getProcsNeeded();
    vector<int> *taskToNode;

    //check if already mapped
    if(mappings.count(jobNum) == 0){ //if not,
        //map AND allocate
        //create mapping data
        vector<long int>usedNodes(nodesNeeded, -1);
        taskToNode = new vector<int>(jobSize, -1);
        //create virtual free nodes
        isFree = new vector<bool>(mach.numNodes, false);
        for(int i = 0; i < nodesNeeded; i++){
            isFree->at(allocInfo->nodeIndices[i]) = true;
        }
        //allocate
        allocMap(*allocInfo, usedNodes, *taskToNode);
        delete isFree;
    } else {
        taskToNode = mappings[jobNum];
        mappings.erase(jobNum);
    }

    TaskMapInfo* tmi = new TaskMapInfo(allocInfo, mach);
    for(long int taskIt = 0; taskIt < jobSize; taskIt++){
        tmi->insert(taskIt, taskToNode->at(taskIt));
    }

    delete taskToNode;
    return tmi;
}

