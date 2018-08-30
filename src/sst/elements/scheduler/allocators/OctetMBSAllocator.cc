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
/**
 * By default the MBSAllocator provides a layered 2D mesh approach to
 * the Multi Buddy Strategy
 * A Note on Extending:  The only thing you need to do is override the initialize method,
 * create complete blocks, and make sure the "root" blocks are in the FBR->
 */

#include "sst_config.h"
#include "OctetMBSAllocator.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "StencilMachine.h"
#include "MBSAllocInfo.h"
#include "output.h"

#define MIN(a,b) ((a)<(b)?(a):(b))


using namespace SST::Scheduler;
using namespace std;

OctetMBSAllocator::OctetMBSAllocator(StencilMachine* m, int x, int y, int z) : MBSAllocator(m)
{
    //we don't do anything special in construction

    //create the starting blocks
    schedout.debug(CALL_INFO, 1, 0, "Initializing OctetMBSAllocator\n");
    std::vector<int> tempVec(3);
    tempVec[0] = x;
    tempVec[1] = y;
    tempVec[2] = z;
    std::vector<int> tempVec2(3,0);
    initialize(new MeshLocation(tempVec), new MeshLocation(tempVec2));
    //if (DEBUG) printFBR("Post Initialize:");
}

string OctetMBSAllocator::getSetupInfo(bool comment) const
{
    string com;
    if (comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    return com + "Multiple Buddy Strategy (MBS) Allocator using Octet divisions";
}

OctetMBSAllocator::OctetMBSAllocator(vector<string>* params, Machine *mach) : MBSAllocator(mach)
{
    //create the starting blocks
    schedout.debug(CALL_INFO, 1, 0, "Initializing OctetMBSAllocator\n");
    std::vector<int> tempVec(3);
    tempVec[0] = mMachine->dims[0];
    tempVec[1] = mMachine->dims[1];
    tempVec[2] = mMachine->dims[2];
    std::vector<int> tempVec2(3,0);
    initialize(new MeshLocation(tempVec), new MeshLocation(tempVec2));

    //if (DEBUG) printFBR("Post Initialize:");
}


void OctetMBSAllocator::initialize(MeshLocation* dim, MeshLocation* off)
{
    //System->out->println("Initializing a "+dim->x+"x"+dim->y+"x"+dim->z+" region at "+off);

    //figure out the largest cube possible
    int constraintSide = MIN(MIN(dim->dims[0], dim->dims[1]), dim->dims[2]);
    int maxSize = (int) (log((double) constraintSide) / log(2.0));
    int sideLen = (int) (1 << maxSize); // 2 ^ maxSize

    //Start creating our cube
    std::vector<int> tempVec(3);
    tempVec[0] = sideLen;
    tempVec[1] = sideLen;
    tempVec[2] = sideLen;
    MeshLocation* blockDim = new MeshLocation(tempVec);
    int blockSize = blockDim->dims[0] * blockDim->dims[1] * blockDim->dims[2];

    //see if we have already made one of these size blocks
    int rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (blockSize)));
    if (rank == (int)ordering -> size()){
        rank = createRank(blockSize);
    }

    //add this block to the FBR
    tempVec[0] = off->dims[0];
    tempVec[1] = off->dims[1];
    tempVec[2] = off->dims[2];
    std::vector<int> tempVec2(3,0);
    tempVec2[0] = blockDim->dims[0];
    tempVec2[1] = blockDim->dims[1];
    tempVec2[2] = blockDim->dims[2];
    Block* block = new Block(new MeshLocation(tempVec),new MeshLocation(tempVec2)); //Do I need to make a new dim object?
    FBR -> at(rank) -> insert(block);
    if (block -> size() > 1) createChildren(block);

    //initialize the 3 remaining regions
    if (dim->dims[0] - sideLen > 0) {
        tempVec[0] = dim->dims[0] - sideLen;
        tempVec[1] = sideLen;
        tempVec[2] = sideLen;
        tempVec2[0] = off->dims[0] + sideLen;
        tempVec2[1] = off->dims[1];
        tempVec2[2] = off->dims[2];
        initialize(new MeshLocation(tempVec), new MeshLocation(tempVec2));
    }
    if (dim->dims[1] - sideLen > 0) {
        tempVec[0] = dim->dims[0];
        tempVec[1] = dim->dims[1] - sideLen;
        tempVec[2] = sideLen;
        tempVec2[0] = off->dims[0];
        tempVec2[1] = off->dims[1] + sideLen;
        tempVec2[2] = off->dims[2];
        initialize(new MeshLocation(tempVec), new MeshLocation(tempVec2));
    }
    if (dim->dims[2] - sideLen > 0) {
        tempVec[0] = dim->dims[0];
        tempVec[1] = dim->dims[1];
        tempVec[2] = dim->dims[2] - sideLen;
        tempVec2[0] = off->dims[0];
        tempVec2[1] = off->dims[1];
        tempVec2[2] = off->dims[2] + sideLen;
        initialize(new MeshLocation(tempVec), new MeshLocation(tempVec2));
    }
}

set<Block*, Block>* OctetMBSAllocator::splitBlock(Block* b)
{
    //create a set to iterate over
    Block* BComp = new Block();
    set<Block*, Block>* children = new set<Block*, Block>(*BComp);

    //determine the size (blocks should be cubes, thus dimension->x=dimension->y=dimension->z)
    int size = (int) (log(b -> dimension->dims[0]) / log(2));

    //we want one size smaller, but they need to be
    if (size - 1 >= 0) {
        //determine new sideLen
        int sideLen = (int) (1 << (size - 1)); // 2^size - 1
        std::vector<int> tempVec(3);
        tempVec[0] = sideLen;
        tempVec[1] = sideLen;
        tempVec[2] = sideLen;
        MeshLocation* dim = new MeshLocation(tempVec);

        //Bottom layer of the cube
        tempVec[0] = b -> location->dims[0];
        tempVec[1] = b -> location->dims[1];
        tempVec[2] = b -> location->dims[2];
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
        tempVec[1] = b -> location->dims[1] + sideLen;
        children -> insert(new Block(new MeshLocation(tempVec),dim,b));
        tempVec[0] = b -> location->dims[0] + sideLen;
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
        tempVec[1] = b -> location->dims[1];
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
        //Top layer of the cube
        tempVec[0] = b -> location->dims[0];
        tempVec[2] = b -> location->dims[2] + sideLen;
        children -> insert(new Block(new MeshLocation(tempVec),dim, b));
        tempVec[1] = b -> location->dims[1] + sideLen;
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
        tempVec[0] = b -> location->dims[0] + sideLen;
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
        tempVec[1] = b -> location->dims[1];
        children -> insert(new Block(new MeshLocation(tempVec), dim, b));
    }

    //if (DEBUG) printf("Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    schedout.debug(CALL_INFO, 7, 0, "Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    return children;
} 
