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
#include "AllocInfo.h"

#include <string>
#include <cmath>

#include "Job.h"
#include "Machine.h"

using namespace SST::Scheduler;

AllocInfo::AllocInfo(Job* job, const Machine & mach)
{
    this -> job = job;
    nodesNeeded = ceil((float) job->getProcsNeeded() / mach.coresPerNode);
    nodeIndices = new int[nodesNeeded];
    nodeIndices[0] = -1; // ConstraintAllocator puts allocation here
}

AllocInfo::AllocInfo(const AllocInfo & ai)
{
    job = ai.job;
    nodesNeeded = ai.nodesNeeded;
    nodeIndices = new int[nodesNeeded];
    for(int i = 0; i < nodesNeeded; i++){
        nodeIndices[i] = ai.nodeIndices[i];
    }
}

AllocInfo::~AllocInfo() 
{
    delete [] nodeIndices;
}

std::string AllocInfo::getProcList() 
{
    return "";
}


