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
#include "RandomAllocator.h"

#include "sst/core/rng/mersenne.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"

using namespace SST::Scheduler;

RandomAllocator::RandomAllocator(Machine* mesh) : Allocator(*mesh)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    rng = new SST::RNG::MersenneRNG();
}

RandomAllocator::~RandomAllocator()
{
    delete rng;
}

std::string RandomAllocator::getParamHelp()
{
    return "";
}

std::string RandomAllocator::getSetupInfo(bool comment) const
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
    if(!canAllocate(*job)) return NULL;

    AllocInfo* retVal = new AllocInfo(job, machine);

    //figure out which processors to use
    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);
    
    std::vector<int>* freeNodes = machine.getFreeNodes();    
    for (int i = 0; i < nodesNeeded; i++) {
        int num = rng->generateNextInt32() % freeNodes -> size();
        retVal->nodeIndices[i] = freeNodes->at(num);
        freeNodes->erase(freeNodes->begin() + num);
    }
    delete freeNodes;
    return retVal;
}
