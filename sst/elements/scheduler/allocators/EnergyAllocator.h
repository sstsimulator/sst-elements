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

#include "sst/core/serialization.h"

#include "Allocator.h"


namespace SST {
    namespace Scheduler {
        class Job;
        class Machine;
        class MeshMachine;

        class EnergyAllocator : public Allocator {

            private:
                std::string configName;

            public:
                EnergyAllocator(std::vector<std::string>* params, MeshMachine* mach);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                AllocInfo* allocate(Job* job);

                AllocInfo* allocate(Job* job, std::vector<MeshLocation*>* available); 
        };

    }
}
#endif
