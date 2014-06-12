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
 * Allocator that assigns the first available processors (according to
 * order specified when allocator is created).
 */

#ifndef SST_SCHEDULER_SORTEDFREELISTALLOCATOR_H__
#define SST_SCHEDULER_SORTEDFREELISTALLOCATOR_H__

#include <string>
#include <vector>

#include "LinearAllocator.h"

namespace SST {
    namespace Scheduler {
        class MachineMesh;
        class Machine;
        class Job;
        class AllocInfo;

        class SortedFreeListAllocator : public LinearAllocator {
            public:
                SortedFreeListAllocator(MachineMesh* m, std::string filename);

                SortedFreeListAllocator(std::vector<std::string>* params, Machine* mach);

                std::string getParamHelp()
                {
                    return "[<file>]\n\tfile: Path to file giving the curve";
                }
                std::string getSetupInfo(bool comment);

                AllocInfo* allocate(Job* job);
        };


    }
}
#endif
