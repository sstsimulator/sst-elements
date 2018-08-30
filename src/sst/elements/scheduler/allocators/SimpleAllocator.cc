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
#include "SimpleAllocator.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"

using namespace SST::Scheduler;

SimpleAllocator::SimpleAllocator(Machine* m) : Allocator(*m)
{

}

std::string SimpleAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Simple Allocator";
}

AllocInfo* SimpleAllocator::allocate(Job* j) 
{  
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, machine);
        std::vector<int>* freeNodes = machine.getFreeNodes();
        for(int i = 0; i < ai->getNodesNeeded(); i++) {
            ai->nodeIndices[i] = freeNodes->at(i);
        }
        delete freeNodes;
        return ai;
    }
    return NULL;
}
