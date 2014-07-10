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

#include "AllocInfo.h"
#include "Job.h"
#include "TaskMapInfo.h"

using namespace SST::Scheduler;

RandomTaskMapper::RandomTaskMapper(Machine* mach) : TaskMapper(mach)
{
    rng = new SST::RNG::MarsagliaRNG();
}

RandomTaskMapper::~RandomTaskMapper()
{
    delete rng;
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
    
    for(int i = 0; i <= jobSize; i++){
        available.push_back(allocInfo->nodeIndices[i]);
        availableCores.push_back(machine->getNumCoresPerNode());
    }
    
    for(int i = jobSize; i > 0; i--){
        int num = rng->generateNextUInt32() % available.size();
        tmi->insert((i - 1), available.at(num));
        availableCores.at(num) = availableCores.at(num) - 1;
        if(availableCores.at(num) == 0)
            available.erase( available.begin() + num );
    }
    
    return tmi;
}

