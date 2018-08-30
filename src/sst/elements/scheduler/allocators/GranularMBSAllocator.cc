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
#include "GranularMBSAllocator.h"

#include <vector>
#include <string>
#include <set>
#include <cstdio>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "StencilMachine.h"
#include "MBSAllocInfo.h"
#include "output.h"

#define DEBUG false

using namespace SST::Scheduler;

std::string GranularMBSAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com ="# ";
    } else { 
        com = "";
    }
    return com + "Multiple Buddy Strategy (MBS) Allocator using Granular divisions";
}

GranularMBSAllocator::GranularMBSAllocator(std::vector<std::string>* params, Machine *mach) : MBSAllocator(mach)
{
    mMachine = dynamic_cast<StencilMachine*>(mach);
    if (NULL == mMachine || mMachine->numDims() != 3) {
        schedout.fatal(CALL_INFO, 1, "Granular MBS Allocator requires 3D mesh/torus machine.");
    }
    
    std::vector<int> tempVec1(3);
    tempVec1[0] = mMachine->dims[0];
    tempVec1[1] = mMachine->dims[1];
    tempVec1[2] = mMachine->dims[2];
    std::vector<int> tempVec2(3, 0);
    initialize(new MeshLocation(tempVec1), 
               new MeshLocation(tempVec2));
}

void GranularMBSAllocator::initialize(MeshLocation* dim, MeshLocation* off)
{
    //add all the 1x1x1's to the std::set of blocks
    int rank = createRank(1);
    std::vector<int> tempVec(3, 1);
    MeshLocation* sizeOneDim = new MeshLocation(tempVec);
    for (int i = 0; i < dim->dims[0]; i++){
        tempVec[0] = i;
        for (int j = 0;j < dim->dims[1]; j++){
            tempVec[1] = j;
            for (int k = 0;k < dim->dims[2]; k++){
                tempVec[2] = k;
                this -> FBR -> at(rank) -> insert(new Block(new MeshLocation(tempVec), sizeOneDim));
            }
        }
    }

    //iterate over all the ranks
    while(mergeAll()){
    }
}

/**
 * The new mergeAll, with start with the given rank, and scan all the ranks below it (descending).
 * mergeAll with make 3 passes for each rank
 */
bool GranularMBSAllocator::mergeAll()
{
    //default return
    bool retVal = false;

    //workaround to delete during iteration
    Block* BComp = new Block();
    std::set<Block*, Block>* toRemove = new std::set<Block*, Block>(*BComp);

    //we will be scanning 3 times, for each dimension
    for (int d = 0;d < 3;d++) {		
        //scan through and try to merge everything
        for (int i = (ordering->size()-1); i >= 0; i--) {
            std::set<Block*, Block>* blocks = FBR -> at(i);
            std::set<Block*, Block>::iterator it = blocks -> begin();

            while (blocks -> size() > 0 && it != blocks -> end()) {
                //get the first block
                Block* first = *it;
                it++;

                //make sure we aren't trying to remove it
                if (toRemove -> count(first) != 0){
                    blocks -> erase(first);
                    toRemove -> erase(first);
                } else {
                    //figure out the second block
                    Block* second = nextBlock(d,first); //TODO: delete this temporary block

                    if (blocks -> count(second) != 0){
                        //get the real second block
                        second = FBRGet(second);

                        //make our new block 
                        Block* newBlock = mergeBlocks(first,second);

                        //preserve hierarchy
                        newBlock -> addChild(first);
                        newBlock -> addChild(second);
                        first -> parent = newBlock;
                        second -> parent = newBlock;

                        //add new block
                        int newRank = createRank(newBlock -> size());
                        FBR -> at(newRank) -> insert(newBlock);

                        //get rid of the old blocks
                        blocks -> erase(first); //it's slower to erase by value instead of using the iterator but we can't corrupt it (it's already been incremented anyway for the same reason) 
                        toRemove -> insert(second);

                        //change return value since we created a block
                        retVal = true;
                    } //ends nested if
                } //ends removal if
            } //ends while iteration
        } //ends for through ranks
    } //ends for all dimensions
    return retVal;
}

/**
 * returns the next block, by looking in x,y,z directions in the order depending on a given d
 */
Block* GranularMBSAllocator::nextBlock(int d, Block* first)
{
    if (d < 0 || d > 2) {
        //error("Index out of bounds in nextBlock");
        schedout.fatal(CALL_INFO, 1, "Index out of bounds in nextBlock");
    }
    if (d == 0) {
        return lookX(first);
    }
    if (d == 1) {
        return lookY(first);
    }
    if (d == 2) {
        return lookZ(first);
    }
    //error("nextBlock returning NULL!");
    schedout.fatal(CALL_INFO, 1, "nextBlock returning NULL!");
    return NULL;
}

/**
 * Return a block that is the given blocks partner in the x direction.
 */
Block* GranularMBSAllocator::lookX(Block* b)
{
    std::vector<int> tempVec(3);
    tempVec[0] = b -> location->dims[0] + b -> dimension->dims[0];
    tempVec[1] = b -> location->dims[1];
    tempVec[2] = b -> location->dims[2];
    return new Block(new MeshLocation(tempVec),b -> dimension); //TODO:  do we want to copy the dimensio as well?
}

Block* GranularMBSAllocator::lookY(Block* b)
{
    std::vector<int> tempVec(3);
    tempVec[0] = b -> location->dims[0];
    tempVec[1] = b -> location->dims[1] + b -> dimension->dims[1];
    tempVec[2] = b -> location->dims[2];
    return new Block(new MeshLocation(tempVec),b -> dimension);
}

Block* GranularMBSAllocator::lookZ(Block* b)
{
    std::vector<int> tempVec(3);
    tempVec[0] = b -> location->dims[0];
    tempVec[1] = b -> location->dims[1];
    tempVec[2] = b -> location->dims[2] + b -> dimension->dims[2];
    return new Block(new MeshLocation(tempVec),b -> dimension);
}

/**
 * Attempts to perform a get operation on a std::set. Returns NULL if the block is not found
 * This method is needed because Block*.equals() is not really equals, but really similarEnough()
 * We need to get the block from the FBR because it comes with the parent/children hierarchy.
 */
Block* GranularMBSAllocator::FBRGet(Block* needle)
{
    Block* retVal;

    //Figure out where too look
    std::set<Block*, Block>* haystack = FBR -> at(distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(),needle -> size())));

    if (haystack -> count(needle) == 0){
        //error("nextBlock not currently in FBR");
        schedout.fatal(CALL_INFO, 1, "nextBlock not currently in FBR");
        return NULL;
    }

    //Locate it, and return
    std::set<Block*, Block>::iterator it = haystack -> begin();
    if (it == haystack -> end()) {
        //error("no blocks of correct rank in FBR when searching for nextBlock");
        schedout.fatal(CALL_INFO, 1, "no blocks of correct rank in FBR when searching for nextBlock");
        return NULL;
    }
    retVal = *it;
    while (!needle -> equals(retVal)) {
        if (it == haystack -> end()) {
            //error("nextBlock not found in correct rank of FBR");
            schedout.fatal(CALL_INFO, 1, "nextBlock not found in correct rank of FBR");
            return NULL;
        }
        ++it;
        retVal = *it;
    }
    return retVal;
}

//Merges two blocks
Block* GranularMBSAllocator::mergeBlocks(Block* first, Block* second)
{
    if (first -> equals(second) || !first -> dimension -> equals(*(second -> dimension))){
        //error("merging two idential blocks, or blocks of different sizes");
        schedout.fatal(CALL_INFO, 1, "merging two idential blocks, or blocks of different sizes");
    }
    //do some std::setup
    std::vector<int> tempVec(3,0);
    MeshLocation* dimension = new MeshLocation(tempVec);
    MeshLocation* location = second->location;
    if ((*dimension)(first -> location,second -> location))  {
        location = first -> location;
    }
    //determine whether we need to change the x dimension
    if (first -> location->dims[0] == second -> location->dims[0]){
        tempVec[0] = first -> dimension->dims[0];
    } else {
        tempVec[0] = first -> dimension->dims[0] + second -> dimension->dims[0];
    }
    //determine whether we need to change the y dimension
    if (first -> location->dims[1] == second -> location->dims[1]){
        tempVec[1] = first -> dimension->dims[1];
    } else {
        tempVec[1] = first -> dimension->dims[1] + second -> dimension->dims[1];
    }
    //determine whether we need to change the z dimension
    if (first -> location->dims[2] == second -> location->dims[2]){
        tempVec[2] = first -> dimension->dims[2];
    } else {
        tempVec[2] = first -> dimension->dims[2]+second -> dimension->dims[2];
    }
    delete dimension;
    dimension = new MeshLocation(tempVec);
    Block* toReturn = new Block(location,dimension);
    toReturn -> addChild(first);
    toReturn -> addChild(second);
    return toReturn;
}

