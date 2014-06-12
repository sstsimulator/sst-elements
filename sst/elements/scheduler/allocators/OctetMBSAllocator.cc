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
/**
 * By default the MBSAllocator provides a layered 2D mesh approach to
 * the Multi Buddy Strategy
 * A Note on Extending:  The only thing you need to do is override the initialize method,
 * create complete blocks, and make sure the "root" blocks are in the FBR->
 */

#include "sst_config.h"
#include "OctetMBSAllocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <time.h>
#include <math.h>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "MBSAllocInfo.h"
#include "misc.h"
#include "output.h"

#define MIN(a,b) ((a)<(b)?(a):(b))


using namespace SST::Scheduler;
using namespace std;

OctetMBSAllocator::OctetMBSAllocator(MachineMesh* m, int x, int y, int z) : MBSAllocator(m)
{
    //we don't do anything special in construction

    //create the starting blocks
    schedout.debug(CALL_INFO, 1, 0, "Initializing OctetMBSAllocator\n");
    initialize(new MeshLocation(x,y,z), new MeshLocation(0,0,0));
    //if (DEBUG) printFBR("Post Initialize:");
}

string OctetMBSAllocator::getSetupInfo(bool comment)
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
    initialize(
               new MeshLocation(meshMachine -> getXDim(),meshMachine -> getYDim(),meshMachine -> getZDim()), 
               new MeshLocation(0,0,0));

    //if (DEBUG) printFBR("Post Initialize:");
}


void OctetMBSAllocator::initialize(MeshLocation* dim, MeshLocation* off)
{
    //System->out->println("Initializing a "+dim->x+"x"+dim->y+"x"+dim->z+" region at "+off);

    //figure out the largest cube possible
    int constraintSide = MIN(MIN(dim -> x, dim -> y), dim -> z);
    int maxSize = (int) (log((double) constraintSide) / log(2.0));
    int sideLen = (int) (1 << maxSize); // 2 ^ maxSize

    //Start creating our cube
    MeshLocation* blockDim = new MeshLocation(sideLen,sideLen,sideLen);
    int blockSize = blockDim -> x * blockDim -> y * blockDim -> z;

    //see if we have already made one of these size blocks
    int rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (blockSize)));
    if (rank == (int)ordering -> size()){
        rank = createRank(blockSize);
    }

    //add this block to the FBR
    Block* block = new Block(new MeshLocation(off -> x,off -> y,off -> z),new MeshLocation(blockDim -> x,blockDim -> y,blockDim -> z)); //Do I need to make a new dim object?
    FBR -> at(rank) -> insert(block);
    if (block -> size() > 1) createChildren(block);

    //initialize the 3 remaining regions
    if (dim -> x - sideLen > 0) {
        initialize(new MeshLocation(dim -> x -sideLen,sideLen,sideLen), new MeshLocation(off -> x + sideLen,off -> y,off -> z));
    }
    if (dim -> y - sideLen > 0) {
        initialize(new MeshLocation(dim -> x,dim -> y - sideLen,sideLen), new MeshLocation(off -> x,off -> y + sideLen,off -> z));
    }
    if (dim -> z - sideLen > 0) {
        initialize(new MeshLocation(dim -> x,dim -> y,dim -> z - sideLen), new MeshLocation(off -> x,off -> y,off -> z + sideLen));
    }
}

set<Block*, Block>* OctetMBSAllocator::splitBlock(Block* b)
{
    //create a set to iterate over
    Block* BComp = new Block();
    set<Block*, Block>* children = new set<Block*, Block>(*BComp);

    //determine the size (blocks should be cubes, thus dimension->x=dimension->y=dimension->z)
    int size = (int) (log(b -> dimension -> x) / log(2));

    //we want one size smaller, but they need to be
    if (size - 1 >= 0) {
        //determine new sideLen
        int sideLen = (int) (1 << (size - 1)); // 2^size - 1
        MeshLocation* dim = new MeshLocation(sideLen,sideLen,sideLen);

        //Bottom layer of the cube
        children -> insert(new Block(new MeshLocation(b -> location -> x, b -> location -> y, b -> location -> z), dim, b));
        children -> insert(new Block(new MeshLocation(b -> location -> x, b -> location -> y + sideLen, b -> location -> z),dim,b));
        children -> insert(new Block(new MeshLocation(b -> location -> x + sideLen, b -> location -> y+sideLen, b -> location -> z), dim, b));
        children -> insert(new Block(new MeshLocation(b -> location -> x + sideLen, b -> location -> y, b -> location -> z), dim, b));
        //Top layer of the cube
        children -> insert(new Block(new MeshLocation(b -> location -> x, b -> location -> y, b -> location -> z + sideLen),dim, b));
        children -> insert(new Block(new MeshLocation(b -> location -> x, b -> location -> y + sideLen, b -> location -> z + sideLen), dim, b));
        children -> insert(new Block(new MeshLocation(b -> location -> x + sideLen, b -> location -> y + sideLen, b -> location -> z + sideLen), dim, b));
        children -> insert(new Block(new MeshLocation(b -> location -> x + sideLen, b -> location -> y, b -> location -> z + sideLen), dim, b));
    }

    //if (DEBUG) printf("Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    schedout.debug(CALL_INFO, 7, 0, "Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    return children;
} 
