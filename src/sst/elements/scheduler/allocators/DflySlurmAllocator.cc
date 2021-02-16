// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//To Do list of Yijia:
//check coding standards.
//combine steps for optimization.
//check the labeling of the nodes in dragonfly.
//find out the best allocation for entire machine case.

#include "sst_config.h"

#include "DflySlurmAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflySlurmAllocator::DflySlurmAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{

}

std::string DflySlurmAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Slurm Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflySlurmAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        //std::cout << "jobSize = " << jobSize << ", allocation, ";
        int BestRouter = -1;
        // possible to fit in one router.
        if (jobSize <= dMach.nodesPerRouter) {
            //find the router with the least free nodes and has enough vacancy for the job.
            int BestRouterFreeNodes = dMach.nodesPerRouter + 1;
            for (int routerID = 0; routerID < dMach.numRouters; routerID++) {
                // count the number of free nodes in this router.
                int thisRouterFreeNode = 0;
                for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                    int nodeID = routerID * dMach.nodesPerRouter + localNodeID;
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        //caution: isFree() will update only after one job is fully allocated.
                        ++thisRouterFreeNode;
                    }
                }
                // update best fit.
                if ( (thisRouterFreeNode >= jobSize) && (thisRouterFreeNode < BestRouterFreeNodes) ) {
                    BestRouter = routerID;
                    BestRouterFreeNodes = thisRouterFreeNode;
                }
            }
            // if there exists best fit, then allocate the job to this best fit.
            if (BestRouter != -1) {
                int nodeID = BestRouter * dMach.nodesPerRouter;
                int i = 0;
                while (i < jobSize) {
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        ai->nodeIndices[i] = nodeID;
                        occupiedNodes.insert(nodeID);
                        //std::cout << nodeID << " ";
                        ++i;
                        ++nodeID;
                    }
                    else {
                        ++nodeID;
                    }
                }
                //std::cout << endl;
                return ai;
            }
        }
        // if cannot fit into router, use best fit Round Robin Router.
        if (BestRouter == -1) {
            int i = 0;
            while (i < jobSize) {
                // first get the router with the least free nodes.
                int BestRouterFreeNodes = dMach.nodesPerRouter + 1;
                for (int routerID = 0; routerID < dMach.numRouters; routerID++) {
                    // count the number of free nodes in this router.
                    int thisRouterFreeNode = 0;
                    for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                        int nodeID = routerID * dMach.nodesPerRouter + localNodeID;
                        if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                            //caution: isFree() will update only after one job is fully allocated.
                            ++thisRouterFreeNode;
                        }
                    }
                    // update best fit, the router should contain at least one vacancy.
                    if ( (thisRouterFreeNode >= 1) && (thisRouterFreeNode < BestRouterFreeNodes) ) {
                        BestRouter = routerID;
                        BestRouterFreeNodes = thisRouterFreeNode;
                    }
                }
                // then allocate all the nodes in this router.
                for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                    int nodeID = BestRouter * dMach.nodesPerRouter + localNodeID;
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        ai->nodeIndices[i] = nodeID;
                        occupiedNodes.insert(nodeID);
                        //std::cout << nodeID << " ";
                        ++i;
                        if (i == jobSize) {
                            break;
                        }
                    }
                    else {
                        if (localNodeID < dMach.nodesPerRouter - 1) {
                            continue;
                        }
                        else {
                            //change router.
                            break;
                        }
                    }
                }
            }
            //std::cout << endl;
            return ai;
        }
    }
    return NULL;
}

