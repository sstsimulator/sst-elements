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

/*
 * Allocator for SimpleMachine- no representation of locality so just
 * keep track of the number of free processors
 */

#ifndef SST_SCHEDULER_RANDOMALLOCATOR_H__
#define SST_SCHEDULER_RANDOMALLOCATOR_H__

#include <string>
#include <vector>

#include "sst/core/rng/sstrng.h"

#include "Allocator.h"

namespace SST {
    namespace Scheduler {
        class Machine;
        class Job;

        class RandomAllocator : public Allocator {

            public:
                RandomAllocator(Machine* mach);
                ~RandomAllocator();

                RandomAllocator Make(std::vector<std::string*>* params);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* job);
                
            private:
                SST::RNG::SSTRandom* rng;  //random number generator

        };

    }
}
#endif

