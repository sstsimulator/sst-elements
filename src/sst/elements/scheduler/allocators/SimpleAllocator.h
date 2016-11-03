// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
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

#ifndef SST_SCHEDULER_SIMPLEALLOCATOR_H__
#define SST_SCHEDULER_SIMPLEALLOCATOR_H__

#include "Allocator.h"

namespace SST {
    namespace Scheduler {

        class Machine;
        class Job;

        class SimpleAllocator : public Allocator {
            public:

                SimpleAllocator(Machine* m);

                ~SimpleAllocator() {}

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* j);  //allocates j if possible
                //returns information on the allocation or NULL if it wasn't possible
                //(doesn't make allocation; merely returns info on possible allocation)
        };

    }
}
#endif
