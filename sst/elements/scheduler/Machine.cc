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

#include "Machine.h"

using namespace SST::Scheduler;

Machine::Machine(int numNodes, int numCoresPerNode, double** D_matrix)
{
    this->D_matrix = D_matrix;
    this->numNodes = numNodes;
    this->coresPerNode = numCoresPerNode;

    isFree = std::vector<bool>(numNodes);
    //reset();
}

Machine::~Machine()
{
    if(D_matrix != NULL){
        for (int i = 0; i < numNodes; i++)
            delete[] D_matrix[i];
        delete[] D_matrix;
    }
}

std::vector<int>* Machine::getFreeNodes() const
{
    std::vector<int>* freeNodes = new std::vector<int>();
    for(int i = 0; i < numNodes; i++){
        if(isFree[i]){
            freeNodes->push_back(i);
        }
    }
    return freeNodes;
}

