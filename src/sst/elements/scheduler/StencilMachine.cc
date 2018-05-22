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

#include <algorithm>
#include <vector>
#include <cmath>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "StencilMachine.h"

using namespace SST::Scheduler;

StencilMachine::StencilMachine(std::vector<int> inDims, int numLinks, int numCoresPerNode, double** D_matrix)
                         : Machine(constHelper(inDims), numCoresPerNode, D_matrix, numLinks),
                           dims(inDims)
{
}

std::string StencilMachine::getParamHelp()
{
    return "[<x dim>,<y dim>,...]\n\t Mesh or torus with specified dimensions";
}

int StencilMachine::constHelper(std::vector<int> dims)
{
    int numNodes = 1;
    for(unsigned int i = 0; i < dims.size() ; i++){
        numNodes *= dims[i];
    }
    return numNodes;
}

int StencilMachine::coordOf(int node, int dim) const
{
    MeshLocation loc(node, *this);
    return loc.dims[dim];
}

int StencilMachine::indexOf(std::vector<int> dims) const
{
    MeshLocation loc(dims);
    return loc.toInt(*this);
}

AllocInfo* StencilMachine::getBaselineAllocation(Job* job) const
{
    std::vector<int> machDims(dims);
    int nodesNeeded = (int) ceil((float) job->getProcsNeeded() / coresPerNode);
    if(nodesNeeded > numNodes){
        schedout.fatal(CALL_INFO, 1, "Baseline allocation requested for %d nodes for a %d-node machine.", nodesNeeded, numNodes);
    }

    //optimization: return here if one node
    if(nodesNeeded == 1){
        AllocInfo* ai = new AllocInfo(job, *this);
        ai->nodeIndices[0] = 0;
        return ai;
    }

    //dimensions if we fit job in a cube
    int cubicDim = (int)ceil( (float) pow((float)nodesNeeded, 1.0/dims.size()) );

    //restrict dimensions
    for(unsigned int i = 0; i < dims.size(); i++){
        if(cubicDim < machDims[i]){
            machDims[i] = cubicDim;
        }
    }

    int readyNodes = 1;
    for(unsigned int i = 0; i < dims.size(); i++){
        readyNodes *= machDims[i];
    }

    //make sure there are sufficient nodes
    //increase the dimensions starting from the first
    for(unsigned int i = 0; i < dims.size() && readyNodes < nodesNeeded; i++){
        while(machDims[i] < dims[i] && readyNodes < nodesNeeded){
            readyNodes = readyNodes / machDims[i] * (machDims[i] + 1);
            machDims[i]++;
        }
    }

    //Fill given space with snake-shaped order
    AllocInfo* allocInfo = new AllocInfo(job, *this);
    std::vector<int> curLocation(machDims.size(), 0);
    std::vector<bool> ifFwd(machDims.size(), true); //whether to go forward
    int curDim = 0; //currently moving direction
    readyNodes = 0;
    //find the last dimension with multiple nodes
    int lastDim = machDims.size() - 1;
    while(lastDim >= 0 && machDims[lastDim] == 1){
        lastDim--;
    }
    //fill
    while(readyNodes < nodesNeeded){
        //add current location
        MeshLocation curMeshLoc(curLocation);
        allocInfo->nodeIndices[readyNodes] = curMeshLoc.toInt(*this);
        readyNodes++;
        //check if last location
        if(readyNodes == nodesNeeded){
            break;
        }
        //move to next location
        //check if we need to change the dimension
        while(  ( ifFwd[curDim] && curLocation[curDim] == (machDims[curDim] - 1))
             || (!ifFwd[curDim] && curLocation[curDim] == 0) ){
            //we are at the end of a dimension
            //change this dimensions direction
            ifFwd[curDim] = !ifFwd[curDim];
            //check next dimension
            if(curDim != lastDim){
                curDim++;
            } else {
                break;
            }
        }
        //return to the current dimension
        while(curDim >= 0){
            if(machDims[curDim] != 1) {
                if(ifFwd[curDim]) curLocation[curDim] += 1;
                else              curLocation[curDim] -= 1;
                break;
            }
            curDim--;
        }
        curDim = 0;
    }
    return allocInfo;
}

MeshLocation::MeshLocation(std::vector<int> inDims)
{
    schedout.init("", 8, 0, Output::STDOUT);
    dims = inDims;
}

MeshLocation::MeshLocation(int inpos, const StencilMachine & m)
{
    dims.resize(m.numDims());
    int stepSize = 1;
    for(int i = 0; i < m.numDims(); i++){
        stepSize *= m.dims[i];
    }
    for(int i = (m.numDims() - 1); i >= 0; i--){
        stepSize /= m.dims[i];
        dims[i] = inpos / stepSize;
        inpos %= stepSize;
    }
    schedout.init("", 8, 0, Output::STDOUT);
}

MeshLocation::MeshLocation(const MeshLocation & in)
{
    schedout.init("", 8, 0, Output::STDOUT);
    dims = in.dims;
    //copy constructor
}

int MeshLocation::L1DistanceTo(const MeshLocation & other) const
{
    int distance = 0;
    for(unsigned int i = 0; i < dims.size() ; i++){
        distance += abs(dims[i] - other.dims[i]);
    }
    return distance;
}

int MeshLocation::LInfDistanceTo(const MeshLocation & other) const
{
    int distance = 0;
    for(unsigned int i = 0; i < dims.size() ; i++){
        int tempDist = abs(dims[i] - other.dims[i]);
        distance = std::max(distance, tempDist);
    }
    return distance;
}

bool MeshLocation::operator()(MeshLocation* loc1, MeshLocation* loc2)
{
    for(unsigned int i = 0; i < dims.size() ; i++){
        if(loc1->dims[i] != loc2->dims[i]){
            return loc1->dims[i] < loc2->dims[i];
        }
    }
    return 0;
}

bool MeshLocation::equals(const MeshLocation & other) const
{
    bool equal = 1;
    for(unsigned int i = 0; i < dims.size() ; i++){
        equal &= (dims[i] == other.dims[i]);
    }
    return equal;
}

int MeshLocation::toInt(const StencilMachine & m)
{
    int stepSize = 1;
    int nodeID = 0;
    for(unsigned int i = 0; i < dims.size() ; i++){
        nodeID += dims[i] * stepSize;
        stepSize *= m.dims[i];
    }
    return nodeID;
}

std::string MeshLocation::toString()
{
    std::stringstream ret;
    ret << "(";
    for(unsigned int i = 0; i < (dims.size() - 1); i++){
        ret << dims[i] << ", ";
    }
    ret << dims.back() << ")";
    return ret.str();
}
