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
 * Abstract base class for machines
 */

#ifndef SST_SCHEDULER_MACHINE_H__
#define SST_SCHEDULER_MACHINE_H__

#include <list>
#include <string>
#include <vector>

namespace SST {
    namespace Scheduler {
        class AllocInfo;
        class Job;
        class TaskMapInfo;

        class Machine{
            public:
                Machine(int numNodes, int numCoresPerNode, double** D_matrix, int numLinks);
                virtual ~Machine();
                
                void reset();
                void allocate(TaskMapInfo* taskMapInfo);
                void deallocate(TaskMapInfo* taskMapInfo);

                inline int getNumFreeNodes() const { return numAvail; }
                inline bool isFree(int nodeNum) const { return freeNodes[nodeNum]; }
                std::vector<bool>* freeNodeList() const { return new std::vector<bool>(freeNodes); }
                std::vector<int>* getFreeNodes() const;
                std::vector<int>* getUsedNodes() const;
                double getCoolingPower() const;
                 
                virtual std::string getSetupInfo(bool comment) = 0;
                
                //returns baseline allocation used for running time estimation
                virtual AllocInfo* getBaselineAllocation(Job* job) const = 0;

                //returns the network distance between two nodes
                virtual int getNodeDistance(int node0, int node1) const = 0;
                
                //max number of nodes at a given distance - NearestAllocMapper uses this
                virtual int nodesAtDistance(int dist) const = 0;
                
                //returns the free nodes at given network distance
                virtual std::list<int>* getFreeAtDistance(int center, int distance) const = 0;

                //finds the communication route between node0 and node1 for the given weight of commWeight
                //@return The link indices used in the route
                virtual std::list<int>* getRoute(int node0, int node1, double commWeight) const = 0;
                
                double** D_matrix;
                
                const int numNodes;          //total number of nodes
                const int coresPerNode;

            private:
                int numAvail;                //number of available nodes
                std::vector<bool> freeNodes;  //whether each node is free
                std::vector<double> traffic;  //traffic on network links
        };
    }
}
#endif

