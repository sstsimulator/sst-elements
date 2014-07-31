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
#include "SimpleAllocator.h"

#include "sst/core/serialization.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "SimpleMachine.h"

using namespace SST::Scheduler;

SimpleAllocator::SimpleAllocator(SimpleMachine* m) 
{
    machine = m;
}

std::string SimpleAllocator::getSetupInfo(bool comment) 
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
        AllocInfo* ai = new AllocInfo(j, *machine);
        std::vector<int>* freeNodes = machine->getFreeNodes();
        for(int i = 0; i < j->getProcsNeeded(); i++) {
            ai->nodeIndices[i] = freeNodes->at(i);
        }
        delete freeNodes;
        return ai;
    }
    return NULL;
}
