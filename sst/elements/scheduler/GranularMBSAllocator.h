// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
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

#ifndef __GRANULARMBSALLOCATOR_H__
#define __GRANULARMBSALLOCATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "MBSAllocClasses.h" 
#include "MBSAllocator.h"
#include "MachineMesh.h"
#include "MBSAllocInfo.h"
#include "Job.h"
#include "misc.h"


namespace SST {
    namespace Scheduler {

        class GranularMBSAllocator : public MBSAllocator {
            public: 
                GranularMBSAllocator(MachineMesh* m, int x, int y, int z);

                string getSetupInfo(bool comment);

                GranularMBSAllocator(vector<string>* params, Machine* mach);

                void initialize(MeshLocation* dim, MeshLocation* off);

                /**
                 * The new mergeAll, with start with the given rank, and scan all the ranks below it (descending).
                 * mergeAll with make 3 passes for each rank
                 */
                bool mergeAll();

                /**
                 * returns the next block, by looking in x,y,z directions in the order depending on a given d
                 */
                Block* nextBlock(int d, Block* first);

                /**
                 * Return a block that is the given blocks partner in the x direction.
                 */
                Block* lookX(Block* b);

                Block* lookY(Block* b);

                Block* lookZ(Block* b);

                /**
                 * Attempts to perform a get operation on a set. Returns null if the block is not found
                 * This method is needed because Block.equals() is not really equals, but really similarEnough()
                 * We need to get the block from the FBR because it comes with the parent/children hierarchy.
                 */
                Block* FBRGet(Block* needle);

                Block* mergeBlocks(Block* first, Block* second);

        };

    }
}
#endif
