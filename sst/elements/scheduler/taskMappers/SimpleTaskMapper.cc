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

#include "SimpleTaskMapper.h"

#include "AllocInfo.h"
#include "Job.h"
#include "TaskMapInfo.h"

using namespace SST::Scheduler;

std::string SimpleTaskMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    return com + "Simple Task Mapper";
}

TaskMapInfo* SimpleTaskMapper::mapTasks(AllocInfo* allocInfo)
{
    TaskMapInfo* tmi = new TaskMapInfo(allocInfo, *machine);
    int jobSize = allocInfo->job->getProcsNeeded();
    int coresPerNode = machine->getNumCoresPerNode();
    for(int i = 0; i < jobSize; i++){
        tmi->insert(i, allocInfo->nodeIndices[i/coresPerNode]);
    }
    return tmi;
}
