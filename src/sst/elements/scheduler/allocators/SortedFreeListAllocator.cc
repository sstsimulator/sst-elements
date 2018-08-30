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

/**
 * Allocator that assigns the first available processors (according to
 * order specified when allocator is created).
 */
#include "sst_config.h"
#include "SortedFreeListAllocator.h"

#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "LinearAllocator.h"
#include "Machine.h"
#include "StencilMachine.h"
#include "output.h"

using namespace SST::Scheduler;

SortedFreeListAllocator::SortedFreeListAllocator(std::vector<std::string>* params, Machine* mach) : LinearAllocator(params, mach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
}

std::string SortedFreeListAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    return com + "Linear Allocator (Sorted Free List)";
}

//allocates j if possible
//returns information on the allocation or NULL if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation)
AllocInfo* SortedFreeListAllocator::allocate(Job* job) 
{
    schedout.debug(CALL_INFO, 7, 0, "Allocating %s \n", job -> toString().c_str());

    if (!canAllocate(*job)) {
        return NULL;
    }

    std::vector<int>* freeNodes = machine.getFreeNodes();
    std::vector<MeshLocation*>* freeprocs = new std::vector<MeshLocation*>(freeNodes->size());
    for(unsigned int i = 0; i < freeNodes->size(); i++){
        freeprocs->at(i) = new MeshLocation(freeNodes->at(i), *mMachine);
    }   
    delete freeNodes;

    stable_sort(freeprocs -> begin(), freeprocs -> end(), *ordering);

    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

    AllocInfo* retVal = new AllocInfo(job, machine);
    for (int i = 0; i < (int)freeprocs -> size(); i++) {
        if (i < nodesNeeded) {
            retVal -> nodeIndices[i] = freeprocs -> at(i) -> toInt(*mMachine);
        }
        delete freeprocs -> at(i);
    }
    delete freeprocs;
    return retVal;
}
