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
#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "misc.h"
#include "output.h"
//#include "NearestAllocClasses.h"

using namespace SST::Scheduler;
//using namespace std;

EnergyAllocator::EnergyAllocator(std::vector<std::string>* params, Machine* mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    MachineMesh* m = (MachineMesh*) mach;
    if (NULL == m) {
        //error("Nearest allocators require a Mesh machine");
        schedout.fatal(CALL_INFO, 1, "Energy allocator requires a Mesh machine");
    }

    configName = "Energy";
    machine = m;
}


std::string EnergyAllocator::getParamHelp()
{
    return "This allocator is currently only supported with a 4x5x2 mesh.\nThe energy constants must be changed in EnergyAllocator.cc";
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
    ret <<com<<"Energy Allocator ("<<configName<<")\n";
    return ret.str();
}

AllocInfo* EnergyAllocator::allocate(Job* job)
{
    return allocate(job,((MachineMesh*)machine) -> freeProcessors());
}

//Allocates job if possible.
//Returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation).
AllocInfo* EnergyAllocator::allocate(Job* job, std::vector<MeshLocation*>* available) 
{

    if (!canAllocate(job, available)) {
        return NULL;
    }

    MeshAllocInfo* retVal = new MeshAllocInfo(job);

    int numProcs = job -> getProcsNeeded();

    //optimization: if exactly enough procs are free, just return them
    if ((unsigned int) numProcs == available -> size()) {
        for (int i = 0; i < numProcs; i++) {
            (*retVal -> processors)[i] = (*available)[i];
            retVal -> nodeIndices[i] = (*available)[i] -> toInt((MachineMesh*)machine);
        }
        //printf("short return\n");
        return retVal;
    }

    std::vector<MeshLocation*>* ret = EnergyHelpers::getEnergyNodes(available, job -> getProcsNeeded(), (MachineMesh*)machine);
    for (int i = 0; i < numProcs; i++) {
        (*retVal -> processors)[i] = ret -> at(i);
        retVal -> nodeIndices[i] = ret -> at(i) -> toInt((MachineMesh*)machine);
    }
    return retVal;

    /*
    //score of best value found so far with it tie-break score:
    std::pair<long,long>* bestVal = new std::pair<long,long>(LONG_MAX,LONG_MAX);

    bool recordingTies = false; //Statistics.recordingTies();

    //stores allocations w/ best score (no tiebreaking) if ties being recorded:
    //(actual best value w/ tiebreaking stored in retVal.processors)
    std::vector<std::vector<MeshLocation*>*>* bestAllocs = NULL;
    if (recordingTies) {
    bestAllocs = new std::vector<std::vector<MeshLocation*> *>(); 
    }

    std::vector<MeshLocation*>* possCenters;

    possCenters = new std::vector<MeshLocation*>();
    // call LP to get centers 
    int* oldx = new int[40];
    int* newx = new int[40];
    for(int x= 0; x < 40; x++) {
    oldx[x] = 1;
    newx[x] = 0;
    }
    for(unsigned int x = 0; x < available -> size(); x++) { 
    oldx[ssttolp[(*available)[x]->toInt((MachineMesh*)machine)]] = 0;
    }

    hybridalloc(oldx, newx, 40, numProcs);
    for(int x = 0; x < 40; x++) {
    if(newx[x] == 1 && oldx[x] == 0) {
    possCenters -> push_back(new MeshLocation(lptosst[x],(MachineMesh*)machine));
    }
    }

    for(unsigned int i = 0; i < possCenters->size(); i++) {
    (*(retVal -> processors))[i] = possCenters->at(i); 
    retVal -> nodeIndices[i] = possCenters->at(i)-> toInt((MachineMesh*) machine);
    return retVal;
    }
    */

} 
