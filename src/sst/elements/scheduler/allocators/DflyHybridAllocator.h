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

// This is the Level - Spread policy for dragonfly systems which spreads
// jobs within the smallest network level that a given job can fit in at the time
// of its allocation. If a job fits within the available nodes that are connected
// to a single router, it selects the router with the largest number of idle nodes
// and allocates the job there. If a job cannot fit within a single router but fits
// within the available nodes in a single group, it selects the most idle group
// and allocates the job there. To further reduce load imbalance on local links in
// this group, it selects nodes connected to different routers in a round - robin
// manner. If a job cannot fit within a single group, it spreads the job throughout
// the entire network, where it selects nodes in different groups in a round - robin
// manner.
// This policy is published in:
// Yijia Zhang, Ozan Tuncer, Fulya Kaplan, Katzalin Olcoz, Vitus J Leung, 
// Ayse K Coskun. Level-Spread: A New Job Allocation Policy for Dragonfly Networks. 
// IEEE International Parallel & Distributed Processing Symposium (IPDPS), 2018.

#ifndef SST_SCHEDULER_DFLYHYBRIDALLOCATOR_H__
#define SST_SCHEDULER_DFLYHYBRIDALLOCATOR_H__

#include "DragonflyAllocator.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class DragonFlyMachine;
        class Job;

        class DflyHybridAllocator : public DragonflyAllocator {
            public:

                DflyHybridAllocator(const DragonflyMachine & mach);

                ~DflyHybridAllocator() { }

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* j);

        };

    }
}
#endif // SST_SCHEDULER_DFLYHYBRIDALLOCATOR_H__
