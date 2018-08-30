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

/*class to implement an energy-efficient allocation algorithm
 * that uses the glpk LP solver to find the most efficient allocation.
 * A hybrid allocator is also available that takes both
 * energy and performance into account; this is found in
 * NearestAllocator.cc
*/


#ifndef SST_SCHEDULER_ENERGYALLOCATOR_H__
#define SST_SCHEDULER_ENERGYALLOCATOR_H__

#include <vector>
#include <string>

#include "Allocator.h"

namespace SST {
    namespace Scheduler {
        class Job;
        class Machine;

        class EnergyAllocator : public Allocator {

            private:
                std::string configName;

            public:
                EnergyAllocator(std::vector<std::string>* params, const Machine & mach);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* job);

                AllocInfo* allocate(Job* job, std::vector<int>* available); 
        };

    }
}
#endif
