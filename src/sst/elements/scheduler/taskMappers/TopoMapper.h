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

// TopoMapper code is partially taken from LibTopoMap version 0.9b
// http://spcl.inf.ethz.ch/Research/Parallel_Programming/MPI_Topologies/LibTopoMap/

#ifndef SST_SCHEDULER_TOPOMAPPER_H_
#define SST_SCHEDULER_TOPOMAPPER_H_

#include "TaskMapper.h"

#include <vector>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;
        class TaskMapInfo;

        class TopoMapper : public TaskMapper {
            public:

                enum AlgorithmType{
                    R_C_M = 0, //reverse Cuthill Mckee algorithm
                    RECURSIVE = 1,
                    //GREEDY = 2, //not implemented
                };

                TopoMapper(const Machine & mach, AlgorithmType mode);
                ~TopoMapper();

                std::string getSetupInfo(bool comment) const;

                TaskMapInfo* mapTasks(AllocInfo* allocInfo);

            private:

                AlgorithmType algorithmType;
                int numNodes;
                int numTasks;
                std::vector<std::vector<int> > commGraph; //adjacency list
                std::vector<std::vector<int> > commWeights; //commInfo edge weights
                std::vector<int> numCores; //number of cores in the nodes
                std::vector<std::vector<int> > phyGraph; //adjacency list of the allocated nodes
                std::vector<std::vector<int> > networkWeights; //network edge weights
                std::vector<int> mapping;

                void setup(AllocInfo* allocInfo);
                int mapRCM(std::vector<std::vector<int> > *commGraph_ref,
                           std::vector<int> *mapping_ref
                           );

                int mapRecursive(std::vector<std::vector<int> > *nodeGraph_ref,
                                 std::vector<std::vector<int> > *networkWeights_ref,
                                 std::vector<int> *numCores_ref,
                                 std::vector<std::vector<int> > *commGraph_ref,
                                 std::vector<std::vector<int> > *weights_ref,
                                 std::vector<int> *mapping_ref,
                                 std::vector<int> commGraph_map,
                                 std::vector<int> *nodeGraph_map_ref
                                 );

        };
    }
}

#endif /* SST_SCHEDULER_TOPOMAPPER_H_ */

