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

#ifndef SST_SCHEDULER_MBSALLOCATOR_H__
#define SST_SCHEDULER_MBSALLOCATOR_H__

#include <vector>
#include <string>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <map>
#pragma clang diagnostic pop

#include "Allocator.h" 
#include "MBSAllocInfo.h" //necessary due to virtual class

namespace SST {
    namespace Scheduler {
        class Block;
        class MBSMeshAllocInfo;
        class MeshLocation;
        class Job;
        class Machine;
        class StencilMachine;

        class MBSAllocator : public Allocator {

            protected:
                std::vector<std::set<Block*, Block>*>* FBR;
                std::vector<int>* ordering;

                //We know it must be a mesh, so make it one so we can access the goods.
                StencilMachine *mMachine;

            public:
                MBSAllocator(Machine * mach);
                MBSAllocator(StencilMachine* m, int x, int y, int z);

                MBSAllocator(std::vector<std::string>* params, Machine *mach);

                std::string getSetupInfo(bool comment) const;

                std::string getParamHelp();

                /**
                 * Initialize will fill in the FBR with z number of blocks (1 for
                 * each layer) that fit into the given x,y dimensions.  It is
                 * assumed that those dimensions are non-zero.
                 */
                void initialize(MeshLocation* dim, MeshLocation* off);

                /**
                 * Creates a rank in both the FBR, and in the ordering.
                 * If a rank already exists, it does not create a new rank,
                 * it just returns the one already there
                 */
                int createRank(int size);

                /**
                 *  Essentially this will reinitialize a block, except add the
                 *  children to the b.children, then recurse
                 */
                void createChildren(Block* b);

                std::set<Block*, Block>* splitBlock (Block* b) ;

                MBSMeshAllocInfo* allocate(Job* job);

                /**
                 * Calculates the RBR, which is a map of ranks to number of blocks at that rank
                 */
                std::map<int,int>* factorRequest(Job* j);

                /**
                 * Breaks up a request for a block with a given rank into smaller request if able.
                 */
                void splitRequest(std::map<int,int>* RBR, int rank);

                /**
                 * Determines whether a split up of a possible larger block was
                 * successful.  It begins looking at one larger than rank.
                 */
                bool splitLarger(int rank);

                void deallocate(AllocInfo* alloc);

                void unallocate(MBSMeshAllocInfo* info);

                void mergeBlock(Block* p);

                void printRBR(std::map<int,int>* RBR);

                void printFBR(std::string msg);

                std::string stringFBR();

        };

    }
}
#endif
