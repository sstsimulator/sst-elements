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

#include "sst_config.h"
#include "NearestAllocMapper.h"

#ifdef HAVE_METIS
#include <metis.h>
#endif

#include "AllocInfo.h"
#include "Job.h"
#include "FibonacciHeap.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "TaskCommInfo.h"
#include "output.h"

#include <cmath>
#include <cfloat>
#include <queue>

using namespace SST::Scheduler;
using namespace std;

NearestAllocMapper::NearestAllocMapper(const MeshMachine & mach,
                                       TaskGenType inTaskGen,
                                       NodeGenType inNodeGen,
                                       TaskOrderType inTaskOrder)
    : AllocMapper(mach), mMachine(mach)
{
    taskGen = inTaskGen;
    nodeGen = inNodeGen;
    taskOrder = inTaskOrder;
    lastNode = 0;
}

NearestAllocMapper::~NearestAllocMapper()
{

}

std::string NearestAllocMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    return com + "Nearest AllocMapper";
}

AllocInfo* NearestAllocMapper::allocate(Job* job)
{
    if (!canAllocate(*job)){
        return NULL;
    }

    AllocInfo *ai = new AllocInfo(job, mach);
    int nodesNeeded = ai->getNodesNeeded();
    int jobSize = job->getProcsNeeded();

    //Optimization: Simple allocation & mapping if <= 2 tasks are provided
    if(jobSize <= 2 || nodesNeeded == 1){
        std::vector<int>* freeNodes = machine.getFreeNodes();
        for(int i = 0; i < ai->getNodesNeeded(); i++) {
            ai->nodeIndices[i] = freeNodes->at(i);
        }
        delete freeNodes;
        std::vector<int> *mapping = new std::vector<int>(jobSize);
        for(int i = 0; i < jobSize; i++){
            mapping->at(i) = ai->nodeIndices[i/mach.coresPerNode];
        }
        addMapping(job->getJobNum(), mapping);
        return ai;
    }

    //reset taskToNode
    taskToNode.resize(nodesNeeded);
    std::fill(taskToNode.begin(), taskToNode.end(), -1);
    //get centerTask
    centerTask = job->taskCommInfo->centerTask;

    //create breadth-first tree around a center task
    createCommGraph(*(job->taskCommInfo));
    //get free node info
    isFree = mach.freeNodeList();

    //choose center node
    switch(nodeGen){
    case GREEDY_NODE:
        centerNode = getCenterNodeGr();
        break;
    case EXHAUST_NODE:
        centerNode = getCenterNodeExh(nodesNeeded);
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown node selection algorithm for Nearest AllocMapper");
    };

    //map tasks
    allocateAndMap();

    //fill the allocInfo
    for(int i = 0; i < nodesNeeded; i++){
        ai->nodeIndices[i] = taskToNode[i];
    }

    //store mapping
    std::vector<int> *mapping = new std::vector<int>(jobSize);
    for(int i = 0; i < jobSize; i++){
        mapping->at(i) = taskToNode[taskToVertex[i]];
    }
    addMapping(job->getJobNum(), mapping);

    //free memory
    taskToNode.clear();
    delete isFree;
    taskToVertex.clear();
    delete commGraph;
    weightTree.clear();

    return ai;
}

void NearestAllocMapper::createCommGraph(const TaskCommInfo & tci)
{
    std::vector<std::map<int,int> >* rawCommGraph = tci.getCommInfo();
    int jobSize = tci.getSize();
    int nodesNeeded = ceil((float) jobSize / mach.coresPerNode);

    //assign center task if required
    if(centerTask == -1){
        switch(taskGen){
        case GREEDY_TASK:
            centerTask = 0;
            break;
        case EXHAUSTIVE_TASK:
            centerTask = getCenterTask(*rawCommGraph);
            break;
        default:
            schedout.fatal(CALL_INFO, 1, "Unknown task generation algorithm for Nearest AllocMapper");
        };
    }

    taskToVertex.clear();
    taskToVertex.resize(jobSize, -1);
    if(mach.coresPerNode == 1){
        commGraph = rawCommGraph;
        for(int i = 0; i < nodesNeeded; i++){
            taskToVertex[i] = i;
        }
    } else {
        //find which task to put in which node
#ifdef HAVE_METIS //use METIS partitioning if available
        idx_t nvtxs = jobSize; //num vertices
        idx_t ncon = 1; //number of balancing constraints
        idx_t nparts = nodesNeeded; //number of parts to partition
        idx_t objval; //output variable
        //create graph info in CSR format
        vector<idx_t> xadj(jobSize + 1);
        vector<idx_t> adjncy;
        vector<idx_t> adjwgt;
        for(int taskIt = 0; taskIt < jobSize; taskIt++){
            xadj[taskIt] = adjncy.size();
            for(std::map<int, int>::iterator it = rawCommGraph->at(taskIt).begin(); it != rawCommGraph->at(taskIt).end(); it++){
                adjncy.push_back(it->first);
                adjwgt.push_back(it->second);
            }
        }
        xadj[jobSize] = adjncy.size();

        std::vector<idx_t> METIS_taskToVertex(taskToVertex.size());

        //partition
        METIS_PartGraphKway(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL,
                            &adjwgt[0], &nparts, NULL, NULL, NULL, &objval, &METIS_taskToVertex[0]);

        for(unsigned int i = 0; i < taskToVertex.size(); i++){
            taskToVertex[i] = METIS_taskToVertex[i];
        }
#else
        //partition with greedy: create vertices using breadth-first search from the center node
        vector<bool> isMarked(jobSize, false);
        isMarked[centerVertex] = true;
        std::queue<int> queue;
        int nextTask;
        int vertexIndex = 0;
        int vertexTaskCount = 0; //tasks within a vertex
        queue.push(centerTask);

        while(!queue.empty()){
            nextTask = queue.front();
            queue.pop();
            //change vertex if necessary
            vertexTaskCount++;
            if(vertexTaskCount > mach.coresPerNode){
                vertexIndex++;
                vertexTaskCount = 1;
            }
            //map
            taskToVertex[nextJob] = vertexIndex;
            //add unmarked neighbors to the queue
            for(std::map<int, int>::const_iterator it = graph.at(nextTask).begin(); it != graph.at(nextTask).end(); it++){
                if(!isMarked[it->first]){
                    isMarked[it->first] = true;
                    queue.push(it->first);
                }
            }
        }
#endif
        //fill commGraph - O(V + E lg V), O(V + E) if commGraph were not using map structure
        commGraph = new std::vector<std::map<int,int> >(nodesNeeded);
        //for all tasks
        for(int taskIt = 0; taskIt < jobSize; taskIt++){
            //for all neighbors
            for(map<int, int>::iterator it = rawCommGraph->at(taskIt).begin(); it != rawCommGraph->at(taskIt).end(); it++){
                if(taskToVertex[taskIt] != taskToVertex[it->first]){
                    if(commGraph->at(taskToVertex[taskIt]).count(taskToVertex[it->first]) == 0){
                        commGraph->at(taskToVertex[taskIt])[taskToVertex[it->first]] = it->second;
                    } else {
                        commGraph->at(taskToVertex[taskIt])[taskToVertex[it->first]] += it->second;
                    }
                }
            }
        }
        delete rawCommGraph;

        //get new center task
        //assign center task if required
        switch(taskGen){
        case GREEDY_TASK:
            centerTask = taskToVertex[centerTask];
            break;
        case EXHAUSTIVE_TASK:
            centerTask = getCenterTask(*commGraph);
            break;
        default:
            schedout.fatal(CALL_INFO, 1, "Unknown task generation algorithm for Nearest AllocMapper");
        };
    }
}

void NearestAllocMapper::allocateAndMap()
{
    std::queue<int> tasks;
    tasks.push(centerTask);
    list<int> frameNodes; //nodes that "frame" the current allocation
    frameNodes.push_back(centerNode);
    //main loop
    while(!tasks.empty()){
        int curTask = tasks.front();
        tasks.pop();
        //get which node to allocate
        int nodeToAlloc = tieBreaker(frameNodes, curTask);

        //allocate and update containers
        taskToNode[curTask] = nodeToAlloc;
        isFree->at(nodeToAlloc) = false;

        //get unallocated neighbors and put in the queue
        vector<int>* neighbors = getNeighbors(curTask);
        for(unsigned int i = 0; i < neighbors->size(); i++){
            tasks.push(neighbors->at(i));
        }
        delete neighbors;

        //extend frame - do not add the same node twice
        list<int> *newNodes = new list<int>();
        closestNodes(nodeToAlloc, 0, newNodes);
        for(list<int>::iterator newIt = newNodes->begin(); newIt != newNodes->end(); newIt++){
            bool found = false;
            for(list<int>::iterator oldIt = frameNodes.begin(); oldIt != frameNodes.end(); oldIt++){
                if(*oldIt == *newIt){
                    found = true;
                    break;
                }
            }
            if(!found){
                frameNodes.push_back(*newIt);
            }
        }
        delete newNodes;
    }
}

int NearestAllocMapper::getCenterTask(const std::vector<std::map<int,int> > & inCommGraph, const long upperLimit) const
{
    int centerTask = INT_MAX;
    double minDist = DBL_MAX;
    int jobSize = inCommGraph.size();
    long searchCount = 0;

    double newDist;
    for(int i = 0; i < jobSize; i++){
        //start from the middle task - higher probability of smallest distance
        int task = max((long) 0, (i + jobSize / 2) % jobSize - upperLimit / 2);
        newDist = dijkstraWithLimit(inCommGraph, task, minDist);
        if(newDist < minDist){
            minDist = newDist;
            centerTask = task;
        }
        searchCount++;
        if(searchCount > upperLimit){
            break;
        }
    }
    return centerTask;
}

int NearestAllocMapper::getCenterNodeExh(const int nodesNeeded, const long upperLimit)
{
    int bestNode = -1;
    double bestScore = -DBL_MAX;
    long searchCount = 0;
    //approximate L1 from center to keep all the nodes in a 3D diamond
    const int optDist = cbrt(nodesNeeded);
    //for all nodes
    for(long nodeIt = 0; nodeIt < mach.numNodes; nodeIt++){
        if(isFree->at(lastNode)){
            double curScore = 0;
            for(int dist = 1; dist <= optDist; dist++){
                curScore += (double) closestNodes(lastNode, dist) / (dist);
            }
            //penalize node if it has more space around it
            //this should increase machine efficiency
            for(int dist = optDist + 1; dist <= optDist + 2; dist++){
                curScore -= (double) closestNodes(lastNode, dist) / (dist*dist);
            }
            //update best node
            if(curScore > bestScore){
                bestScore = curScore;
                bestNode = lastNode;
            }
            //optimization - preempt if upper bound reached
            searchCount++;
            if(searchCount >= upperLimit){
                break;
            }
        } else {
            lastNode = (lastNode + 1) % mach.numNodes;
        }
    }
    return bestNode;
}

int NearestAllocMapper::getCenterNodeGr()
{
    for(long nodeIt = 0; nodeIt < mach.numNodes; nodeIt++){
        if(isFree->at(lastNode)){
            break;
        } else {
            lastNode = (lastNode + 1) % mach.numNodes;
        }
    }
    return lastNode;
}

double NearestAllocMapper::dijkstraWithLimit(const vector<map<int,int> > & graph,
                                         const unsigned int source,
                                         const double limit
                                         ) const
{
    //initialize
    double totDist = 0;
    vector<double> dists(graph.size(), DBL_MAX);
    dists[source] = 0;
    vector<unsigned int> prevVertex(graph.size());
    prevVertex[source] = source;
    FibonacciHeap heap(dists.size());
    for(unsigned int i = 0; i < dists.size(); i++){
        heap.insert(i, dists[i]);
    }

    //main loop
    while(!heap.isEmpty()){
        unsigned int curNode = heap.deleteMin();
        //preempt if total distance is higher than limit
        totDist += dists[curNode] - dists[prevVertex[curNode]];
        if(totDist > limit){
            break;
        }
        for(std::map<int, int>::const_iterator it = graph[curNode].begin(); it != graph[curNode].end(); it++){
            double newDist = dists[curNode] + (double) 1 / it->second;
            if(newDist < dists[it->first]){
                dists[it->first] = newDist;
                prevVertex[it->first] = curNode;
                heap.decreaseKey(it->first, newDist);
            }
        }
    }
    return totDist;
}

int NearestAllocMapper::closestNodes(const long srcNode, const int initDist, std::list<int> *outList) const
{
    int nodeCount = 0;
    int retNode;
    int delta = initDist - 1;  //L1 distance to search for
    int srcX = mMachine.xOf(srcNode);
    int srcY = mMachine.yOf(srcNode);
    int srcZ = mMachine.zOf(srcNode);
    int largestDim = mMachine.getXDim() + mMachine.getYDim() + mMachine.getZDim();
    while(nodeCount == 0){
        if(initDist == 0){
            //increase L1 distance of the search
            delta++;
        } else { //function is called for specific distance
            break;
        }
        if(delta > largestDim){
            //no available node found - this is an exception while allocating the last node
            break;
        }
        //scan the nodes with L1 dist = delta;
        for(int x = srcX - delta; x < mMachine.getXDim() && x < (srcX + delta + 1); x++){
            if(x < 0){
                continue;
            } else {
                int yRange = delta - abs(srcX - x);
                for(int y = srcY - yRange; y < mMachine.getYDim() && y < (srcY + yRange + 1); y++){
                    if(y < 0){
                        continue;
                    } else {
                        int zRange = yRange - abs(srcY - y);

                        int z = srcZ - zRange;
                        if(z >= 0) {
                            retNode = mMachine.indexOf(x, y, z);
                            if(isFree->at(retNode)){
                                nodeCount++;
                                if(outList != NULL){
                                    outList->push_back(retNode);
                                }
                            }
                        }

                        z = srcZ + zRange;
                        if(zRange != 0 && z < mMachine.getZDim()){
                            retNode = mMachine.indexOf(x, y, z);
                            if(isFree->at(retNode)){
                                nodeCount++;
                                if(outList != NULL){
                                    outList->push_back(retNode);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return nodeCount;
}

int NearestAllocMapper::tieBreaker(list<int> & tiedNodes, int inTask) const
{
    int bestNode = 0;
    //optimization
    if(tiedNodes.size() == 1){
        bestNode = tiedNodes.front();
        tiedNodes.clear();
    } else {
        //get allocated neighbors
        vector<int> neighbors;
        vector<int> neighWeights;
        for(map<int, int>::iterator it = commGraph->at(inTask).begin(); it != commGraph->at(inTask).end(); it++){
            if(taskToNode[it->first] != -1){
                neighbors.push_back(it->first);
                neighWeights.push_back(it->second);
            }
        }

        //for each possible node
        double minDist = DBL_MAX;
        list<int>::iterator bestIt;
        for(list<int>::iterator nodeIt = tiedNodes.begin(); nodeIt != tiedNodes.end(); nodeIt++){
            double curDist = 0;
            //iterate over task neighbors and calculate the total comm distance if this task was allocated here
            for(unsigned int nIt = 0; nIt < neighbors.size(); nIt++){
                curDist += mach.getNodeDistance(taskToNode[neighbors[nIt]], *nodeIt) * neighWeights[nIt];
            }
            if(curDist < minDist){
                minDist = curDist;
                bestNode = *nodeIt;
                bestIt = nodeIt;
            }
        }
        tiedNodes.erase(bestIt);
    }
    return bestNode;
}

vector<int>* NearestAllocMapper::getNeighbors(int taskNo) const
{
    vector<int>* neighbors = new vector<int>();

    //find unallocated neighbors
    for(map<int, int>::const_iterator it = commGraph->at(taskNo).begin(); it != commGraph->at(taskNo).end(); it++){
        if(taskToVertex[it->first] == -1){
            neighbors->push_back(it->first);
        }
    }

    //sort if necessary
    if(taskOrder == SORTED_ORDER){
        int size = neighbors->size();
        vector<int> commWeights(size);
        //for all neighbors
        for(vector<int>::const_iterator neighIt = neighbors->begin(); neighIt != neighbors->end(); neighIt++){
            //for all of 2nd degree neighbors, add up communication for those already allocated
            for(map<int, int>::const_iterator neigh2It = commGraph->at(*neighIt).begin(); neigh2It != commGraph->at(*neighIt).end(); neigh2It++){
                if(taskToVertex[neigh2It->first] != -1){
                    commWeights[*neighIt] += neigh2It->second;
                }
            }
        }

        //sort weights, get indices
        vector<int> sorted = sortWithIndices(commWeights);

        //re-order neighbors
        vector<int>* sortedNeighbors = new vector<int>(size);
        for(int i = 0; i < size; i++){
            sortedNeighbors->at(i) = neighbors->at(sorted[i]);
        }
        delete neighbors;
        neighbors = sortedNeighbors;
    } else if(taskOrder != GREEDY_ORDER){
        schedout.fatal(CALL_INFO, 1, "Unknown task ordering option for Nearest AllocMapper");
    }

    return neighbors;
}

vector<int> NearestAllocMapper::sortWithIndices(const vector<int> & toSort, vector<int> *values) const
{
   vector<pair<int, int> > pairs(toSort.size());
   vector<int> indices(toSort.size());

   for(unsigned int i = 0; i < toSort.size(); i++){
       pairs[i] = pair<int, int>(toSort[i], i);
   }

   stable_sort(pairs.begin(), pairs.end(), SortHelper());

   for(unsigned int i = 0; i < toSort.size(); i++){
       indices[i] = pairs[i].second;
   }

   if(values != NULL){
       values->clear();
       values->resize(toSort.size());
       for(unsigned int i = 0; i < toSort.size(); i++){
           values->at(i) = pairs[i].first;
       }
   }

   return indices;
}

