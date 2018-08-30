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
 * Classes representing information about an allocation
 */

#include <string>
#include <climits>

#ifndef SST_SCHEDULER_ALLOCINFO_H__
#define SST_SCHEDULER_ALLOCINFO_H__

namespace SST {
    namespace Scheduler {

        class Job;
        class Machine;

        class AllocInfo {
            public:
                Job* job;
                int* nodeIndices;

                AllocInfo(Job* job, const Machine & mach);
                AllocInfo(const AllocInfo & ai);

                virtual ~AllocInfo();

                virtual std::string getProcList();
                
                int getNodesNeeded() const { return nodesNeeded; }
                
            private:
                int nodesNeeded;
        };


    }
}
#endif
