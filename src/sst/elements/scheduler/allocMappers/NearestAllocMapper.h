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

#ifndef SST_SCHEDULER_NEARESTALLOCMAPPER_H_
#define SST_SCHEDULER_NEARESTALLOCMAPPER_H_

#include "AllocMapper.h"
#include "FibonacciHeap.h"

#include <climits>
#include <list>
#include <map>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class StencilMachine;

        class NearestAllocMapper : public AllocMapper {
            public:
                enum NodeGenType{//center machine node generation
                    GREEDY_NODE = 0,   //O(N + VE)
                    EXHAUST_NODE = 1,  //O(NV + VE)
                };

                //add O(VE + V^2 lg V) if center task is not provided

                NearestAllocMapper(const Machine & mach,
                                   bool allocateAndMap,
                                   NodeGenType nodeGen = EXHAUST_NODE);
                ~NearestAllocMapper();

                std::string getSetupInfo(bool comment) const;

            private:
                NodeGenType nodeGen;
                long int lastNode;
                std::vector<long> radiusToVolume;

                //allocation variables:
                std::vector<int> vertexToNode; //maps communication graph vertices to machine nodes
                std::vector<int> taskToVertex; //maps task #s to communication graph vertices
                std::vector<std::map<int,int> >* commGraph;
                std::vector<bool> marked;
                int centerTask;
                int centerNode;

                //allocation & mapping function
                void allocMap(const AllocInfo & ai,
                              std::vector<long int> & usedNodes,
                              std::vector<int> & taskToNode);

                //creates a new communication graph (hyper graph) based on the # coresPerNode
                //if(centerTask_given)
                //  if(coresPerNode == 1)           O(V)
                //  if(coresPerNode == c)
                //      if(METIS_available)         O(V + E lg V)
                //else                              O(VE + V^2 lg V)
                void createCommGraph(const Job & job);

                //finds the vertex that minimizes the cumulative communication distance
                //@upperLimit: max number of tasks to search for
                //if(upperLimit < V),   O((E + V lg V) * upperLimit)
                //else,                 O((E + V lg V) * V)
                int getCenterTask(const std::vector<std::map<int,int> > & inCommGraph, const long int upperLimit = LONG_MAX) const;

                //returns a center machine node for allocation
                //@upperLimit: max number of nodes to search for
                //chooses an heuristic center that has approximately nodesNeeded free nodes around it
                //tries first next upperLimit nodes
                //if(upperLimit < N),   O(N + upperLimit * V)
                //else,                 O(N * V)
                int getCenterNodeExh(const int nodesNeeded, const long int upperLimit = LONG_MAX);

                //returns a center machine node for allocation
                //gets the next free node
                //O(N), depends on the machine utilization, expected: O(N * util)
                int getCenterNodeGr();

                //custom implementation of Dijkstra's algorithm with Fibonacci heap
                //O(E + V lg V)
                //the algorithm terminates if the total distance is larger than given limit
                //edge distances are taken as ( 1 / edgeWeight)
                double dijkstraWithLimit(const std::vector<std::map<int,int> > & graph,
                                         const unsigned int source,
                                         const double limit
                                         ) const;

                //if initDist = 0, returns the list of the closest available nodes in the machine graph
                //else, returns the list of the available nodes with distance=initDist in the machine graph
                //@initDist: distance from the center node
                //if initDist = 0, O(N^2), but typically O(1)
                //else, O(initDist^2)
                std::list<int> *closestNodes(const long int srcNode, const int initDist) const;

                //returns the tiedNodes element with the least total communication distance using inTask
                //removes the returned index from the list
                //O(tiedNodes->size() * E + V), O(tiedNodes->size() * E + V) when called for all tasks
                int bestNode(std::list<int> & tiedNodes, int inTask) const;

                //adds the unallocated neighbors of curTask to the task list with ascending weights
                //O(V), O(V+E) when called for all nodes
                void updateTaskList(int mappedTask, FibonacciHeap & taskList);
        };
    }
}

#endif /* SST_SCHEDULER_NEARESTALLOCMAPPER_H_ */
