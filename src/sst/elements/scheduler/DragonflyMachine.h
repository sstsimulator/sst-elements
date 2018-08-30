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

#ifndef SST_SCHEDULER_DRAGONFLYMACHINE_H__
#define SST_SCHEDULER_DRAGONFLYMACHINE_H__

#include <list>
#include <map>
#include <string>
#include <vector>

#include "AllocInfo.h"
#include "Machine.h"

namespace SST {
    namespace Scheduler {

        class DragonflyMachine : public Machine {
                
            public:
                enum localTopo{
                    ALLTOALL,
                };
                
                enum globalTopo{
                    CIRCULANT,
                    ABSOLUTE,
                    RELATIVE,
                };
                
                DragonflyMachine(int routersPerGroup, int portsPerRouter, int opticalsPerRouter,
                    int nodesPerRouter, int coresPerNode, localTopo lt, globalTopo gt,
                    double** D_matrix = NULL);
                ~DragonflyMachine() { };
				
				AllocInfo* getBaselineAllocation(Job* job) const;

                std::string getSetupInfo(bool comment);
                
                //returns the network distance of the given nodes
                int getNodeDistance(int node0, int node1) const;
                
                //returns the free nodes at the given Distance
                std::list<int>* getFreeAtDistance(int center, int distance) const;
                
                //max number of nodes at the given distance - NearestAllocMapper uses this
                int nodesAtDistance(int dist) const;
                
                //DragonflyMachine default routing is shortest path
                //@return list of link indices
                std::list<int>* getRoute(int node0, int node1, double commWeight) const;
                
                const localTopo ltopo;
                const globalTopo gtopo;
                const int routersPerGroup;
                const int nodesPerRouter;
                const int portsPerRouter;
                const int opticalsPerRouter;
                const int numGroups;
                const int numNodes;
                const int numRouters;
                const int numLinks;

                inline int routerOf(int nodeID) const { return nodeID / nodesPerRouter; }
                inline int groupOf(int routerID) const { return routerID / routersPerGroup; }
                inline int localIdOf(int routerID) const { return routerID % routersPerGroup; }
                
            private:                
                //constructor helpers
                int getNumNodes(int opticalsPerRouter, int routersPerGroup, int nodesPerRouter) const
                {
                    int numGroups = getNumGroups(routersPerGroup, opticalsPerRouter);
                    return (numGroups * routersPerGroup * nodesPerRouter);
                }
                int getNumLinks(int portsPerRouter, int nodesPerRouter, int routersPerGroup, int opticalsPerRouter) const
                {
                    int numRouters = getNumRouters(routersPerGroup, opticalsPerRouter);
                    return ((portsPerRouter + nodesPerRouter) * numRouters / 2);
                }
                int getNumRouters(int routersPerGroup, int opticalsPerRouter) const
                {
                    int numGroups = getNumGroups(routersPerGroup, opticalsPerRouter);
                    return (routersPerGroup * numGroups);
                }
                int getNumGroups(int routersPerGroup, int opticalsPerRouter) const
                {
                    return (routersPerGroup * opticalsPerRouter + 1);
                }

                //router graph: routers[routerID] = map<targetRouterID, linkInd>
                std::vector<std::map<int,int> > routers;
                std::vector<int> nodesAtDistances;                
        };
    }
}
#endif

