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

#include "sst_config.h"

#include "DflyRRNAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflyRRNAllocator::DflyRRNAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{
}

DflyRRNAllocator::~DflyRRNAllocator()
{
}

std::string DflyRRNAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Round Robin Nodes Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflyRRNAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        const int nodesPerGroup = dMach.routersPerGroup * dMach.nodesPerRouter;
        //std::cout << "jobSize = " << jobSize << ", allocation, ";
        int groupID = 0;
        for (int i = 0; i < jobSize; i++) {
            int localNodeID = 0;
            while (true) {
                int nodeID = groupID * nodesPerGroup + localNodeID;
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    occupiedNodes.insert(nodeID);
                    //std::cout << nodeID << " ";
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
        //std::cout << endl;
        return ai;
    }
    return NULL;
}

