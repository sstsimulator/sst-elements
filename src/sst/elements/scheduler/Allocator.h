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

#ifndef SST_SCHEDULER_ALLOCATOR_H__
#define SST_SCHEDULER_ALLOCATOR_H__

#include <cmath>
#include <vector>
#include <string>

#include "Job.h"
#include "Machine.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class MeshLocation;

        class Allocator {
            public:
                Allocator(const Machine & mach) : machine(mach){ };

                virtual ~Allocator() {}

                virtual std::string getSetupInfo(bool comment) const = 0;

                bool canAllocate(const Job & j)
                {  
                    return (machine.getNumFreeNodes() >= ceil((float) j.getProcsNeeded() / machine.coresPerNode ));
                }
                bool canAllocate(const Job & j, std::vector<MeshLocation*>* available)
                {  
                    return (available -> size() >= (unsigned int) ceil((float) j.getProcsNeeded() / machine.coresPerNode));
                }

                //allocates job if possible
                //returns information on the allocation or NULL if it wasn't possible
                //(doesn't make allocation; merely returns info on possible allocation)
                virtual AllocInfo* allocate(Job* job) = 0;

                //in case Allocator wants to know when a job is deallocated
                //added for MBS, which wants to update its data structures
                virtual void deallocate(AllocInfo* aInfo) { }

                //called at end of simulation so allocator can report statistics
                virtual void done() { }

            protected:
                const Machine & machine;

        };

    }
}

#endif

