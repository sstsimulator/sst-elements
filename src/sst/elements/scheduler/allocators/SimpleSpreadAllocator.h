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

/*
 * Allocator for SimpleMachine- no representation of locality so just
 * keep track of the number of free processors
 */

#ifndef SST_SCHEDULER_SIMPLESPREADALLOCATOR_H__
#define SST_SCHEDULER_SIMPLESPREADALLOCATOR_H__

#include "DragonflyAllocator.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class DragonFlyMachine;
        class Job;

        class SimpleSpreadAllocator : public DragonflyAllocator {
            public:

                SimpleSpreadAllocator(const DragonflyMachine & mach);

                ~SimpleSpreadAllocator() { }

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* j);
            private:
                int nextNodeId(int curNode);
        };

    }
}
#endif // SST_SCHEDULER_SIMPLESPREADALLOCATOR_H__
