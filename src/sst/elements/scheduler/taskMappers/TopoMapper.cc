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

#include "TopoMapper.h"

#include "sst_config.h"

#ifdef HAVE_METIS
#include "metis.h"
#endif
#include "Rcm.h"

#include <cassert>
#include <climits>
#include <numeric>
#include <vector>
#include <cmath>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "output.h"
#include "SimpleTaskMapper.h"
#include "TaskCommInfo.h"
#include "TaskMapInfo.h"

using namespace SST::Scheduler;
using namespace std;

TopoMapper::TopoMapper(const Machine & mach, AlgorithmType mode) : TaskMapper(mach)
{
    algorithmType = mode;
}

TopoMapper::~TopoMapper()
{

}

std::string TopoMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    if(algorithmType == RECURSIVE){
        return com + "TopoMapper";
    } else {
        return com + "RCM (Reverse Cuthill Mckee) Mapper";
    }
}

TaskMapInfo* TopoMapper::mapTasks(AllocInfo* allocInfo)
{
    numTasks = allocInfo->job->getProcsNeeded();
    numNodes = allocInfo->getNodesNeeded();

    //Optimization: SimpleTaskMapper if <= 2 tasks are provided
    if(numTasks <= 2 || numNodes == 1){
        SimpleTaskMapper simpleMapper = SimpleTaskMapper(mach);
        return simpleMapper.mapTasks(allocInfo);
    }

    setup(allocInfo); //convert network & job info into required formats

    mapping = vector<int>(numTasks);
    vector<int> part(numTasks); //partition vector: part[task] = partition #
    //Partition task if nodes have homogeneous multiple processors
    //Current code does not support heterogeneous case
    if( mach.coresPerNode > 1){
#ifdef HAVE_METIS
        idx_t objval; //objective function result
        vector<idx_t> METIS_part(numTasks); //partition vector: part[task] = partition #
        idx_t nvtxs = numTasks;
        idx_t ncon = 1; //number of balancing constraints
        idx_t nparts = ceil((double) numTasks / mach.coresPerNode); //number of parts to partition

        //create graph info in CSR format
        vector<idx_t> xadj;
        vector<idx_t> adjncy;
        vector<idx_t> adjwgt;
        for(int i = 0; i < numTasks; i++){
            xadj.push_back(adjncy.size());
            for(unsigned int j = 0; j < commGraph[i].size(); j++){
                adjncy.push_back(commGraph[i][j]);
                adjwgt.push_back(commWeights[i][j]);
            }
        }
        xadj.push_back(adjncy.size());

        //partition
        METIS_PartGraphKway(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL,
                            &adjwgt[0], &nparts, NULL, NULL, NULL, &objval, &METIS_part[0]);

        //check if partitioning is balanced or not, correct if necessary
        //count number of tasks at each vertex
        std::vector<int> numTasksInNode(nparts, 0);
        std::vector<int> extraTasks; //keeps overflowing tasks
        bool isBalanced = true;
        for(int taskNo = 0; taskNo < numTasks; taskNo++){
            numTasksInNode[METIS_part[taskNo]]++;
            if(numTasksInNode[METIS_part[taskNo]] > mach.coresPerNode){
                extraTasks.push_back(taskNo);
                isBalanced = false;
            } else {
                part[taskNo] = METIS_part[taskNo];
            }
        }
        //fix if not balanced
        long int nodeIter = 0;
        while(!isBalanced && !extraTasks.empty()){
            if(numTasksInNode[nodeIter] < mach.coresPerNode){
                numTasksInNode[nodeIter]++;
                part[extraTasks.back()] = nodeIter;
                extraTasks.pop_back();
            } else {
                nodeIter++;
            }
        }

#else
        schedout.fatal(CALL_INFO, 1, "Topo Mapper requires METIS library for multi-core nodes");
#endif
    } else {
        for(unsigned int i = 0; i < part.size(); i++){
            part[i] = i;
        }
    }

    //build node topology graph for new task distribution
    vector<vector<int> > nodeTopGraph(ceil((double) numTasks / mach.coresPerNode));
    vector<vector<int> > nodeTopGraphWeights(nodeTopGraph.size()); // edge weights
    for(unsigned int source = 0; source < commGraph.size(); ++source) {
        for(unsigned int j = 0; j < commGraph[source].size(); ++j) {
            //for each communicating task pair
            int target = commGraph[source][j];
            int sourceNode = part[source];
            int targetNode = part[target];
            //add edge to node topology graph if necessary
            if(sourceNode != targetNode) {
                int found = 0;
                int pos = 0;
                for(unsigned int k = 0; k < nodeTopGraph[sourceNode].size(); ++k, ++pos){
                    if(nodeTopGraph[sourceNode][pos] == targetNode){
                        found = 1;
                        break;
                    }
                }
                if(found) { //edge already existent - add to weight
                    nodeTopGraphWeights[sourceNode][pos] += commWeights[source][j];
                } else { //create edge
                    nodeTopGraph[sourceNode].push_back(targetNode);
                    nodeTopGraphWeights[sourceNode].push_back(commWeights[source][j]);
                }
            }
        }
    }

    for(unsigned int i = 0; i < numCores.size(); ++i){
        if(numCores[i] > 0){
            numCores[i] = 1;
        }
    }

    vector<int> nodeTopGraphMap(nodeTopGraph.size()); // mapping from nodeTopGraph to nodeGraph

    if(algorithmType == R_C_M){

        mapRCM(&nodeTopGraph, &nodeTopGraphMap);

    } else if (algorithmType == RECURSIVE){
        // copy nodeGraph and networkWeights graphs
        vector<vector<int> > pyhTopGraph = vector<vector<int> >(phyGraph);
        vector<vector<int> > phyTopGraphWeights = vector<vector<int> >(networkWeights);
        vector<int> pmap; //physical mapping
        for(unsigned int i = 0; i < phyGraph.size(); ++i){
            pmap.push_back(i);
        }
        vector<int> lmap; //logical mapping
        for(unsigned int i = 0; i < nodeTopGraph.size(); ++i){
            lmap.push_back(i);
        }
        mapRecursive(&pyhTopGraph, &phyTopGraphWeights, &numCores, &nodeTopGraph, &nodeTopGraphWeights, &nodeTopGraphMap, lmap, &pmap);
    }

    // inflate mapping back to logical topology size
    for(unsigned int i = 0; i < commGraph.size(); ++i) {
      int pos_in_nodeTopGraph = part[i];
      mapping[i] = nodeTopGraphMap[pos_in_nodeTopGraph];
    }

    // convert to SST's task map info
    TaskMapInfo* tmi = new TaskMapInfo(allocInfo, mach);
    for(unsigned int i = 0; i < mapping.size(); i++){
        tmi->insert(i, allocInfo->nodeIndices[mapping[i]]);
    }

    //free memory
    commGraph.clear();
    commWeights.clear();
    numCores.clear();
    phyGraph.clear();
    networkWeights.clear();
    mapping.clear();

    return tmi;
}

void TopoMapper::setup(AllocInfo* allocInfo)
{
    //create communication graph
    for(int i = 0; i < numTasks; i++){
        commGraph.push_back(vector<int>());
        commWeights.push_back(vector<int>());
    }
    TaskCommInfo* tci = allocInfo->job->taskCommInfo;
    //ensure symmetry
    for(int i = 0; i < numTasks; i++){
        for(int j = i + 1; j < numTasks; j++){
            int i_to_j = tci->getCommWeight(i, j);
            int j_to_i = tci->getCommWeight(j, i);
            if( i_to_j != 0 || j_to_i != 0){
                commGraph[i].push_back(j);
                commGraph[j].push_back(i);
                if(i_to_j != 0){
                    commWeights[i].push_back(i_to_j);
                    commWeights[j].push_back(i_to_j);
                } else {
                    commWeights[i].push_back(j_to_i);
                    commWeights[j].push_back(j_to_i);
                }
            }
        }
    }

    //add node weights
    numCores = vector<int>(numNodes);

    std::fill(numCores.begin(), numCores.end(), mach.coresPerNode);

    //create node graph
    for(int i = 0; i < numNodes; i++){
        phyGraph.push_back(vector<int>());
        networkWeights.push_back(vector<int>());
    }
    for(int i = 0; i < numNodes; i++){
        for(int j = 0; j < numNodes; j++){ //make the graph symmetric
            //assuming neighbors have distance of 1
            if(mach.getNodeDistance(allocInfo->nodeIndices[i], allocInfo->nodeIndices[j]) == 1){
                phyGraph[i].push_back(j);
                networkWeights[i].push_back(1);
            }
        }
    }
}

int TopoMapper::mapRCM(std::vector<std::vector<int> > *commGraph_ref,
                          std::vector<int> *mapping_ref)
{
    vector<vector<int> >& commGraph = *commGraph_ref;
    vector<int>& mapping = *mapping_ref;

    vector<vector<int> > rtg;
    vector<int> rtg2ptgmap(phyGraph.size()); // map to translate rtg vertices to nodeGraph vertices
    vector<int> rcm_nodeGraph_map(phyGraph.size());
    RCM rcm;

    int ptgn = 0;
    for(unsigned int i = 0; i < phyGraph.size(); ++i){
        ptgn += phyGraph[i].size();
    }
    vector<int> xadj(phyGraph.size()+1); // CSR index
    vector<int> adjncy(ptgn); // CSR list
    for(unsigned int i=0; i < (phyGraph.size()+1); i++){
        if(i==0){
            xadj[i]=0;
        } else {
            xadj[i] = xadj[i-1]+phyGraph[i-1].size();
        }
    }
    int pos=0;
    for(unsigned int i=0; i<phyGraph.size(); i++){
        for(unsigned int j=0; j<phyGraph[i].size(); ++j){
            adjncy[pos++] = phyGraph[i][j];
        }
    }
    vector<signed char> mask(phyGraph.size(), 1);
    vector<int> degs(phyGraph.size());
    rcm.genrcm((int)phyGraph.size(), &xadj[0], &adjncy[0], &rcm_nodeGraph_map[0], &mask[0], &degs[0]);

    vector<int> rcm_commGraph_map(commGraph.size());
    {
        int ltgn=0;
        for(unsigned int i=0; i<commGraph.size(); ++i){
            ltgn+=commGraph[i].size();
        }
        vector<int> xadj(commGraph.size()+1); // CSR index
        vector<int> adjncy(ltgn); // CSR list
        for(unsigned int i=0; i<(commGraph.size()+1); i++){
            if(i==0){
                xadj[i]=0;
            } else {
                xadj[i]=xadj[i-1]+commGraph[i-1].size();
            }
        }
        int pos=0;
        for(unsigned int i=0; i<commGraph.size(); i++) {
            for(unsigned int j=0; j<commGraph[i].size(); ++j) {
                adjncy[pos++] = commGraph[i][j];
            }
        }

        vector<signed char> mask(commGraph.size(), 1);
        vector<int> degs(commGraph.size());
        rcm.genrcm((int)commGraph.size(), &xadj[0], &adjncy[0], &rcm_commGraph_map[0], &mask[0], &degs[0]);
    }

    // translate mappings
    for(unsigned int i=0; i<commGraph.size(); ++i) {
        mapping[rcm_commGraph_map[i]] = rcm_nodeGraph_map[i];
    }

    return 0;
}

int TopoMapper::mapRecursive(vector<vector<int> > *nodeGraph_ref, vector<vector<int> > *networkWeights_ref,
        vector<int> *numCores_ref, vector<vector<int> > *commGraph_ref, vector<vector<int> > *weights_ref,
        vector<int> *mapping_ref, vector<int> commGraph_map, vector<int> *nodeGraph_map_ref)
{
#ifdef HAVE_METIS
    // turn refs into normal objects
    vector<vector<int> >& phyTopGraph = *nodeGraph_ref;
    vector<vector<int> >& networkWeights = *networkWeights_ref; // nodeGraph edge capacities!
    vector<vector<int> >& commGraph = *commGraph_ref;
    vector<vector<int> >& commWeights = *weights_ref; // commGraph edge weights!
    vector<int>& numCores = *numCores_ref;
    vector<int>& mapping = *mapping_ref;
    vector<int>& pmap= *nodeGraph_map_ref;

    if(commGraph_map.size() != 1) {
        //create graph info in CSR format
        vector<idx_t> xadj;
        vector<idx_t> adjncy;
        vector<idx_t> adjwgt;
        vector<idx_t> vwgt;
        for(unsigned int i = 0; i < networkWeights.size(); i++){
            xadj.push_back(adjncy.size());
            vwgt.push_back(numCores[i]);
            for(unsigned int j = 0; j < networkWeights[i].size(); j++){
                adjncy.push_back(phyTopGraph[i][j]);
                adjwgt.push_back(networkWeights[i][j]);
            }
        }
        xadj.push_back(adjncy.size());

        idx_t nvtxs = phyTopGraph.size();
        idx_t ncon = 1; //number of balancing constraints
        idx_t nparts = 2; //number of parts to partition
        idx_t edgecut; //objective function result
        vector<idx_t> part(phyTopGraph.size()); //partition vector: part[task] = partition #

        METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], &vwgt[0], NULL, &adjwgt[0],
                                         &nparts, NULL, NULL, NULL, &edgecut, &part[0]);

        int group1 = 0, group0 = 0, diff = 2;
        while(diff > 1) {
            group0 = 0;
            group1 = 0;
            for(int i = 0; i < nvtxs; ++i) {
                if(part[i] == 0) group0 += numCores[i];
                if(part[i] == 1) group1 += numCores[i];
            }

            // check if it's balanced or not -- correct if necessary
            diff = group0-group1;
            if(diff < 0) diff *= -1;
            if(diff > 1) { //it's not balanced!
                int min, min_vert;
                if(group0 > group1) { // reset one 0 to a 1 in part: pick the least connected one!
                    min=std::numeric_limits<int>::max();
                    min_vert=-1;
                    for(unsigned int i=0; i<part.size(); ++i) {
                        if(part[i] == 0 && numCores[i] <= diff) {
                            int outweight = std::accumulate(phyTopGraph[i].begin(), phyTopGraph[i].end(), 0);
                            if(min > outweight) {
                                min_vert = i;
                                min = outweight;
                            }
                        }
                    }
                    assert(min_vert != -1);
                    part[min_vert] = 1;
                }
                if(group1 > group0) { // reset one 1 to a 0 in part: pick the least connected one!
                    min=std::numeric_limits<int>::max();
                    min_vert=-1;
                    for(unsigned int i=0; i<part.size(); ++i) {
                        if(part[i] == 1 && numCores[i] <= diff) {
                            int outweight = std::accumulate(phyTopGraph[i].begin(), phyTopGraph[i].end(), 0);
                            if(min > outweight) {
                                min_vert = i;
                                min = outweight;
                            }
                        }
                    }
                    assert(min_vert != -1);
                    part[min_vert] = 0;
                }
            }
        }

        // build two ptgs for the two partitions
        vector<vector<int> > ptg0, ptg1;
        vector<vector<int> > ptgc0, ptgc1;
        vector<int> pmap0, pmap1; // needed to translate ranks back to original graph
        vector<int> nprocs0, nprocs1;

        vector<int> newverts(phyTopGraph.size()); // translation table from old vertices to index in two new graphs
        int idx0=0, idx1=0;
        for(unsigned int i=0; i<phyTopGraph.size(); ++i) {
            if(part[i] == 0) { newverts[i] = idx0++; }
            if(part[i] == 1) { newverts[i] = idx1++; }
        }

        for(unsigned int i=0; i<part.size(); ++i) {
            if(part[i] == 0) {
                ptg0.resize(ptg0.size()+1);
                ptgc0.resize(ptgc0.size()+1);
                nprocs0.push_back(numCores[i]);
                pmap0.push_back(pmap[i]);
                for(unsigned int j=0; j<phyTopGraph[i].size(); ++j) {
                    // only add edges that are leading to a vertex in the same partition
                    if(part[i] == part[ phyTopGraph[i][j] ]) {
                        ptg0[ptg0.size()-1].push_back(newverts[phyTopGraph[i][j]]);
                        ptgc0[ptgc0.size()-1].push_back(phyTopGraph[i][j]);
                    }
                }
            }
            if(part[i] == 1) {
                ptg1.resize(ptg1.size()+1);
                ptgc1.resize(ptgc1.size()+1);
                nprocs1.push_back(numCores[i]);
                pmap1.push_back(pmap[i]);
                for(unsigned int j=0; j<phyTopGraph[i].size(); ++j) {
                    // only add edges that are leading to a vertex in the same partition
                    if(part[i] == part[ phyTopGraph[i][j] ]) {
                        ptg1[ptg1.size()-1].push_back(newverts[phyTopGraph[i][j]]);
                        ptgc1[ptgc1.size()-1].push_back(phyTopGraph[i][j]);
                    }
                }
            }
        }


        // partition logical topology graph //
        nvtxs = commGraph_map.size();
        idx_t nedges=0; for(unsigned int i=0; i<commGraph_map.size(); ++i) nedges += commGraph[ commGraph_map[i] ].size();

        // invert nodeGraph_map (nodeGraph graph) to translate adjacency lists to new graph
        int max = *std::max_element(commGraph_map.begin(),commGraph_map.end());
        vector<idx_t> invmap(max+1,-1);
        for(int i=0; i<nvtxs; ++i) invmap[ commGraph_map[i] ] = i;
        // end inversion

        xadj.resize(nvtxs+1);
        vwgt.resize(nvtxs+1);
        adjncy.resize(nedges+1);
        adjwgt.resize(nedges+1);
        part.resize(nvtxs);

        nparts = 2;
        edgecut = 0;

        idx_t offset = 0;
        idx_t adjoff = 0;
        for(int i=0; i<nvtxs; ++i) {
            xadj[i] = offset;
            // compute the right offset (only including edges in our group)
            for(unsigned int j=0; j<commGraph[ commGraph_map[i] ].size(); ++j) if(commGraph[ commGraph_map[i] ][j] <= max && invmap[ commGraph[ commGraph_map[i] ][j] ] != -1) offset++;

            vwgt[i] = 1;

            for(unsigned int j=0; j<commGraph[ commGraph_map[i] ].size(); ++j) {
                // see phyTopGraph for comments about datastructures
                int dst = -1;
                if(commGraph[ commGraph_map[i] ][j] <= max) { // if the destination is > max then it's not in our set!
                    dst = invmap[ commGraph[ commGraph_map[i] ][j] ];
                }
                // -1 are the destinations outside our group! Exclude them!
                if(dst != -1) {
                    adjncy[adjoff] = dst;
                    adjwgt[adjoff] = commWeights[ commGraph_map[i] ][j];
                    adjoff++;
                }
            }
        }
        xadj[nvtxs] = offset;

        METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL, &adjwgt[0], &nparts, NULL, NULL, NULL, &edgecut, &part[0]);

        // check if it's balanced or not -- correct if necessary
        diff = 2; // enter at least once
        while(diff > 1) {
            int group1 = std::accumulate(part.begin(), part.end(), 0); // number of vertices in the second group
            int group0 = part.size()-group1;
            diff = group0-group1;
            if(diff < 0) diff *= -1;
            if(diff > 1) { //it's not balanced!
                int min_vert, min;
                if(group0 > group1) { // reset one 0 to a 1 in part: pick the least connected one!
                    min=std::numeric_limits<int>::max();
                    min_vert=-1;
                    for(unsigned int i=0; i<part.size(); ++i) {
                        if(part[i] == 0) {
                            int outweight = std::accumulate(commWeights[ commGraph_map[i] ].begin(), commWeights[ commGraph_map[i] ].end(), 0);
                            if(min > outweight) {
                                min_vert = i;
                                min = outweight;
                            }
                        }
                    }
                    part[min_vert] = 1;
                }
                if(group1 > group0) { // reset one 0 to a 1 in part: pick the least connected one!
                    min=std::numeric_limits<int>::max();
                    min_vert=-1;
                    for(unsigned int i=0; i<part.size(); ++i) {
                        if(part[i] == 1) {
                            int outweight = std::accumulate(commWeights[ commGraph_map[i] ].begin(), commWeights[ commGraph_map[i] ].end(), 0);
                            if(min > outweight) {
                                min_vert = i;
                                min = outweight;
                            }
                        }
                    }
                    part[min_vert] = 0;
                }
            }
        }

        vector<int> lmap1, lmap2;
        // fill new Mapping vectors
        for(unsigned int i=0; i<part.size(); ++i) {
            // the i-th entry in part is the i-th entry in nodeGraph_map
            if(part[i] == 0) {
                lmap1.push_back( commGraph_map[i] );
            } else {
                lmap2.push_back( commGraph_map[i] );
            }
        }

        // save memory in recursion!
        xadj.resize(0);
        vwgt.resize(0);
        adjncy.resize(0);
        adjwgt.resize(0);
        part.resize(0);
        invmap.resize(0);

        // call recursively (two subtrees)
        // check which pmap fits which lmap size (might be two different if uneven group size)
        unsigned int nprocs0_size = std::accumulate(nprocs0.begin(), nprocs0.end(), 0);
        unsigned int nprocs1_size = std::accumulate(nprocs1.begin(), nprocs1.end(), 0);

        if(lmap1.size() == nprocs0_size && lmap2.size() == nprocs1_size) {
            mapRecursive(&ptg0, &ptgc0, &nprocs0, commGraph_ref, weights_ref, mapping_ref, lmap1, &pmap0);
            mapRecursive(&ptg1, &ptgc1, &nprocs1, commGraph_ref, weights_ref, mapping_ref, lmap2, &pmap1);
        } else if(lmap1.size() == nprocs1_size && lmap2.size() == nprocs0_size) {
            mapRecursive(&ptg1, &ptgc1, &nprocs1, commGraph_ref, weights_ref, mapping_ref, lmap1, &pmap1);
            mapRecursive(&ptg0, &ptgc0, &nprocs0, commGraph_ref, weights_ref, mapping_ref, lmap2, &pmap0);
        } else {
            schedout.fatal(CALL_INFO, 1, "TopoMap recursive mapper partitions are not equal.");
        }

    } else {
        // end of recursion -- finalize mapping
        int index = 0; while(numCores[index] == 0) index++;
        mapping[ commGraph_map[0] ] = pmap[index];
    }
#else
    schedout.fatal(CALL_INFO, 1, "TopoMap recursive requires METIS library.");
#endif
    return 0;
}

