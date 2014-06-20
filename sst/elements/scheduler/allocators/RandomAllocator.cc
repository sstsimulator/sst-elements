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

#include "sst_config.h"
#include "RandomAllocator.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "misc.h"
#include "output.h"

using namespace SST::Scheduler;


RandomAllocator::RandomAllocator(Machine* mesh) 
{
    schedout.init("", 8, ~0, Output::STDOUT);
    machine = dynamic_cast<MeshMachine*>(mesh);
    if (machine == NULL) {
        schedout.fatal(CALL_INFO, 1, "Random Allocator requires Mesh");
    }
    srand(0);
}

/*
   RandomAllocator RandomAllocator::Make(std::vector<std::string*>* params){
   Factory.argsAtLeast(0,params);
   Factory.argsAtMost(0,params);

   MeshMachine* m = (MeshMachine) Main->getMachine();
   if(m == NULL)
   error("Random allocator requires a Mesh machine");
   return new RandomAllocator(m);
   return null;

   }
   */

std::string RandomAllocator::getParamHelp()
{
    return "";
}

std::string RandomAllocator::getSetupInfo(bool comment)
{
    std::string com;
    if(comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    return com + "Random Allocator";
}

//allocates job if possible
//returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation)
AllocInfo* RandomAllocator::allocate(Job* job){
    if(!canAllocate(job)) return NULL;

    MeshAllocInfo* retVal = new MeshAllocInfo(job);

    //figure out which processors to use
    int numProcs = job -> getProcsNeeded();
    std::vector<MeshLocation*>* available = ((MeshMachine*)machine) -> freeProcessors();
    for (int i = 0; i < numProcs; i++) {
        int num = rand() % available -> size();
        (*retVal -> processors)[i] = (*available)[num];
        retVal -> nodeIndices[i] = (*available)[num] -> toInt((MeshMachine*)machine);
        available -> erase(available -> begin() + num);
    }
    for (int i = 0; i < (int)available -> size(); i++) {
        delete available -> at(i);
    }
    available -> clear();
    delete available;
    return retVal;
}
