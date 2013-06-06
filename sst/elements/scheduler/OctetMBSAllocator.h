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

#ifndef __OCTETMBSALLOCATOR_H__
#define __OCTETMBSALLOCATOR_H__

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

        class OctetMBSAllocator : public MBSAllocator {
            public:

                OctetMBSAllocator(MachineMesh* m, int x, int y, int z);
                OctetMBSAllocator(vector<string>* params, Machine* m);

                string getSetupInfo(bool comment);

                static OctetMBSAllocator Make(vector<string>* params, MachineMesh* mach);

                void initialize(MeshLocation* dim, MeshLocation* off);

                set<Block*, Block>* splitBlock(Block* b);

        };

    }
}
#endif
