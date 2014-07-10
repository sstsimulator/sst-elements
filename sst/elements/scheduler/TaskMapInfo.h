// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_SCHEDULER_TASKMAPINFO_H__
#define SST_SCHEDULER_TASKMAPINFO_H__

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class MeshMachine;
        class TaskCommInfo;

        class TaskMapInfo {

            private:
                TaskCommInfo* taskCommInfo;
                int size;
                int mappedCount;
                double avgHopDist;

            public:
                AllocInfo* allocInfo;
                Job* job;
                int* taskToNode;

                TaskMapInfo(AllocInfo* ai);

                ~TaskMapInfo();

                //does not do the mapping; only assigns the given task
                //does not check if given node or task is already mapped
                void insert(int taskInd, int nodeInd);

                //returns average per-neighbor hop distance
                //assumes no migration, i.e., it calculates the hop distance only once
                double getAvgHopDist(const MeshMachine & machine);
        };
    }
}
#endif /* SST_SCHEDULER_TASKMAPINFO_H__ */
