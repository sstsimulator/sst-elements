// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "DflyJokanovicAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflyJokanovicAllocator::DflyJokanovicAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{

}

std::string DflyJokanovicAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Jokanovic Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflyJokanovicAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        //std::cout << "jobSize=" << jobSize << ", ";
        int nodesPerGroup = dMach.routersPerGroup * dMach.nodesPerRouter;
        if (jobSize <= nodesPerGroup) {
            //start from the leftmost group, find a group with enough idle nodes. If there isn't one, just put it simply from the leftmost place.
            bool finish = false;
            for (int GroupID = 0; GroupID < dMach.numGroups; GroupID++) {
                if (finish) {
                    break;
                }
                //check if enough idle nodes.
                int thisGroupFreeNode = 0;
                for (int localNodeID = 0; localNodeID < nodesPerGroup; localNodeID++) {
                    int nodeID = GroupID * nodesPerGroup + localNodeID;
                    if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                        ++thisGroupFreeNode;
                    }
                }
                if (jobSize <= thisGroupFreeNode) {
                    //allocate to this group.
                    int i = 0;//node index of the job.
                    for (int localNodeID = 0; localNodeID < nodesPerGroup; localNodeID++) {
                        int nodeID = GroupID * nodesPerGroup + localNodeID;
                        if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                            ai->nodeIndices[i] = nodeID;
                            occupiedNodes.insert(nodeID);
                            //std::cout << nodeID << " ";
                            ++i;
                            if (i == jobSize) {
                                finish = true;
                                //std::cout << ",grouped";
                                //std::cout << endl;
                                break;
                            }
                        }
                        else {
                            continue;
                        }
                    }
                }
                else {
                    continue;
                }
            }
            //no group has enough space for this small job.
            int i = 0;//node index of the job.
            for (int nodeID = 0; nodeID < nodesPerGroup * dMach.numGroups; nodeID++) {
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    occupiedNodes.insert(nodeID);
                    //std::cout << nodeID << " ";
                    ++i;
                    if (i == jobSize) {
                        //std::cout << ",simple from left";
                        //std::cout << endl;
                        break;
                    }
                }
                else {
                    continue;
                }
            }
        }
        //job cannot fit in one group, so
        //it will simply be allocated from the rightmost node.
        else {
            int i = 0;//node index of the job.
            for (int nodeID = nodesPerGroup * dMach.numGroups - 1; nodeID >= 0; nodeID--) {
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    occupiedNodes.insert(nodeID);
                    //std::cout << nodeID << " ";
                    ++i;
                    if (i == jobSize) {
                        //std::cout << ",simple from right";
                        //std::cout << endl;
                        break;
                    }
                }
                else {
                    continue;
                }
            }
        }
        return ai;
    }
    return NULL;
}

