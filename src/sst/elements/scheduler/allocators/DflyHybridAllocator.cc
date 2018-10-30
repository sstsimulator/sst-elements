// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// This is the Level - Spread policy for dragonfly systems which spreads
// jobs within the smallest network level that a given job can fit in at the time
// of its allocation. If a job fits within the available nodes that are connected
// to a single router, it selects the router with the largest number of idle nodes
// and allocates the job there. If a job cannot fit within a single router but fits
// within the available nodes in a single group, it selects the most idle group
// and allocates the job there. To further reduce load imbalance on local links in
// this group, it selects nodes connected to different routers in a round - robin
// manner. If a job cannot fit within a single group, it spreads the job throughout
// the entire network, where it selects nodes in different groups in a round - robin
// manner.
// This policy is published in:
// Yijia Zhang, Ozan Tuncer, Fulya Kaplan, Katzalin Olcoz, Vitus J Leung, 
// Ayse K Coskun. Level-Spread: A New Job Allocation Policy for Dragonfly Networks. 
// IEEE International Parallel & Distributed Processing Symposium (IPDPS), 2018.

#include "sst_config.h"

#include "DflyHybridAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflyHybridAllocator::DflyHybridAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{

}

std::string DflyHybridAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Hybrid Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflyHybridAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        std::cout << "jobSize=" << jobSize << ", ";
        if (jobSize <= dMach.nodesPerRouter) {
            //find the router with the most free nodes.
            int BestRouter = -1;
            int BestRouterFreeNodes = 0;
            for (int routerID = 0; routerID < dMach.numRouters; routerID++) {
                int thisRouterFreeNode = 0;
                for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                    int nodeID = routerID * dMach.nodesPerRouter + localNodeID;
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        //caution: isFree() will update only after one job is fully allocated.
                        ++thisRouterFreeNode;
                    }
                }
                if (thisRouterFreeNode > BestRouterFreeNodes) {
                    BestRouter = routerID;
                    BestRouterFreeNodes = thisRouterFreeNode;
                }
            }
            //if job can fit in this router, then 
            //we allocate the job to this router simply.
            if (jobSize <= BestRouterFreeNodes) {
                int nodeID = BestRouter * dMach.nodesPerRouter;
                int i = 0;
                while (i < jobSize) {
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        ai->nodeIndices[i] = nodeID;
                        occupiedNodes.insert(nodeID);
                        std::cout << nodeID << " ";
                        ++i;
                        ++nodeID;
                    }
                    else {
                        ++nodeID;
                    }
                }
                std::cout << ",inRouter";
                std::cout << endl;
                return ai;
            }
        }
        int nodesPerGroup = dMach.routersPerGroup * dMach.nodesPerRouter;
        if (jobSize <= nodesPerGroup) {
            //find the group with the most free nodes.
            int BestGroup = -1;
            int BestGroupFreeNodes = 0;
            for (int GroupID = 0; GroupID < dMach.numGroups; GroupID++) {
                int thisGroupFreeNode = 0;
                for (int localNodeID = 0; localNodeID < nodesPerGroup; localNodeID++) {
                    int nodeID = GroupID * nodesPerGroup + localNodeID;
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        ++thisGroupFreeNode;
                    }
                }
                if (thisGroupFreeNode > BestGroupFreeNodes) {
                    BestGroup = GroupID;
                    BestGroupFreeNodes = thisGroupFreeNode;
                }
            }
            //if job can fit in this group, then 
            //we allocate the job to this group spreadingly.
            if (jobSize <= BestGroupFreeNodes) {
                int routerID = BestGroup * dMach.routersPerGroup;
                for (int i = 0; i < jobSize; i++) {
                    int localNodeID = 0;
                    while (true) {
                        int nodeID = routerID * dMach.nodesPerRouter + localNodeID;
                        if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                            ai->nodeIndices[i] = nodeID;
                            occupiedNodes.insert(nodeID);
                            std::cout << nodeID << " ";
                            //change router.
                            if (routerID < (BestGroup + 1) * dMach.routersPerGroup - 1) {
                                ++routerID;
                            }
                            else {
                                routerID = BestGroup * dMach.routersPerGroup;
                            }
                            break;
                        }
                        else {
                            //move to next node.
                            if (localNodeID < dMach.nodesPerRouter - 1) {
                                ++localNodeID;
                                continue;
                            }
                            else {
                                //change router.
                                if (routerID < (BestGroup + 1) * dMach.routersPerGroup - 1) {
                                    ++routerID;
                                }
                                else {
                                    routerID = BestGroup * dMach.routersPerGroup;
                                }
                                localNodeID = 0;
                                continue;
                            }
                        }
                    }
                }
                std::cout << ",inGroup";
                std::cout << endl;
                return ai;
            }
        }
        //job cannot fit in one group, so
        //it will simply spread across the machine.
        int groupID = 0;
        for (int i = 0; i < jobSize; i++) {
            int localNodeID = 0;
            while (true) {
                int nodeID = groupID * nodesPerGroup + localNodeID;
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    occupiedNodes.insert(nodeID);
                    std::cout << nodeID << " ";
                    //change group.
                    if (groupID < dMach.numGroups - 1) {
                        ++groupID;
                    }
                    else {
                        groupID = 0;
                    }
                    break;
                }
                else {
                    if (localNodeID < nodesPerGroup - 1) {
                        ++localNodeID;
                        continue;
                    }
                    else {
                        //change group.
                        if (groupID < dMach.numGroups - 1) {
                            ++groupID;
                        }
                        else {
                            groupID = 0;
                        }
                        localNodeID = 0;
                        continue;
                    }
                }
            }
        }
        std::cout << ",spread";
        std::cout << endl;
        return ai;
    }
    return NULL;
}

