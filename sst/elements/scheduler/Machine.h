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

/*
 * Abstract base class for machines
 */

#ifndef SST_SCHEDULER_MACHINE_H__
#define SST_SCHEDULER_MACHINE_H__

#include <string>
#include <vector>

namespace SST {
    namespace Scheduler {
        class TaskMapInfo;

        class Machine{
            public:

                Machine(long numNodes, int numCoresPerNode, double** D_matrix, unsigned int numLinks);
                virtual ~Machine();
                
                void reset();
                void allocate(TaskMapInfo* taskMapInfo);
                void deallocate(TaskMapInfo* taskMapInfo);

                long getNumFreeNodes() const { return numAvail; }
                bool isFree(int nodeNum) const { return freeNodes[nodeNum]; }
                std::vector<bool>* freeNodeList() const { return new std::vector<bool>(freeNodes); }
                std::vector<int>* getFreeNodes() const;
                std::vector<int>* getUsedNodes() const;
                double getCoolingPower() const;
                 
                virtual std::string getSetupInfo(bool comment) = 0;
                virtual unsigned int getNodeDistance(int node0, int node1) const = 0;

                //finds the communication route between node0 and node1 for the given weight of commWeight
                //@return The link indices used in the route
                virtual std::vector<unsigned int> getRoute(int node0, int node1, double commWeight) const = 0;
                
                double** D_matrix;
                
                const long numNodes;          //total number of nodes
                const int coresPerNode;

            private:
                long numAvail;                //number of available nodes
                std::vector<bool> freeNodes;  //whether each node is free
                std::vector<double> traffic;  //traffic on network links
        };
    }
}
#endif

