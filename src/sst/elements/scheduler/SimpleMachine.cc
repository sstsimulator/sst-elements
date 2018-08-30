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
#include "SimpleMachine.h"

#include <string>
#include <stdio.h>

#include "output.h"

using namespace SST::Scheduler;

//takes number of nodes
SimpleMachine::SimpleMachine(int numNodes, bool insimulationmachine, int numCoresPerNode, double** D_matrix)
                             : Machine(numNodes, numCoresPerNode, D_matrix, 1) 
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
    sprintf(mesg, "SimpleMachine with %d nodes, %d cores per node", numNodes, coresPerNode);
    return com + mesg;
}

std::list<int>* SimpleMachine::getFreeAtDistance(int center, int distance) const
{
    std::list<int>* retList = new std::list<int>();
    retList->push_back(0);
    return retList;
}

int SimpleMachine::getNodeDistance(int node1, int node2) const
{
    return 1;
}

int SimpleMachine::nodesAtDistance(int dist) const
{
    return numNodes;
}

std::list<int>* SimpleMachine::getRoute(int node1, int node2, double commWeight) const
{
    std::list<int>* retVal = new std::list<int>;
    retVal->push_back(0);
    return retVal;
}
