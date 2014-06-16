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

/*functions that implement the actual calls to glpk for energy-aware allocators*/

#ifndef SST_SCHEDULER_ENERGYALLOCCLASSES_H__
#define SST_SCHEDULER_ENERGYALLOCCLASSES_H__

#include "sst_config.h"
#include <vector>

#include "MachineMesh.h"
#include "MeshAllocInfo.h"



#define ARRAY(row,col) 40*(row-1) + (col-1)

namespace SST {
    namespace Scheduler {
        class MeshLocation;
        class MachineMesh;
        namespace EnergyHelpers {
            void roundallocarray(double * x, int processors, int numneeded, int* newx);

            void hybridalloc(int* oldx, int* roundedalloc, int processors, int requiredprocessors, MachineMesh * machine);

            std::vector<MeshLocation*>* getEnergyNodes(std::vector<MeshLocation*>* available, int numProcs, MachineMesh * machine);
        }
    }
}
#endif
