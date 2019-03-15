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

#ifndef SST_SCHEDULER__OCTETMBSALLOCATOR_H__
#define SST_SCHEDULER__OCTETMBSALLOCATOR_H__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <set>
#include <string>
#include <vector>
#pragma clang diagnostic pop

#include "MBSAllocator.h"

namespace SST {
    namespace Scheduler {
        class StencilMachine;
        class Machine;
        class Block;
        class MeshLocation;

        class OctetMBSAllocator : public MBSAllocator {
            public:

                OctetMBSAllocator(StencilMachine* m, int x, int y, int z);
                OctetMBSAllocator(std::vector<std::string>* params, Machine* m);

                std::string getSetupInfo(bool comment) const;

                static OctetMBSAllocator Make(std::vector<std::string>* params, StencilMachine* mach);

                void initialize(MeshLocation* dim, MeshLocation* off);

                std::set<Block*, Block>* splitBlock(Block* b);
        };

    }
}
#endif
