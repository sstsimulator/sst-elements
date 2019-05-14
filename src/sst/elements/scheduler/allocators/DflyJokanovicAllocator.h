// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_SCHEDULER_DFLYJOKANOVICALLOCATOR_H__
#define SST_SCHEDULER_DFLYJOKANOVICALLOCATOR_H__

#include "DragonflyAllocator.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class DragonFlyMachine;
        class Job;

        class DflyJokanovicAllocator : public DragonflyAllocator {
            public:

                DflyJokanovicAllocator(const DragonflyMachine & mach);

                ~DflyJokanovicAllocator() { }

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* j);

        };

    }
}
#endif // SST_SCHEDULER_DFLYJOKANOVICALLOCATOR_H__
