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
#include "MBSAllocator.h"

#include <sstream>
#include <cstdio>
#include <string>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "StencilMachine.h"
#include "MBSAllocInfo.h"
#include "output.h"

#define MIN(a,b)  ((a)<(b)?(a):(b))
#define DEBUG false

using namespace SST::Scheduler;

//this constructor doesn't call initialize() and is for derived classes
MBSAllocator::MBSAllocator(Machine* mach) : Allocator(*mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    mMachine = dynamic_cast<StencilMachine*>(mach);
    if (NULL == mMachine || mMachine->numDims() != 3) {
        schedout.fatal(CALL_INFO, 1, "MBS Allocator requires 3D mesh/torus machine.");
    }
    FBR = new std::vector<std::set<Block*,Block>*>();
    ordering = new std::vector<int>();
}

MBSAllocator::MBSAllocator(StencilMachine* m, int x, int y, int z) : Allocator(*m)
{
    schedout.init("", 8, 0, Output::STDOUT);
    mMachine = m;
    FBR = new std::vector<std::set<Block*,Block>*>();
    ordering = new std::vector<int>();

    //create the starting blocks
    schedout.debug(CALL_INFO, 1, 0, "Initializing MBSAllocator:");
    std::vector<int> tempVec1(3);
    tempVec1[0] = x;
    tempVec1[1] = y;
    tempVec1[2] = z;
    std::vector<int> tempVec2(3, 0);
    initialize(new MeshLocation(tempVec1), 
               new MeshLocation(tempVec2));
    //if (DEBUG) printFBR("Post Initialize:");
}

MBSAllocator::MBSAllocator(std::vector<std::string>* params, Machine* mach) : Allocator(*mach)
{ 
    mMachine = dynamic_cast<StencilMachine*>(mach);
    if (NULL == mMachine || mMachine->numDims() != 3) {
        schedout.fatal(CALL_INFO, 1, "MBS Allocator requires 3D mesh/torus machine.");
    }
    FBR = new std::vector<std::set<Block*,Block>*>();
    ordering = new std::vector<int>();

    //create the starting blocks
    schedout.debug(CALL_INFO, 1, 0, "Initializing MBSAllocator:");
    std::vector<int> tempVec1(3);
    tempVec1[0] = mMachine->dims[0];
    tempVec1[1] = mMachine->dims[1];
    tempVec1[2] = mMachine->dims[2];
    std::vector<int> tempVec2(3, 0);
    initialize(new MeshLocation(tempVec1), 
               new MeshLocation(tempVec2));

}

std::string MBSAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Multiple Buddy Strategy (MBS) Allocator";
}

std::string MBSAllocator::getParamHelp()
{
    return "";
}

/**
 * Initialize will fill in the FBR with z number of blocks (1 for
 * each layer) that fit into the given x,y dimensions.  It is
 * assumed that those dimensions are non-zero.
 */
void MBSAllocator::initialize(MeshLocation* dim, MeshLocation* off)
{
    schedout.debug(CALL_INFO, 7, 0, "Initializing a %dx%dx%d region at %s\n", dim->dims[0], dim->dims[1], dim->dims[2], off -> toString().c_str());

    //Figure out the largest possible block possible
    int maxSize = (int) (log((double) MIN(dim->dims[0],dim->dims[1])) / log(2.0));
    int sideLen = (int) (1 << maxSize); //supposed to be 2^maxSize
    //create a flat square
    std::vector<int> tempVec1(3);
    std::vector<int> tempVec2(3);
    tempVec1[0] = sideLen;
    tempVec1[1] = sideLen;
    tempVec1[2] = 1;
    
    MeshLocation* blockdim = new MeshLocation(tempVec1);
    int size = blockdim->dims[0] * blockdim->dims[1];
    size *= blockdim->dims[2];

    //see if we have already made one of these size blocks
    int rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), size));
    if (rank == (int)ordering -> size()){
        rank = createRank(size);
    }

    //add block to the set at the given rank, determined by lookup
    for (int i = 0; i < dim->dims[2]; i++) {
        tempVec1[0] = off->dims[0];
        tempVec1[1] = off->dims[1];
        tempVec1[2] = i;
        tempVec2[0] = blockdim->dims[0];
        tempVec2[1] = blockdim->dims[1];
        tempVec2[2] = blockdim->dims[2];
        Block* block = new Block(new MeshLocation(tempVec1),new MeshLocation(tempVec2)); 
        FBR -> at(rank) -> insert(block);
        createChildren(block);

        //update the rank (createChildren may have added new ranks to ordering and FBR)
        rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), size));
    }

    //initialize the two remaining rectangles of the region
    if (dim->dims[0] - sideLen > 0) {
        tempVec1[0] = dim->dims[0] - sideLen;
        tempVec1[1] = dim->dims[1];
        tempVec1[2] = dim->dims[2];
        tempVec2[0] = off->dims[0] + sideLen;
        tempVec2[1] = off->dims[1];
        tempVec2[2] = 1;
        initialize(new MeshLocation(tempVec1),new MeshLocation(tempVec2));
    }
    if (dim->dims[1] - sideLen > 0) {
        tempVec1[0] = sideLen;
        tempVec1[1] = dim->dims[1] - sideLen;
        tempVec1[2] = dim->dims[2];
        tempVec2[0] = off->dims[0];
        tempVec2[1] = off->dims[1] + sideLen;
        tempVec2[2] = 1;
        initialize(new MeshLocation(tempVec1),new MeshLocation(tempVec2));
    }
    delete blockdim;

}

/*
 * Creates a rank in both the FBR, and in the ordering.
 * If a rank already exists, it does not create a new rank,
 * it just returns the one already there
 */
int MBSAllocator::createRank(int size)
{
    std::vector<int>::iterator it = find(ordering -> begin(), ordering -> end(), size);
    int i = distance(ordering -> begin(),it); 
    if (it != ordering -> end()) {
        return i;
    }


    std::vector<std::set<Block*,Block>*>::iterator FBRit = FBR -> begin();
    i = 0;
    for (it = ordering -> begin();it != ordering -> end() && *it < size; it++){
        i++;
        FBRit++;
    }

    //add this block size into the ordering
    ordering -> insert(it,size);

    //make our corresponding set
    Block* BComp = new Block();
    FBR -> insert(FBRit, new std::set<Block*, Block>(*BComp));
    delete BComp;

    //if (DEBUG) printf("Added a rank %d for size %d\n", i, size);
    schedout.debug(CALL_INFO, 7, 0, "Added a rank %d for size %d\n", i, size);
    return i;
}

/**
 *  Essentially this will reinitialize a block, except add the
 *  children to the b->children, then recurse
 */
void MBSAllocator::createChildren(Block* b){
    std::set<Block*, Block>* childrenset = splitBlock(b);
    std::set<Block*, Block>::iterator children = childrenset -> begin();
    Block* next;

    //if (DEBUG) printf("Creating children for %s :: ", b -> toString().c_str());
    schedout.debug(CALL_INFO, 7, 0, "Creating children for %s :: ", b -> toString().c_str());

    while (children != childrenset -> end()){
        next = *children;

        //if (DEBUG) printf("%s ", next->toString().c_str());
        schedout.debug(CALL_INFO, 7, 0, "%s ", next->toString().c_str());

        b -> addChild(next);

        //make sure the proper rank exists, in both ordering and FBR
        int size = next -> dimension->dims[0] * next -> dimension->dims[1]*next -> dimension->dims[2];
        createRank(size);

        if (next -> size() > 1) {
            createChildren(next);
        }
        children++;
    }
    //if (DEBUG) printf("\n");
    schedout.debug(CALL_INFO, 7, 0, "\n");
}

std::set<Block*, Block>* MBSAllocator::splitBlock (Block* b) 
{
    //create the set to iterate over
    Block* BCComp = new Block();
    std::set<Block*, Block>* children = new std::set<Block*, Block>(*BCComp);
    delete BCComp;

    //determine the size (blocks should be cubes, thus dimension->x=dimension->y)
    int size = (int) (log((double) b -> dimension->dims[0])/log(2));
    //we want one size smaller, but they need to be
    if(size - 1 >= 0){
        int sideLen =  1 << (size - 1);
        std::vector<int> tempVec1(3);
        tempVec1[0] = sideLen;
        tempVec1[1] = sideLen;
        tempVec1[2] = 1;
        MeshLocation* dim = new MeshLocation(tempVec1);

        tempVec1[0] = b -> location->dims[0];
        tempVec1[1] = b -> location->dims[1];
        tempVec1[2] = b -> location->dims[2];
        children -> insert(new Block(new MeshLocation(tempVec1), dim,b));
        tempVec1[0] = b -> location->dims[0];
        tempVec1[1] = b -> location->dims[1] + sideLen;
        tempVec1[2] = b -> location->dims[2];
        children -> insert(new Block(new MeshLocation(tempVec1), dim,b));
        tempVec1[0] = b -> location->dims[0] + sideLen;
        tempVec1[1] = b -> location->dims[1] + sideLen;
        tempVec1[2] = b -> location->dims[2];
        children -> insert(new Block(new MeshLocation(tempVec1), dim,b));
        tempVec1[0] = b -> location->dims[0] + sideLen;
        tempVec1[1] = b -> location->dims[1];
        tempVec1[2] = b -> location->dims[2];
        children -> insert(new Block(new MeshLocation(tempVec1), dim,b));
    }
    //if (DEBUG) printf("Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    schedout.debug(CALL_INFO, 7, 0, "Made blocks for splitBlock(%s)\n", b -> toString().c_str());
    return children;
}

MBSMeshAllocInfo* MBSAllocator::allocate(Job* job)
{
    //if (DEBUG) printf("Allocating %s\n",job -> toString().c_str());
    schedout.debug(CALL_INFO, 7, 0, "Allocating %s\n",job -> toString().c_str());

    MBSMeshAllocInfo* retVal = new MBSMeshAllocInfo(job, machine);
    int allocated = 0;

    //a map of dimensions to numbers
    std::map<int,int>* RBR = factorRequest(job);

    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

    while (allocated < nodesNeeded){
        //Start trying allocate the largest blocks
        if (RBR -> empty()) {
            schedout.fatal(CALL_INFO, 1, "RBR empty in allocate()");
        }
        int currentRank = RBR -> rbegin() -> first; //this gives us the largest key in RBR

        //see if there is a key like that in FBR->
        if (FBR -> at(currentRank) -> size() > 0){  //TODO: try/catch for error?
            //Move the block from FBR to retVal
            Block* newBlock = *(FBR -> at(currentRank) -> begin());
            retVal -> blocks -> insert(newBlock);
            FBR -> at(currentRank) -> erase(newBlock);

            //add all the processors to retVal, and make progress
            //in the loop
            std::set<MeshLocation*, MeshLocation>* newBlockprocs = newBlock -> processors();
            std::set<MeshLocation*, MeshLocation>::iterator it = newBlockprocs -> begin();
            //processors() is sorted by MeshLocation comparator
            for (int i = allocated; it != newBlockprocs -> end();i++){
                retVal -> nodes -> at(i) = *(it);
                retVal -> nodeIndices[i] = (*it) -> toInt(*mMachine);
                it++;
                allocated++;
            }
            int currentval = RBR->find(currentRank)->second;
            //also be sure to remove the allocated block from the RBR
            if (currentval - 1 > 0){
                RBR -> erase(currentRank);
                RBR -> insert(std::pair<int, int>(currentRank,currentval-1));
            } else {
                RBR -> erase(currentRank);
            }
        } else {
            //See if there is a larger one we can split up
            if (!splitLarger(currentRank)){
                //since we were unable to split a larger block, make request smaller
                splitRequest(RBR,currentRank);

                //if there are non left to request at the current rank, clean up the RBR
                if (RBR -> find(currentRank) -> second <= 0){
                    RBR -> erase(currentRank);

                    //make sure we look at the next lower rank (commented out in the Java)
                    //currentRank = currentRank - 1;
                }

            }
            //if (DEBUG) printFBR("After all splitting");
            schedout.debug(CALL_INFO, 7, 0, "After all splitting");
        }
    }
    RBR -> clear();
    delete RBR;
    return retVal;
}

/**
 * Calculates the RBR, which is a map of ranks to number of blocks at that rank
 */
std::map<int,int>* MBSAllocator::factorRequest(Job* j)
{
    std::map<int,int>* retVal = new std::map<int,int>();
    int procs = 0;

    int nodesNeeded = ceil((double) j->getProcsNeeded() / machine.coresPerNode);

    while (procs < nodesNeeded){
        //begin our search
        std::vector<int>::iterator sizes = ordering -> begin();

        //look for the largest size block that fits the procs needed
        int size = -1;
        int prevSize = -1;
        while (sizes != ordering -> end()){
            prevSize = size;
            size = *(sizes);
            sizes++;
            if (size > (nodesNeeded - procs)){
                //cancel the progress made with this run through the loop
                size = prevSize;
                break;
            }
        }
        //make sure something was done
        if (prevSize == -1 || size == -1) {
            //catch the special case where we only have one size
            if (ordering -> size() == 1) {
                size = ordering -> at(0);
            } else {
                schedout.fatal(CALL_INFO, 1, "while loop never ran in MBSAllocator");
            }
        }

        //get the rank
        int rank = distance(ordering -> begin(), find(ordering -> begin(),ordering -> end(), size));
        if (retVal -> find(rank) == retVal -> end()){
            retVal -> insert(std::pair<int,int>(rank,0));
        }

        //increment that value of the map
        retVal -> find(rank) -> second++;

        //make progress in the larger while loop
        procs += ordering -> at(rank);
    }

    schedout.debug(CALL_INFO, 7, 0, "Factored request: \n");
    printRBR(retVal);
    return retVal;
}

/**
 * Breaks up a request for a block with a given rank into smaller request if able
 */

void MBSAllocator::splitRequest(std::map<int,int>* RBR, int rank){
    if (RBR -> count(rank) == 0)
        schedout.fatal(CALL_INFO, 1, "Out of bounds in MBSAllocator::splitRequest()");
    if (rank <= 0)
        schedout.fatal(CALL_INFO, 1, "Cannot split a request of size 0");
    if (RBR -> find(rank) -> second == 0){
        //throw new UnsupportedOperationException("Cannot split a block of size 0");
        schedout.fatal(CALL_INFO, 1, "Cannot split a block of size 0");
        return;
    }

    //decrement the current rank
    RBR -> find(rank) -> second--;

    //get the number of blocks we need from the previous rank
    int count = (int) ordering -> at(rank)/ordering -> at(rank - 1);

    //increment the previous rank, and if it doesn't exists create it
    if (RBR -> find(rank - 1) != RBR-> end()) {
        RBR -> find(rank - 1) -> second += count;
    } else {
        RBR -> insert(std::pair<int,int>(rank - 1,count));
    }

    schedout.debug(CALL_INFO, 7, 0, "Split a request up\n");
    printRBR(RBR);
}

/**
 * Determines whether a split up of a possible larger block was
 * successful->  It begins looking at one larger than rank->
 */
bool MBSAllocator::splitLarger(int rank)
{
    schedout.debug(CALL_INFO, 7, 0, "Splitting a block at rank %d\n",rank);

    //make sure that we can search in rank+1
    //FBR has same size as ordering
    if (rank + 1 >= (int)FBR -> size()) {
        return false;
    }

    //pass off the work
    if (FBR -> at(rank + 1) -> size() == 0) {
        //recurse! if necessary
        if (!splitLarger(rank + 1)) {
            return false;
        }
    }

    //split a block since by this point in the method we have guaranteed its existence
    Block* toSplit = *(FBR -> at(rank + 1) -> begin());
    std::set<Block*, Block>::iterator spawn = toSplit -> getChildren() -> begin();

    //add children to the FBR
    while (spawn != toSplit -> getChildren() -> end()) {
        FBR -> at(rank) -> insert(*spawn);
        spawn++;
    }

    //remove toSplit from the FBR
    FBR -> at(rank + 1) -> erase(toSplit);

    return true;
}

void MBSAllocator::deallocate(AllocInfo* alloc)
{
    //check to make sure it is a MBSMeshAllocInfo->->->                        
    if (NULL == dynamic_cast<MBSMeshAllocInfo*>(alloc)) {
        schedout.fatal(CALL_INFO, 1, "MBS allocator can only deallocate instances of MBSMeshAllocInfo");
    } else {
        unallocate((MBSMeshAllocInfo*) alloc);
    }
}

void MBSAllocator::unallocate(MBSMeshAllocInfo* info){
    //add all blocks back into the FBR
    for (std::set<Block*,Block>::iterator b = info -> blocks -> begin(); b != info -> blocks -> end(); b++){
        int rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (*b) -> size()));
        FBR -> at(rank) -> insert(*b);
    }
    //for each block see if its parent is all free
    for (std::set<Block*,Block>::iterator b = info -> blocks -> begin(); b != info -> blocks -> end(); b++){
        mergeBlock((*b) -> parent);
    }
    //delete info;
}

void MBSAllocator::mergeBlock(Block* p){
    if (NULL == p) {
        return;
    }

    //make sure p isn't in FBR
    int rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), p -> size()));
    if (FBR -> at(rank) -> count(p) == 1) {
        return;
    }

    //see if children are in the FBR
    for (std::set<Block*, Block>::iterator child = p -> children -> begin(); child != p -> children -> end(); child++){
        rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (*child) -> size()));
        if (0 == FBR -> at(rank) -> count(*child)) {
            return;
        }
    }
    //by this point in the code they all are
    for (std::set<Block*, Block>::iterator child = p -> children -> begin(); child != p -> children -> end(); child++){
        rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (*child) -> size()));
        FBR -> at(rank) -> erase(*child);
    }
    rank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), p -> size()));
    FBR -> at(rank) -> insert(p);
    //recurse!
    mergeBlock(p -> parent);
}

void MBSAllocator::printRBR(std::map<int,int>* RBR)
{
    for (std::map<int,int>::iterator key = RBR -> begin(); key != RBR -> end(); key++) {
        schedout.debug(CALL_INFO, 7, 0, "Rank %d has %d requested blocks\n", key -> first, key -> second);
    }
}

void MBSAllocator::printFBR(std::string msg)
{
    schedout.debug(CALL_INFO, 7, 0, "%s\n",msg.c_str());
    if (ordering -> size() != FBR -> size()) {
        schedout.fatal(CALL_INFO, 1, "Ordering vs FBR size mismatch");
    }
    for (int i = 0;i < (int)ordering -> size(); i++) { 
        schedout.debug(CALL_INFO, 7, 0, "Rank: %d for size %d\n", i, ordering -> at(i));
        std::set<Block*, Block>::iterator it = FBR -> at(i) -> begin();
        while (it != FBR -> at(i)->end()) {
            schedout.debug(CALL_INFO, 7, 0, "  %s\n", (*it) -> toString().c_str()); 
            it++;
        }
    }
}

std::string MBSAllocator::stringFBR()
{
    std::stringstream retVal;
    if (ordering -> size() != FBR -> size()) {
        schedout.fatal(CALL_INFO, 1, "Ordering vs FBR size mismatch");
    }
    for (int i = 0;i < (int)ordering->size();i++) {
        retVal << "Rank: " << i << " for size " << ordering -> at(i) << "\n";
        std::set<Block*, Block>::iterator it = FBR -> at(i) -> begin();
        while (it != FBR -> at(i) -> end()){
            retVal << "  " << (*it) -> toString() << "\n";
            ++it;
        }
        it = FBR -> at(i) -> begin();
        while (it != FBR -> at(i) -> end()){
            retVal << "  " << (*it) -> toString(); //the Java does not have toString
            ++it;
        }
    }
    return retVal.str();
}

