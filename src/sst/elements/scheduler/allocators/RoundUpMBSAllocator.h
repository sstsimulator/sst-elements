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
 * create complete blocks, and make sure the "root" blocks are in the FBR.
 */

#ifndef __ROUNDUPMBSALLOCATOR_H__
#define __ROUNDUPMBSALLOCATOR_H__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <vector>
#include <string>
#include <map>
#pragma clang diagnostic pop

#include "GranularMBSAllocator.h" 

namespace SST {
    namespace Scheduler {
        class StencilMachine;
        class Job;
        class MBSMeshAllocInfo;
        class Block;

        /**
         * The Rounding Up MBS Allocator will do the same GranularMBS style allocation, except that
         * instead of preserving larger blocks like all of the other MBS algorithms, one
         * will default to breaking up a large block, by rounding up the request size.
         */

        class RoundUpMBSAllocator : public GranularMBSAllocator {
            public:

                std::string getSetupInfo(bool comment) const;

                RoundUpMBSAllocator(std::vector<std::string>* params, StencilMachine* mach);

                MBSMeshAllocInfo* allocate(Job* job);

                /**
                 * is rounding up part.  method answers the question, should the
                 * job be allocated ideally in system, what size block will it get.
                 * 
                 * @param procs the number of processors we are trying to allocate
                 * @return an RBR which represents the ideal request for system.
                 */
                std::map<int,int>* generateIdealRequest(int procs);

                /**
                 * method does the work of processing a request, and fitting it
                 * in to the available blocks.
                 * @param RBR the request to to allocate
                 * @param job the job the request is for
                 * @return what you need to send the Mesh object, to actually allocate the processors
                 */
            private:
                StencilMachine* mMachine;
                MBSMeshAllocInfo* processRequest(std::map<int,int>* RBR, Job* job);

                /**
                 * Controller method for whether or not to partially allocate, or fully allocate a block
                 * @param block the block in question
                 * @param procsLeft how many processors we have left to allocate
                 * @return a list of blocks (may be size 1) which we will allocate.
                 */
                std::vector<Block*>* makeBlockAllocation(Block* block, int procsLeft) ;

                /**
                 * Perform the allocation, in terms of adding the MeshLocations contained in each block
                 * to the AllocInfo
                 * @param retVal the AllocInfo block we are adding on to.
                 * @param blocks The blocks we are going to allocate
                 * @param allocated the number of processors allocated so far
                 * @return the AllocInfo with the given blocks added in.
                 */
                MBSMeshAllocInfo* allocateBlocks(MBSMeshAllocInfo* retVal, std::vector<Block*>* blocks, int allocated);

                /**
                 * Turns a request for value key size blocks, into a number of the next highest block size.
                 * @param key corresponds to indices in ordering, and FBR
                 * @param value the number of blocks of rank key
                 * @return a new RBR which the reduced blocks
                 */
                std::map<int,int>* reduceRequest(int key, int value);

                /**
                 * Figure out how many processors we have allocate thus far
                 * @param info the allocation we want to tally
                 * @return the number of processors
                 */
                int tallyAllocated(MBSMeshAllocInfo* info);

                /**
                 * method will allocate only a portion of the given block by breaking it up recursively.
                 * method is used to handle the remainders of the rounded up requests.  method
                 * modifies the FBR.
                 * @param block The block from which to allocate a smaller number of processors
                 * @param procs The number of processors we are trying to allocate
                 * @return which blocks, out of those remaining from the given block's break up, are we going to use?
                 */
            protected:
                std::vector<Block*>* partiallyAllocate(Block* block, int procs);
        };


    }
}
#endif
