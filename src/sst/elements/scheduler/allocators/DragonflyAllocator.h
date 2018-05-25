// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_SCHEDULER__DRAGONFLYALLOCATOR_H__
#define SST_SCHEDULER__DRAGONFLYALLOCATOR_H__

#include "Allocator.h"
#include "DragonflyMachine.h"
#include "output.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;

        class DragonflyAllocator : public Allocator {
            public:
                DragonflyAllocator(const DragonflyMachine & mach)
                  : dMach(mach), Allocator(mach) { }
                ~DragonflyAllocator() { }
                virtual std::string getSetupInfo(bool comment) const = 0;
                virtual AllocInfo* allocate(Job* job) = 0;

                const DragonflyMachine & dMach;
        };

    }
}
#endif // SST_SCHEDULER__DRAGONFLYALLOCATOR_H__
