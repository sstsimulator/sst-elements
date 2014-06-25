// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
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
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"

using namespace SST::Scheduler;

EnergyAllocator::EnergyAllocator(std::vector<std::string>* params, MeshMachine* mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    configName = "Energy";
    machine = mach;
}


std::string EnergyAllocator::getParamHelp()
{
    return "This allocator requires d_matrix input.";
}

std::string EnergyAllocator::getSetupInfo(bool comment)
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
    return allocate(job,((MeshMachine*)machine) -> freeProcessors());
}

//Allocates job if possible.
//Returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation).
AllocInfo* EnergyAllocator::allocate(Job* job, std::vector<MeshLocation*>* available) 
{
    MeshMachine* mMachine = static_cast<MeshMachine*>(machine);

    if (!canAllocate(*job, available)) {
        return NULL;
    }

    MeshAllocInfo* retVal = new MeshAllocInfo(job);

    int numProcs = job -> getProcsNeeded();
    
    //optimization: if exactly enough procs are free, just return them
    if ((unsigned int) numProcs == available -> size()) {
        for (int i = 0; i < numProcs; i++) {
            (*retVal -> processors)[i] = (*available)[i];
            retVal -> nodeIndices[i] = (*available)[i] -> toInt(*mMachine);
        }
        delete available;
        return retVal;
    }

    std::vector<MeshLocation*>* ret = EnergyHelpers::getEnergyNodes(available, job -> getProcsNeeded(), *mMachine);
    for (int i = 0; i < numProcs; i++) {
        (*retVal->processors)[i] = ret->at(i);
        retVal->nodeIndices[i] = ret->at(i)->toInt(*mMachine);
        delete (*available)[i];
    }
    delete available;
    return retVal;
} 
