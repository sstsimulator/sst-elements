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
 * create complete blocks, and make sure the "root" blocks are in the FBR.
 */

#ifndef SST_SCHEDULER__OCTETMBSALLOCATOR_H__
#define SST_SCHEDULER__OCTETMBSALLOCATOR_H__

#include <set>
#include <string>
#include <vector>

#include "MBSAllocator.h"

namespace SST {
    namespace Scheduler {
        class MeshMachine;
        class Machine;
        class Block;
        class MeshLocation;


        class OctetMBSAllocator : public MBSAllocator {
            public:

                OctetMBSAllocator(MeshMachine* m, int x, int y, int z);
                OctetMBSAllocator(std::vector<std::string>* params, Machine* m);

                std::string getSetupInfo(bool comment) const;

                static OctetMBSAllocator Make(std::vector<std::string>* params, MeshMachine* mach);

                void initialize(MeshLocation* dim, MeshLocation* off);

                std::set<Block*, Block>* splitBlock(Block* b);
        };

    }
}
#endif
