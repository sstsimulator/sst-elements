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

/*
 * Classes representing information about an allocation for Mesh
 * Machines
 */

#ifndef SST_SCHEDULER_MESHALLOCINFO_H__
#define SST_SCHEDULER_MESHALLOCINFO_H__

#include <vector>
#include <string>

#include "AllocInfo.h"
#include "MeshMachine.h"

namespace SST {
    namespace Scheduler {

        class Job;

        class MeshAllocInfo : public AllocInfo {
            public:
                std::vector<MeshLocation*>* nodes;
                MeshAllocInfo(Job* j, const Machine & mach);
                ~MeshAllocInfo();

                std::string getNodeList(Machine* m);
                
                //baseline allocation: minimum-volume rectangular prism that fits into the machine
                static AllocInfo* getBaselineAllocation(const MeshMachine & mach, Job* job);
        };

    }
}

#endif
