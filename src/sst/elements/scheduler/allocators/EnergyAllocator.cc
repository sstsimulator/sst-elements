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

/*class to implement an energy-efficient allocation algorithm
 * that uses the glpk LP solver to find the most efficient allocation.
 * A hybrid allocator is also available that takes both
 * energy and performance into account; this is found in
 * NearestAllocator.cc
*/

#include "sst_config.h"
#include "EnergyAllocator.h"

#include <sstream>
#include <limits>
#include <vector>
#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "AllocInfo.h"
#include "EnergyAllocClasses.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"

using namespace SST::Scheduler;

EnergyAllocator::EnergyAllocator(std::vector<std::string>* params, const Machine & mach) : Allocator(mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    configName = "Energy";
}


std::string EnergyAllocator::getParamHelp()
{
    return "This allocator requires d_matrix input.";
}

std::string EnergyAllocator::getSetupInfo(bool comment) const
{ 
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    std::stringstream ret;
    ret <<com<<"Energy Allocator ("<<configName<<")";
    return ret.str();
}

AllocInfo* EnergyAllocator::allocate(Job* job)
{
    std::vector<int>* available = machine.getFreeNodes();    
    return allocate(job, available);
}

//Allocates job if possible.
//Returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation).
AllocInfo* EnergyAllocator::allocate(Job* job, std::vector<int>* available) 
{
    if (!canAllocate(*job)) {
        return NULL;
    }

    AllocInfo* retVal = new AllocInfo(job, machine);

    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);
    
    //optimization: if exactly enough procs are free, just return them
    if ((unsigned int) nodesNeeded == available -> size()) {
        for (int i = 0; i < nodesNeeded; i++) {
            retVal -> nodeIndices[i] = available->at(i);
        }
        delete available;
        return retVal;
    }

    std::vector<int>* ret = EnergyHelpers::getEnergyNodes(available, nodesNeeded, machine);
    for (int i = 0; i < nodesNeeded; i++) {
        retVal->nodeIndices[i] = ret->at(i);
    }
    delete available;
    return retVal;
} 
