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

#include "Machine.h"
#include "output.h"

using namespace SST::Scheduler;

//takes number of nodes
SimpleMachine::SimpleMachine(int numNodes, bool insimulationmachine, int numCoresPerNode, double** D_matrix)
                             : Machine(numNodes, numCoresPerNode, D_matrix) 
{  
    schedout.init("", 8, ~0, Output::STDOUT);
    simulationmachine = insimulationmachine;
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
    sprintf(mesg, "SimpleMachine with %ld nodes, %d cores per node", numNodes, coresPerNode);
    return com + mesg;
}

long SimpleMachine::getNodeDistance(int node1, int node2) const
{
    schedout.fatal(CALL_INFO, 1, "Attempt to read node distance from Simple Machine");
    return -1;
}

