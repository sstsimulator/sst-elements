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

/*functions that implement the actual calls to glpk for energy-aware allocators*/

#ifndef SST_SCHEDULER_ENERGYALLOCCLASSES_H__
#define SST_SCHEDULER_ENERGYALLOCCLASSES_H__

#include "sst_config.h"
#include <vector>

namespace SST {
    namespace Scheduler {
        class Machine;

        namespace EnergyHelpers {
            void roundallocarray(double * x, int processors, int numneeded, int* newx);

            void hybridalloc(int* oldx, int* roundedalloc, int processors, int requiredprocessors, const Machine & machine);

            std::vector<int>* getEnergyNodes(std::vector<int>* available, int numProcs, const Machine & machine);
        }
    }
}
#endif
