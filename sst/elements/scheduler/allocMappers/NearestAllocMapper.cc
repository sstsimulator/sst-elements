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

#include "NearestAllocMapper.h"

#ifdef HAVE_METIS
#include "metis.h"
#endif

#include "AllocInfo.h"
#include "Job.h"
#include "FibonacciHeap.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "TaskCommInfo.h"
#include "TaskMapInfo.h"
#include "output.h"

#include <cmath>
#include <cfloat>
#include <queue>

using namespace SST::Scheduler;
using namespace std;

NearestAllocMapper::NearestAllocMapper(const MeshMachine & mach, AlgorithmType algMode, CenterGenType centerMode)
    : AllocMapper(mach), mMachine(mach)
{
    algorithm = algMode;
    centerGen = centerMode;
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
    switch(centerGen){
    case GREEDY_CEN:
        centerNode = getCenterNodeGr();
        break;
    case EXHAUST_CEN:
        centerNode = getCenterNodeExh(nodesNeeded);
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown node selection algorithm for Nearest AllocMapper");
    };

    //allocate & map
    switch(algorithm){
    case GREEDY:
        greedyMap();
        break;
    case EXHAUSTIVE:
        exhaustiveMap();
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown algorithm for Nearest AllocMapper");
    }

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
    //delete freeNodes;
    delete isFree;
    taskToVertex.clear();
    delete commGraph;
    delete commTree;
    weightTree.clear();

    return ai;
}

TaskMapInfo* NearestAllocMapper::mapTasks(AllocInfo* allocInfo)
{
    long jobNum = allocInfo->job->getJobNum();
    std::vector<int>* mapping = getMappingOf(jobNum);
    TaskMapInfo* tmi = new TaskMapInfo(allocInfo);
    for(long taskIt = 0; taskIt < allocInfo->job->getProcsNeeded(); taskIt++){
        tmi->insert(taskIt, mapping->at(taskIt));
    }
    delete mapping;
    return tmi;
}

void NearestAllocMapper::createCommGraph(const TaskCommInfo & tci)
{
    std::vector<std::map<int,int> >* rawCommGraph = tci.getCommInfo();
    int jobSize = tci.getSize();
    int nodesNeeded = ceil((float) jobSize / mach.coresPerNode);

    //update center task if necessary
    switch(centerGen){
    case GREEDY_CEN:
        centerTask = 0;
        break;
    case EXHAUST_CEN:
        //find center task if not given
        if(centerTask == -1){
            centerTask = getCenterTask(*rawCommGraph);
        }
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown node selection algorithm for Nearest AllocMapper");
    };

    taskToVertex.resize(jobSize);
    if(mach.coresPerNode == 1){
        commGraph = rawCommGraph;
        commTree = breadthFirstTree(centerTask, *rawCommGraph, weightTree);
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
        //partition
        METIS_PartGraphKway(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL,
                            &adjwgt[0], &nparts, NULL, NULL, NULL, &objval, &taskToVertex[0]);

#else
        //partition with greedy: create vertices using breadth-first search from the center node
        std::vector<std::vector<int> >* rawCommTree = breadthFirstTree(centerTask, *rawCommGraph, weightTree);
        std::queue<int> queue;
        queue.push(centerTask);
        int nextJob;
        int vertexIndex = 0;
        int vertexTaskCount = 0; //tasks within a vertex
        while(!queue.empty()){
            nextJob = queue.front();
            queue.pop();
            //change vertex if necessary
            vertexTaskCount++;
            if(vertexTaskCount > mach.coresPerNode){
                vertexIndex++;
                vertexTaskCount = 1;
            }
            //map
            taskToVertex[nextJob] = vertexIndex;
            //add neighbors to the queue
            for(unsigned int i = 0; i < rawCommTree->at(nextJob).size(); i++){
                queue.push(rawCommTree->at(nextJob)[i]);
            }
        }

        delete rawCommTree;

#endif
        //fill commGraph - O(V + E lg V)
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
        centerTask = getCenterTask(*commGraph);
        //fill commTree
        commTree = breadthFirstTree(centerTask, *commGraph, weightTree);
    }
}

void NearestAllocMapper::greedyMap()
{
    //initialize & allocate center task to center node
    std::queue<int> tasks;
    std::queue<int> lastNode; //last allocated node to the tasks
    tasks.push(centerTask);
    lastNode.push(centerNode);
    //main loop
    while(!tasks.empty()){
        int curTask = tasks.front();
        tasks.pop();
        //get possible nodes
        list<int> *tiedNodes = new list<int>();
        closestNodes(lastNode.front(), 0, tiedNodes);
        lastNode.pop();
        //break ties
        int nodeToAlloc = tieBreaker(*tiedNodes, curTask);
        delete tiedNodes;
        //allocate to next node
        taskToNode[curTask] = nodeToAlloc;
        isFree->at(nodeToAlloc) = false;

        //put ordered neighbors in the queue
        vector<int> weightMap = orderTreeNeighByComm(curTask);
        for(unsigned int i = 0; i < commTree->at(curTask).size(); i++){
            tasks.push(commTree->at(curTask)[weightMap[i]]);
            lastNode.push(taskToNode[curTask]);
        }
    }
}

void NearestAllocMapper::exhaustiveMap()
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

        //put ordered neighbors in the queue
        vector<int> weightMap = orderTreeNeighByComm(curTask);
        for(unsigned int i = 0; i < commTree->at(curTask).size(); i++){
            tasks.push(commTree->at(curTask)[weightMap[i]]);
        }

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

vector<vector<int> >* NearestAllocMapper::breadthFirstTree(int centerVertex,
                                                           const std::vector<map<int,int> > & graph,
                                                           vector<std::vector<int> > & outWeights ) const
{
    int jobSize = graph.size();
    vector<vector<int> >* commTree = new vector<vector<int> >(jobSize);
    outWeights.clear();
    outWeights.resize(jobSize);
    vector<bool> isMarked(jobSize, false);
    isMarked[centerVertex] = true;
    std::queue<int> queue;
    queue.push(centerVertex);
    int nextTask;

    while(!queue.empty()){
        nextTask = queue.front();
        queue.pop();
        for(std::map<int, int>::const_iterator it = graph.at(nextTask).begin(); it != graph.at(nextTask).end(); it++){
            if(!isMarked[it->first]){
                isMarked[it->first] = true;
                queue.push(it->first);
                commTree->at(nextTask).push_back(it->first);
                outWeights[nextTask].push_back(it->second);
            }
        }
    }

    return commTree;
}

int NearestAllocMapper::getCenterNodeExh(const int nodesNeeded, const long upperLimit) const
{
    int bestNode = 0;
    double bestScore = -DBL_MAX;
    long searchCount = 0;
    //approximate L1 from center to keep all the nodes in a 3D diamond
    const int optDist = cbrt(nodesNeeded);
    //for all nodes
    for(long nodeIt = 0; nodeIt < mach.numNodes; nodeIt++){
        if(isFree->at(nodeIt)){
            double curScore = 0;
            for(int dist = 1; dist <= optDist; dist++){
                curScore += (double) closestNodes(nodeIt, dist) / dist;
            }
            //penalize node if it has more space around it
            //this should increase machine efficiency
            for(int dist = optDist + 1; dist <= optDist + 2; dist++){
                curScore -= (double) closestNodes(nodeIt, dist) / (dist*dist);
            }
            //update best node
            if(curScore > bestScore){
                bestScore = curScore;
                bestNode = nodeIt;
            }
            //optimization - preempt if upper bound reached
            searchCount++;
            if(searchCount >= upperLimit){
                break;
            }
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

/*
void NearestAllocMapper::createMachineGraph()
{
    //get free machine nodes
    vector<int> *freeNodes = mach.getFreeNodes();
    //fill graph
    machGraph.resize(freeNodes->size());
    for(unsigned int i = 0; i < freeNodes->size(); i++){
        for(unsigned int j = 0; j < freeNodes->size(); j++){
            if(i != j){
                machGraph[i][j] = mach.getNodeDistance(freeNodes->at(i), freeNodes->at(j));
            }
        }
    }
    delete freeNodes;
}*/

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

vector<int> NearestAllocMapper::sortWithIndices(const vector<int> & toSort, vector<int> *values) const
{
   vector<pair<int, int> > pairs(toSort.size());
   vector<int> indices(toSort.size());

   for(uint i = 0; i < toSort.size(); i++){
       pairs[i] = pair<int, int>(toSort[i], i);
   }

   stable_sort(pairs.begin(), pairs.end(), SortHelper());

   for(uint i = 0; i < toSort.size(); i++){
       indices[i] = pairs[i].second;
   }

   if(values != NULL){
       values->clear();
       values->resize(toSort.size());
       for(uint i = 0; i < toSort.size(); i++){
           values->at(i) = pairs[i].first;
       }
   }

   return indices;
}

std::vector<int> NearestAllocMapper::orderTreeNeighByComm(int task) const
{
    int neighborSize = commTree->at(task).size();
    vector<int> weights(neighborSize, 0);
    //optimization
    if(neighborSize == 1){
        weights[0] = 0;
        return weights;
    }
    for(int i = 0; i < neighborSize; i++){
        weights[i] = weightTree[task][i];
    }
    return sortWithIndices(weights);
}

