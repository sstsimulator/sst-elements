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
#include "RoundUpMBSAllocator.h"

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

/**
 * The Rounding Up MBS Allocator will do the same GranularMBS style allocation, except that
 * instead of preserving larger blocks like all of the other MBS algorithms, one
 * will default to breaking up a large block, by rounding up the request size->
 * 
 * @author Peter Walker
 * 
 */

using namespace SST::Scheduler;
using namespace std;

std::string RoundUpMBSAllocator::getSetupInfo(bool comment) const
{
    string com;
    if (comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    return com + "Multiple Buddy Strategy (MBS) Allocator using Granular divisions";
}

RoundUpMBSAllocator::RoundUpMBSAllocator(vector<string>* params, StencilMachine* mach) : GranularMBSAllocator(params, mach)
{
    schedout.init("", 8, ~0, Output::STDOUT);
    //constructor is the same as GranularMBSAllocator
}

MBSMeshAllocInfo* RoundUpMBSAllocator::allocate(Job* job)
{
    //if (DEBUG) printf("Allocating %s\n",job -> toString().c_str());
    schedout.debug(CALL_INFO, 1, 0, "Allocating %s\n",job -> toString().c_str());

    //a map of dimensions to numbers
    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);
    map<int,int>* RBR = generateIdealRequest(nodesNeeded);
    //if (DEBUG) printRBR(RBR);
    printRBR(RBR);

    //process the request, split stuff, and allocate stuff
    MBSMeshAllocInfo* retVal = processRequest(RBR, job);

    printFBR("all done");
    schedout.debug(CALL_INFO, 7, 0, "%s\n", retVal -> toString().c_str());
    //if (DEBUG) {
    //    printFBR("all done");
    //    printf("%s\n", retVal -> toString().c_str());
    //}
    return retVal;
}

/**
 * is rounding up part.  method answers the question, should the
 * job be allocated ideally in system, what size block will it get.
 * 
 * @param procs the number of processors we are trying to allocate
 * @return an RBR which represents the ideal request for system.
 */
map<int,int>* RoundUpMBSAllocator::generateIdealRequest(int procs)
{
    //make return value
    map<int,int>* retVal = new map<int,int>();

    //construct ideal request (round up)
    int roundedIndex = 0;
    while (roundedIndex <  (int)ordering -> size() && ordering -> at(roundedIndex) < procs) {
        roundedIndex++;
    };
    if (ordering -> at(roundedIndex) < procs){
        retVal -> insert(pair<int,int>(roundedIndex, 
                                       (int)ceil(((double) procs)/((double)ordering -> at(roundedIndex)))));
    } else {
        retVal -> insert(pair<int,int>(roundedIndex, 1));
    }

    return retVal;
}

/**
 * method does the work of processing a request, and fitting it
 * in to the available blocks->
 * @param RBR the request to to allocate
 * @param job the job the request is for
 * @return what you need to send the Mesh object, to actually allocate the processors
 */
MBSMeshAllocInfo* RoundUpMBSAllocator::processRequest(map<int,int>* RBR, Job* job)
{
    //construct what we will eventually return
    //TreeMap<int,int> retVal = new TreeMap<int,int>();
    MBSMeshAllocInfo* retVal = new MBSMeshAllocInfo(job, *mMachine);

    int procs = 0;

    //process all of the RBR
    while (RBR -> size() > 0) {
        //get the largest key entry in the RBR
        int key = RBR -> end() -> first;
        int value = RBR -> find(key) -> second;

        //if (DEBUG) printFBR("  trying to fit size blocks");
        printFBR("  trying to fit size blocks");
        schedout.debug(CALL_INFO, 7, 0, "RBR size %d\n", (int)RBR -> size());

        //if there exists blocks in FBR to support, add it
        if (key >= (int)FBR -> size())
            schedout.fatal(CALL_INFO, 1, "key in RBR does not correspond to FBR");

        int numberAvailable = FBR -> at(key) -> size();

        //either make one available, by splitting a larger block
        //or reduces the block sizes requested
        if (numberAvailable == 0) {
            //if (DEBUG) printf("Didn't find any available blocks of rank %d\n",key);
            schedout.debug(CALL_INFO, 7, 0, "Didn't find any available blocks of rank %d\n",key);
            //try splitting a larger block
            //but make sure that the larger block isn't already accounted for
            if (splitLarger(key)) {
                numberAvailable = FBR -> at(key) -> size();

                //if (DEBUG)  {
                //    printf("There are now %dblocks of rank %d\n", numberAvailable, key);
                //}
                schedout.debug(CALL_INFO, 7, 0, "There are now %dblocks of rank %d\n", numberAvailable, key);
            } else {
                //reduce request
                map<int,int>* reduced = reduceRequest(key, value);
                //merge reduced back into RBR
                for (map<int,int>::iterator entry = reduced -> begin(); entry != reduced -> end(); entry++) {
                    if (RBR -> count((entry -> first)) == 0) {
                        RBR -> insert(pair<int,int>(entry -> first, entry -> second));
                    } else {
                        RBR -> find(entry -> first) -> second += entry -> second;
                    }
                }
                //we've just eliminated all current blocks (because they have been reduced)
                value = 0;
                RBR -> erase(key);
                RBR -> insert(pair<int,int>(key, 0)); //also zero out the RBR
            }
        }
        //since splitting may not have worked, we check again
        if (numberAvailable > 0) {
            //add all you can
            int toAssign = 0;
            if (numberAvailable < value){
                toAssign = numberAvailable;
            } else {
                toAssign = value;
            }
            int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);
            //add if necessary
            for (int assigned = 0; assigned < toAssign && value > 0; assigned++)
            {
                vector<Block*>* toAllocate = makeBlockAllocation(*(FBR -> at(key) -> begin()), nodesNeeded - procs);

                retVal = allocateBlocks(retVal, toAllocate, procs);

                procs = tallyAllocated(retVal);

                //update our value
                value--;

                //end the entire loop if we have not allocated enough blocks
                if (procs >= nodesNeeded){
                    RBR -> clear();
                    value = 0;
                }
            }
            RBR -> erase(key);
            RBR -> insert(pair<int,int>(key, value));
        }

        //remove key from RBR, since we should be all done with it
        //make progress in the loop
        if (0 == value) RBR -> erase(key);
    }

    return retVal;
}

/**
 * method will allocate only a portion of the given block by breaking it up recursively->
 * method is used to handle the remainders of the rounded up requests->  method
 * modifies the FBR->
 * @param block The block from which to allocate a smaller number of processors
 * @param procs The number of processors we are trying to allocate
 * @return which blocks, out of those remaining from the given block's break up, are we going to use?
 */
vector<Block*>* RoundUpMBSAllocator::partiallyAllocate(Block* block, int procs)
{
    vector<Block*>* retVal = new vector<Block*>();

    //keep track of the remaining procs after 
    int procsLeft = procs;

    //remove the given block from the FBR
    int blockRank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (block -> size())));
    FBR -> at(blockRank) -> erase(block);
    //add its children to the FBR
    set<Block*, Block>::iterator children = block -> children -> begin();
    while (children != block -> children -> end()){
        Block* next = *children;
        children++;
        int nextRank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), next -> size()));
        FBR -> at(nextRank) -> insert(next);
    }

    //use children of the given block
    children = block -> children -> begin();
    while (children != block -> children -> end() && procsLeft > 0) {
        Block* child = *children;
        children++;
        if (child -> size() > procsLeft){
            vector<Block*>* newBlocks = partiallyAllocate(child, procsLeft);
            //iterate over newBlocks adding to retVal
            vector<Block*>::iterator it = newBlocks -> begin();
            while (it != newBlocks -> end()) {
                Block* next = *it;
                it++;
                retVal -> push_back(next);
                //decreasing procsLeft
                procsLeft  -= next-> size();
            }
        } else {
            retVal -> push_back(child); //TODO do I need to remove block from FBR here?
            procsLeft  -= child-> size();
        }
    }
    //if a smaller size of child is needed, recurse!

    return retVal;
}

/**
 * Controller method for whether or not to partially allocate, or fully allocate a block
 * @param block the block in question
 * @param procsLeft how many processors we have left to allocate
 * @return a list of blocks (may be size 1) which we will allocate->
 */
vector<Block*>* RoundUpMBSAllocator::makeBlockAllocation(Block* block, int procsLeft) {
    if (block -> size() > procsLeft){
        return partiallyAllocate(block, procsLeft);
    }
    vector<Block*>* retVal = new vector<Block*>();
    retVal -> push_back(block);
    return retVal;
}

/**
 * Perform the allocation, in terms of adding the MeshLocations contained in each block
 * to the AllocInfo
 * @param retVal the AllocInfo block we are adding on to->
 * @param blocks The blocks we are going to allocate
 * @param allocated the number of processors allocated so far
 * @return the AllocInfo with the given blocks added in->
 */
MBSMeshAllocInfo* RoundUpMBSAllocator::allocateBlocks(MBSMeshAllocInfo* retVal, vector<Block*>* blocks, int allocated)
{
    for (vector<Block*>::iterator block = blocks -> begin(); block != blocks -> end(); block++) {
        int blockRank = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), (*block) -> size()));

        retVal -> blocks -> insert(*block);
        FBR -> at(blockRank) -> erase(*block);

        set<MeshLocation*, MeshLocation>* blockprocs = (*block) -> processors();
        set<MeshLocation*, MeshLocation>::iterator procs = blockprocs -> begin();
        for (int i = allocated;procs != blockprocs -> end(); i++) {
            retVal -> nodes -> at(i) = *procs;
            ++procs;
            allocated++;
        }
    }
    return retVal;
}

/**
 * Turns a request for value key size blocks, into a number of the next highest block size->
 * @param key corresponds to indices in ordering, and FBR
 * @param value the number of blocks of rank key
 * @return a new RBR which the reduced blocks
 */
map<int,int>* RoundUpMBSAllocator::reduceRequest(int key, int value)
{
    if (key <= 0) schedout.fatal(CALL_INFO, 1, "can't factor a requested block of rank 0");

    map<int,int>* retVal = new map<int,int>();
    double size = ordering -> at(key);
    double smallerSize = ordering -> at(key-1);

    //figure out how many smallerSizes go into the number of a regular sizes given
    int numberOfSmaller = (int) (((double)size * value) / smallerSize);
    if (retVal -> count(key - 1) == 0) {
        retVal->insert(pair<int,int>(key-1,numberOfSmaller));
    } else {
        retVal -> find(key - 1) -> second += numberOfSmaller;
    }

    if (0 == retVal -> count(key - 1)) {
        retVal -> insert(pair<int,int>(key - 1,numberOfSmaller));
    } else {
        retVal -> find(key - 1) -> second += numberOfSmaller;
    }

    //check if there is extra
    int remainder = (int) (size - (numberOfSmaller*smallerSize));
    if (remainder > 0) {
        int smallerIndex = distance(ordering -> begin(), find(ordering -> begin(), ordering -> end(), remainder));
        if (smallerIndex == -1) {
            schedout.fatal(CALL_INFO, 1, "The remainder is not a block size!");
        }
        if (0 == retVal -> count(smallerIndex)) {
            retVal -> insert(pair<int,int>(smallerIndex,1));
        } else {
            retVal -> find(smallerIndex) -> second += 1;
        }
    }

    return retVal;
}

/**
 * Figure out how many processors we have allocate thus far
 * @param info the allocation we want to tally
 * @return the number of processors
 */
int RoundUpMBSAllocator::tallyAllocated(MBSMeshAllocInfo* info)
{
    int procs = 0;
    set<Block*, Block>::iterator it = info -> blocks -> begin();
    while (it != info -> blocks -> end()) {
        procs += (*it) -> size();
        ++it;
    }
    return procs;
}
