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

#include <string> 

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



//allocates j if possible
//returns information on the allocation or NULL if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation)
AllocInfo* SimpleAllocator::allocate(Job* j) 
{  
    if (canAllocate(j)) {
        return new AllocInfo(j);
    }
    return NULL;
}
