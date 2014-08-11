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
        class TaskCommInfo;

        class NearestAllocMapper : public AllocMapper {
            public:
                enum AlgorithmType{
                    GREEDY = 0,
                    EXHAUSTIVE = 1,
                };

                enum CenterGenType{//center machine node generation
                    GREEDY_NODE = 0,
                    EXHAUST_NODE = 1,
                };

                NearestAllocMapper(const MeshMachine & mach, AlgorithmType algMode = EXHAUSTIVE, CenterGenType centerMode = EXHAUST_NODE);
                ~NearestAllocMapper();

                std::string getSetupInfo(bool comment) const;

                //returns allocation information or NULL if it wasn't possible
                //(doesn't make allocation; merely returns info on possible allocation)
                //providing a center Task significantly speeds up the algorithm
                AllocInfo* allocate(Job* job);

                //returns task mapping info of a single job; does not map the tasks
                TaskMapInfo* mapTasks(AllocInfo* allocInfo);

            private:

                AlgorithmType algorithm;
                CenterGenType centerGen;
                const MeshMachine & mMachine;
                long lastNode;

                //allocation variables:
                // - as object variables for easier access, deleted after allocation
                std::vector<int> taskToNode;   //maps communication graph vertices to machine nodes
                std::vector<bool>* isFree;     //keeps a temporary copy of node list
                std::vector<int> taskToVertex; //maps task #s to communication graph vertices
                std::vector<std::map<int,int> >* commGraph;
                std::vector<std::vector<int> >* commTree;   //breadth-first tree version of the commGraph
                std::vector<std::vector<int> > weightTree;  //weight of the commTree
                int centerTask;
                int centerNode;

                //creates a new communication graph (hyper graph) based on the # coresPerNode
                //if(GREEDY_CEN || centerTask_given)
                //  if(coresPerNode == 1)           O(V)
                //  if(coresPerNode == c)
                //      if(METIS_available)         O(VE/c + (V/c)^2 lg (V/c) + METIS_partitioning(V/c))
                //      else                        O(VE/c + (V/c)^2 lg (V/c))
                //else                              O(VE + V^2 lg V)
                void createCommGraph(const TaskCommInfo & tci);

                //O(V + E)
                void greedyMap();

                //O(V * E)
                void exhaustiveMap();

                //finds the vertex that minimizes the cumulative communication distance
                //@upperLimit: max number of tasks to search for
                //else, O(V*dijkstraWithLimit) = O(VE + V^2 lg V)
                int getCenterTask(const std::vector<std::map<int,int> > & inCommGraph, const long upperLimit = LONG_MAX) const;

                //returns adjacency list of a directed tree with centerTask as its root
                //O(V + E)
                //@weights output for tree weights
                std::vector<std::vector<int> >* breadthFirstTree(int centerTask,
                                                                 const std::vector<std::map<int,int> > & graph,
                                                                 std::vector<std::vector<int> > & outWeights) const;

                //returns a center machine node for allocation
                //@upperLimit: max number of nodes to search for
                //chooses an heuristic center that has approximately nodesNeeded free nodes around it
                //tries all the nodes
                //O(N * V^2)
                int getCenterNodeExh(const int nodesNeeded, const long upperLimit = 1000) const;

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
                //@initDist: starting distance from the center node
                //adds the nodes to the outList when provided
                //if initDist = 0, O(N^2), but typically O(1)
                //else, O(initDist^2)
                int closestNodes(const long srcNode, const int initDist, std::list<int> *outList = NULL) const;

                //returns the tiedNodes element with the least communication using inTask
                //removes the returned index from the list
                //O(tiedNodes->size() * E + V), O(tiedNodes->size() * E + V) when called for all tasks
                int tieBreaker(std::list<int> & tiedNodes, int inTask) const;

                //orders neighbors of the given task in commTree by their communication
                //return indexes for largest to smallest
                //O(E lg E), O(V) when called for all tasks
                std::vector<int> orderTreeNeighByComm(int task) const;

                //sorts given vector from largest to smallest
                //O(n lg n), n=toSort.size()
                //@values sorted values
                //@return sorted vector indexes
                std::vector<int> sortWithIndices(const std::vector<int> & toSort,
                                                 std::vector<int> *values = NULL ) const;
                struct SortHelper {
                    bool operator() (const std::pair<int, int>& l, const std::pair<int, int>& r)
                    {
                        return (l.first > r.first);
                    }
                } compObject;
        };
    }
}

#endif /* SST_SCHEDULER_NEARESTALLOCMAPPER_H_ */

