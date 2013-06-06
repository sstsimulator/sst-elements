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
 * Allocator that assigns the first available processors (according to
 * order specified when allocator is created).
 */

#ifndef __SORTEDFREELISTALLOCATOR_H__
#define __SORTEDFREELISTALLOCATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "LinearAllocator.h"
#include "MachineMesh.h"
#include "Job.h"
#include "misc.h"

namespace SST {
    namespace Scheduler {

        class SortedFreeListAllocator : public LinearAllocator {
            public:
                SortedFreeListAllocator(MachineMesh* m, string filename);

                SortedFreeListAllocator(vector<string>* params, Machine* mach);

                string getParamHelp()
                {
                    return "[<file>]\n\tfile: Path to file giving the curve";
                }
                string getSetupInfo(bool comment);

                AllocInfo* allocate(Job* job);
        };


    }
}
#endif
