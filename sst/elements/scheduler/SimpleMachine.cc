// Copyright 2011 Sandia Corporation. Under the terms                          
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
#include "SimpleMachine.h"

#include <vector>
#include <string>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "allocators/SimpleAllocator.h"

using namespace SST::Scheduler;

//takes number of nodes
SimpleMachine::SimpleMachine(int numNodes, bool insimulationmachine, int numCoresPerNode, double** D_matrix)
                             : Machine(numNodes, numCoresPerNode, D_matrix) 
{  
    schedout.init("", 8, ~0, Output::STDOUT);
    simulationmachine = insimulationmachine;
    reset();
}

//return to beginning-of-simulation state
void SimpleMachine::reset() 
{  
    numAvail = numNodes;
    for(int i = 0; i < numNodes; i++)
        isFree[i] = true;
}

std::string SimpleMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) {
        com="# ";
    } else {
        com="";
    }
    char mesg[100];
    sprintf(mesg, "SimpleMachine with %d nodes, %d cores per node", numNodes, coresPerNode);
    return com + mesg;
}

//Allocates nodes
void SimpleMachine::allocate(AllocInfo* allocInfo) 
{
    int num = allocInfo -> job -> getProcsNeeded();  //number of nodes

    if (allocInfo -> job -> hasStarted()) {
        schedout.debug(CALL_INFO, 7, 0, "%lu: allocate(%s) ending at %lu\n", allocInfo -> job -> getStartTime(), allocInfo -> job -> toString().c_str(), allocInfo -> job -> getStartTime() + allocInfo -> job -> getActualTime());
    } else {
        schedout.debug(CALL_INFO, 7, 0, "allocate(%s);\n", allocInfo -> job -> toString().c_str());
    }
    if(num > numAvail) {
        schedout.fatal(CALL_INFO, 1, "Attempt to allocate %d nodes when only %d are available", num, numAvail);
    }

    numAvail -= num;
    for(int i = 0; i < num; i++) {
        isFree[allocInfo -> nodeIndices[i]] = false;
    }
}

//Deallocates nodes
void SimpleMachine::deallocate(AllocInfo* allocInfo) 
{  
    int num = allocInfo ->job -> getProcsNeeded();  //number of nodes
    schedout.debug(CALL_INFO, 7, 0, "deallocate(%s); %d nodes free\n", allocInfo -> job ->toString().c_str(), numAvail + num);

    if(num > (numNodes - numAvail)) {
        schedout.fatal(CALL_INFO, 1, "Attempt to deallocate %d nodes when only %d are busy", num, (numNodes-numAvail));
    }

    numAvail += num;
    
    for(int i = 0; i < num; i++) {
        isFree[allocInfo -> nodeIndices[i]] = true;
    }
}

long SimpleMachine::getNodeDistance(int node1, int node2) const
{
    schedout.fatal(CALL_INFO, 1, "Attempt to read node distance from Simple Machine");
    return -1;
}

