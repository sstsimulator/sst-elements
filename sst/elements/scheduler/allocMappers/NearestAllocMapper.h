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

#ifndef SST_SCHEDULER_NEARESTALLOCMAPPER_H_
#define SST_SCHEDULER_NEARESTALLOCMAPPER_H_

#include "AllocMapper.h"

#include <list>
#include <map>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class MeshMachine;

        class NearestAllocMapper : public AllocMapper {
            public:
                enum TaskGenType{//center task generation
                    GREEDY_TASK = 0, //O(VE)
                    EXHAUSTIVE_TASK = 1, //O(VE + V^2 lg V) - can be limited
                };

                enum NodeGenType{//center machine node generation
                    GREEDY_NODE = 0,   //O(N)
                    EXHAUST_NODE = 1,  //O(N * V^2) - can be limited
                };

                enum TaskOrderType{//neighbor task ordering
                    GREEDY_ORDER = 0, //O(VE)
                    SORTED_ORDER = 1, //O(VE + V^2 lg V) while expanding,
                                      //chooses the task with the highest
                                      //communication to the currently allocated tasks
                 };
                //if number of cores per node > 1, add O(V + E lg V)
                //replace all V with V/c when c cores per node, except for taskGenType

                NearestAllocMapper(const MeshMachine & mach,
                                   TaskGenType taskGen = GREEDY_TASK,
                                   NodeGenType nodeGen = EXHAUST_NODE,
                                   TaskOrderType taskOrder = SORTED_ORDER);
                ~NearestAllocMapper();

                std::string getSetupInfo(bool comment) const;

                //returns allocation information or NULL if it wasn't possible
                //(doesn't make allocation; merely returns info on possible allocation)
                //providing a center Task significantly speeds up the algorithm
                AllocInfo* allocate(Job* job);

            private:

                TaskGenType taskGen;
                NodeGenType nodeGen;
                TaskOrderType taskOrder;

                const MeshMachine & mMachine;
                long lastNode;

                //allocation variables:
                // - as object variables for easier access, deleted after allocation
                std::vector<int> taskToNode;   //maps communication graph vertices to machine nodes
                std::vector<bool>* isFree;     //keeps a temporary copy of node list
                std::vector<int> taskToVertex; //maps task #s to communication graph vertices
                std::vector<std::map<int,int> >* commGraph;
                std::vector<std::vector<int> > weightTree;  //weight of the commTree
                std::vector<bool> marked;
                int centerTask;
                int centerNode;

                //creates a new communication graph (hyper graph) based on the # coresPerNode
                //if(GREEDY_CEN || centerTask_given)
                //  if(coresPerNode == 1)           O(V)
                //  if(coresPerNode == c)
                //      if(METIS_available)         O(V + E lg V + METIS_partitioning({V,E}))
                //      else                        O(V + E lg V)
                //else                              O(VE + V^2 lg V)
                void createCommGraph(const TaskCommInfo & tci);

                //O(V * E) - not sure
                void allocateAndMap();

                //finds the vertex that minimizes the cumulative communication distance
                //@upperLimit: max number of tasks to search for
                //if(upperLimit < V),   O((E + V lg V) * upperLimit)
                //else,                 O((E + V lg V) * V)
                int getCenterTask(const std::vector<std::map<int,int> > & inCommGraph, const long upperLimit = LONG_MAX) const;

                //returns a center machine node for allocation
                //@upperLimit: max number of nodes to search for
                //chooses an heuristic center that has approximately nodesNeeded free nodes around it
                //tries first next upperLimit nodes
                //if(upperLimit < N),   O(N + upperLimit * V^2)
                //else,                 O(N * V^2)
                int getCenterNodeExh(const int nodesNeeded, const long upperLimit = LONG_MAX);

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

                //if initDist = 0, returns the number of the closest available nodes in the machine graph
                //else, returns the number of the available nodes with distance=initDist in the machine graph
                //@initDist: distance from the center node
                //adds the nodes to the outList when provided
                //if initDist = 0, O(N^2), but typically O(1)
                //else, O(initDist^2)
                int closestNodes(const long srcNode, const int initDist, std::list<int> *outList = NULL) const;

                //returns the tiedNodes element with the least total communication distance using inTask
                //removes the returned index from the list
                //O(tiedNodes->size() * E + V), O(tiedNodes->size() * E + V) when called for all tasks
                int bestNode(std::list<int> & tiedNodes, int inTask) const;

                //adds the unallocated neighbors of curTask to the task list with descenting weights
                //O(V lg V)
                void getSortedNeighbors(int curTask, std::list<std::pair<int, double> > *taskList);

                //sorts given vector, descending
                //O(n lg n), n=toSort.size()
                //@return sorted vector indexes
                //std::vector<int> sortIndices(const std::vector<std::pair<int, double>> & toSort) const;
                struct ByWeights {
                    bool operator() (const std::pair<int, double>& l, const std::pair<int, double>& r)
                    {
                        return (l.second >= r.second);
                    }
                } compObject;
        };
    }
}

#endif /* SST_SCHEDULER_NEARESTALLOCMAPPER_H_ */

