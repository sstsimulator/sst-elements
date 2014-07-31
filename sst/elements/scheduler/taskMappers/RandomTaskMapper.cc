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

#include "RandomTaskMapper.h"

#include <vector>
#include <cmath>

#include "AllocInfo.h"
#include "Job.h"
#include "TaskMapInfo.h"

using namespace SST::Scheduler;

RandomTaskMapper::RandomTaskMapper(const Machine & mach) : TaskMapper(mach)
{
}

RandomTaskMapper::~RandomTaskMapper()
{
}

std::string RandomTaskMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    return com + "Random Task Mapper";
}

TaskMapInfo* RandomTaskMapper::mapTasks(AllocInfo* allocInfo)
{
    TaskMapInfo* tmi = new TaskMapInfo(allocInfo);
    int jobSize = allocInfo->job->getProcsNeeded();
    
    std::vector<int> available = std::vector<int>();
    std::vector<int> availableCores = std::vector<int>();

    for(int i = 0; i < allocInfo->getNodesNeeded(); i++){
        availableCores.push_back(mach.coresPerNode);
        available.push_back(allocInfo->nodeIndices[i]);
    }
    
    for(int i = 0; i < jobSize; i++){
        int num = rand() % available.size();
        tmi->insert(i, available.at(num));
        availableCores.at(num) = availableCores.at(num) - 1;
        if(availableCores.at(num) == 0){
            available.erase(available.begin() + num );
            availableCores.erase(availableCores.begin() + num);
        }
    }
    
    return tmi;
}

