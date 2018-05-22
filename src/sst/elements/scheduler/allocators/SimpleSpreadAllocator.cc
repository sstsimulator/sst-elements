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

#include "sst_config.h"

#include "SimpleSpreadAllocator.h"

#include "AllocInfo.h"
#include "DragonflyMachine.h"
#include "Job.h"

using namespace SST::Scheduler;

SimpleSpreadAllocator::SimpleSpreadAllocator(const DragonflyMachine & mach)
  : DragonflyAllocator(mach)
{

}

std::string SimpleSpreadAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Simple Spread Allocator";
}

#include <iostream>
using namespace std;


AllocInfo* SimpleSpreadAllocator::allocate(Job* j)
{
    if (canAllocate(*j)) {
        AllocInfo* ai = new AllocInfo(j, dMach);
        int lastNode = -1;
        for(int i = 0; i < ai->getNodesNeeded(); i++) {
            do {
                lastNode = nextNodeId(lastNode);
            } while(!dMach.isFree(lastNode));
            int rId = dMach.routerOf(lastNode);
            ai->nodeIndices[i] = lastNode;
        }
        return ai;
    }
    return NULL;
}

int SimpleSpreadAllocator::nextNodeId(int curNode)
{
    if(curNode < 0) {
        return 0;
    }
    if((curNode + 1) % dMach.nodesPerRouter != 0) {
        //using the same router
        return curNode + 1;
    }
    //calculate the next router's ID
    int rId = dMach.routerOf(curNode);
    if(dMach.groupOf(rId) != dMach.numGroups - 1) {
        //use the same local ID in the next group
        rId += dMach.routersPerGroup;
    } else {
        //use the next local ID in the first group
        rId = dMach.localIdOf(rId + 1);
    }
    return rId * dMach.nodesPerRouter;
}
