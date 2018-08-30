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

#ifndef SST_SCHEDULER_TASKMAPINFO_H__
#define SST_SCHEDULER_TASKMAPINFO_H__

#include <map>
#include <vector>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class Machine;
        class TaskCommInfo;

        class TaskMapInfo {

            public:
                AllocInfo* allocInfo;
                Job* job;

                TaskMapInfo(AllocInfo* ai, const Machine & machine);

                ~TaskMapInfo();

                //does not do the mapping; only assigns the given task
                //does not check if given node or task is already mapped
                void insert(int taskInd, long int nodeInd);

                //returns average per-neighbor hop distance
                //assumes no migration, i.e., it calculates the hop distance only once
                double getAvgHopDist();

                //returns the links used by this job along with the traffic weights
                //assumes static route
                std::map<unsigned int, double> getTraffic();
                //return max traffic in a link, created only by current job
                double getMaxJobCongestion();
                //return the sum of congestion on all links
                double getHopBytes();

                std::map<int, std::vector<int> > nodeToTasks; //NetworkSim: holds node IDs and tasks mapped to each node

            private:
                TaskCommInfo* taskCommInfo;
                int mappedCount;
                double avgHopDist;
                const Machine & machine;
                std::map<long int, int> numAvailCores;
                std::vector<long int> taskToNode;
                std::map<unsigned int, double> traffic;
                double maxCongestion;        //max congestion within the corresponding job
                double hopBytes;
                void updateNetworkMetrics(); //calculates traffic and hop distance info
        };
    }
}
#endif /* SST_SCHEDULER_TASKMAPINFO_H__ */

