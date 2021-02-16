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

#include "DflyRRRAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflyRRRAllocator::DflyRRRAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{
}

DflyRRRAllocator::~DflyRRRAllocator()
{
}

std::string DflyRRRAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Round Robin Routers Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflyRRRAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        const int nodesPerGroup = dMach.nodesPerRouter * dMach.routersPerGroup;
        //std::cout << "jobSize = " << jobSize << ", allocation, ";
        int groupID = 0;
        int i = 0;
        while (i < jobSize) {
            // find the first not-full router in this group,
            // if cannot find one, go to the next group.
            int findLocalRouterID = -1;
            while (findLocalRouterID == -1) {
                for (int localRouterID = 0; localRouterID < dMach.routersPerGroup; localRouterID++) {
                    for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                        int nodeID = groupID * nodesPerGroup + localRouterID * dMach.nodesPerRouter + localNodeID;
                        if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                            findLocalRouterID = localRouterID;
                            break;
                        }
                    }
                    if (findLocalRouterID != -1) {
                        break;
                    }
                }
                // haven't found one such router, go to next group.
                if (findLocalRouterID == -1) {
                    if (groupID < dMach.numGroups - 1) {
                        ++groupID;
                    }
                    else {
                        groupID = 0;
                    }
                }
            }
            // allocate all the nodes in this router,
            for (int localNodeID = 0; localNodeID < dMach.nodesPerRouter; localNodeID++) {
                int nodeID = groupID * nodesPerGroup + findLocalRouterID * dMach.nodesPerRouter + localNodeID;
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    occupiedNodes.insert(nodeID);
                    //std::cout << nodeID << " ";
                    ++i;
                    if (i == jobSize) {
                        break;
                    }
                }
            }
            // then go to the next group.
            if (groupID < dMach.numGroups - 1) {
                ++groupID;
            }
            else {
                groupID = 0;
            }
        }
        //std::cout << endl;
        return ai;
    }
    return NULL;
}

