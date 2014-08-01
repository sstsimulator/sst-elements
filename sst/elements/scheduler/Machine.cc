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

#include "AllocInfo.h"
#include "Job.h"
#include "output.h"

using namespace SST::Scheduler;

Machine::Machine(int inNumNodes, int numCoresPerNode, double** D_matrix) : numNodes(inNumNodes), coresPerNode(numCoresPerNode)
{
    this->D_matrix = D_matrix;
    freeNodes = std::vector<bool>(numNodes);
    reset();
}

Machine::~Machine()
{
    if(D_matrix != NULL){
        for (int i = 0; i < numNodes; i++)
            delete[] D_matrix[i];
        delete[] D_matrix;
    }
}

void Machine::reset()
{
    numAvail = numNodes;
    for(int i = 0; i < numNodes; i++){
        freeNodes[i] = true;
    }
}

void Machine::allocate(AllocInfo* allocInfo)
{
    int nodeCount = allocInfo->getNodesNeeded();
    
    if(numAvail < nodeCount){
        schedout.fatal(CALL_INFO, 1, "Attempted to allocate %d nodes when only %d are available", nodeCount, numAvail);
    } else {
        numAvail -= nodeCount;
    }
    
    for(int i = 0; i < nodeCount; i++) {
        if(!freeNodes[allocInfo -> nodeIndices[i]]){
            schedout.fatal(CALL_INFO, 0, "Attempted to allocate job %ld to a busy node: ", allocInfo->job->getJobNum() );
        }
        freeNodes[allocInfo -> nodeIndices[i]] = false;
    }
}

void Machine::deallocate(AllocInfo* allocInfo)
{
    int nodeCount = allocInfo->getNodesNeeded();
    
    if(nodeCount > (numNodes - numAvail)) {
        schedout.fatal(CALL_INFO, 1, "Attempted to deallocate %d nodes when only %d are busy", nodeCount, (numNodes-numAvail));
    } else {
        numAvail += nodeCount;
    }
    
    for(int i = 0; i < nodeCount; i++) {
        if(freeNodes[allocInfo -> nodeIndices[i]]){
            schedout.fatal(CALL_INFO, 0, "Attempted to deallocate job %ld from an idle node: ", allocInfo->job->getJobNum() );
        }
        freeNodes[allocInfo -> nodeIndices[i]] = true;
    }
}

std::vector<int>* Machine::getFreeNodes() const
{
    std::vector<int>* freeList = new std::vector<int>();
    for(int i = 0; i < numNodes; i++){
        if(freeNodes[i]){
            freeList->push_back(i);
        }
    }
    return freeList;
}

std::vector<int>* Machine::getUsedNodes() const
{
    std::vector<int>* usedNodes = new std::vector<int>();
    for(int i = 0; i < numNodes; i++){
        if(!freeNodes[i]){
            usedNodes->push_back(i);
        }
    }
    return usedNodes;
}

double Machine::getCoolingPower() const
{
    int Putil=2000;
    int Pidle=1000;

    double Tred=30;  

    int busynodes = 0;
    double max_inlet = 0;
    double sum_inlet = 0;

    //max inlet temp and number of busy nodes
    for (int i = 0; i < numNodes; i++) {
        if( !freeNodes[i] ){
            busynodes++;
        }
        if(D_matrix != NULL){
            sum_inlet = 0;
            for (int j = 0; j < numNodes; j++)
            {
                sum_inlet += D_matrix[i][j] * (Pidle + Putil * (!freeNodes[i]));
            }
            if(sum_inlet > max_inlet){
                max_inlet = sum_inlet;
            }
        }
    }

    // Total power of data center
    double Pcompute = busynodes * Putil + numNodes * Pidle;

    // Supply temperature
    double Tsup;
    if(D_matrix != NULL){
        Tsup = Tred - max_inlet;
    } else {
        Tsup = Tred;
    }

    // Coefficient of performance
    double COP = 0.0068 * Tsup * Tsup + 0.0008 * Tsup + 0.458;

    // Cooling power in kW
    double Pcooling = 0.001 * Pcompute * (1 / COP);

    return  Pcooling;
}

