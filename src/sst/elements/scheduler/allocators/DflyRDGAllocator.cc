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
#include "sst/core/rng/mersenne.h"

#include "DflyRDGAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

DflyRDGAllocator::DflyRDGAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{
    rng = new SST::RNG::MersenneRNG();
}

DflyRDGAllocator::~DflyRDGAllocator()
{
    delete rng;
}

std::string DflyRDGAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Dragonfly Random Group Allocator";
}

#include <iostream>
using namespace std;

AllocInfo* DflyRDGAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        //This set keeps track of allocated nodes in the current allocation.
        std::set<int> occupiedNodes;
        const int jobSize = ai->getNodesNeeded();
        const int nodesPerGroup = dMach.nodesPerRouter * dMach.routersPerGroup;
        //std::cout << "jobSize = " << jobSize << ", allocation, ";
        int i = 0;
        while (i < jobSize) {
            //randomly choose a group.
            int groupID = rng->generateNextUInt32() % dMach.numGroups;
            //select nodes one by one in this group.
            for (int localNodeID = 0; localNodeID < nodesPerGroup; localNodeID++) {
                int nodeID = groupID * nodesPerGroup + localNodeID;
                if ( dMach.isFree(nodeID) && occupiedNodes.find(nodeID) == occupiedNodes.end() ) {
                    ai->nodeIndices[i] = nodeID;
                    ++i;
                    occupiedNodes.insert(nodeID);
                    //std::cout << nodeID << " ";
                }
                else {
                    continue;
                }
                if (i == jobSize) {
                    break;
                }
            }
        }
        //std::cout << endl;
        return ai;
    }
    return NULL;
}

