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
 * Allocator that uses tbe first-fit linear allocation strategy
 * (according to the order specified when the allocator is created).
 * Uses the first intervals of free processors that is big enough.  If
 * none are big enough, chooses the one that minimizes the span
 * (maximum distance along linear order between assigned processors).
 */
#ifndef SST_SCHEDULER_BESTFITALLOCATOR_H__
#define SST_SCHEDULER_BESTFITALLOCATOR_H__

#include "LinearAllocator.h"

namespace SST {
    namespace Scheduler {

        class Machine;
        class Job;
        class AllocInfo;

        class BestFitAllocator : public LinearAllocator {
            public:

                BestFitAllocator(std::vector<std::string>* params, Machine* mach) ;

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* job) ;
        };
    }
}
#endif
