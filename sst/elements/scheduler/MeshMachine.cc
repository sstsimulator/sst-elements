// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Machine based on a mesh structure
 */

#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <utility>

#include "sst_config.h"
#include "sst/core/serialization.h"

#include "Allocator.h"
#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "output.h"

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

using namespace SST::Scheduler;

MeshMachine::MeshMachine(int Xdim, int Ydim, int Zdim, int numCoresPerNode, double** D_matrix)
                         : Machine((long) Xdim*Ydim*Zdim,
                                   numCoresPerNode,
                                   D_matrix,
                                   3*Xdim*Ydim*Zdim - Xdim*Ydim - Xdim*Zdim - Ydim*Zdim)
{
    schedout.init("", 8, 0, Output::STDOUT);
    xdim = Xdim;
    ydim = Ydim;
    zdim = Zdim;
}

std::string MeshMachine::getParamHelp() 
{
    return "[<x dim>,<y dim>,<z dim>]\n\t3D Mesh with specified dimensions";
}

std::string MeshMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) com="# ";
    else com="";
    std::stringstream ret;
    ret << com << xdim << "x" << ydim << "x" << zdim << " Mesh";
    ret << ", " << coresPerNode << " cores per node";
    return ret.str();
}

unsigned int MeshMachine::getNodeDistance(int node1, int node2) const
{
    MeshLocation loc1 = MeshLocation(node1, *this);
    MeshLocation loc2 = MeshLocation(node2, *this);
    std::vector<MeshLocation*>* locs = new std::vector<MeshLocation*>();
    locs->push_back(&loc1);
    locs->push_back(&loc2);
    long dist = pairwiseL1Distance(locs);
    delete locs;
    return dist;
}

std::list<int>* MeshMachine::getFreeAtL1Distance(int center, int dist) const
{
    int centerX = xOf(center);
    int centerY = yOf(center);
    int centerZ = zOf(center);
    std::list<int>* nodeList = new std::list<int>();
    if(dist < 1){
        return nodeList;
    }
    for(int x = std::max(centerX - dist, 0); x <= std::min(centerX + dist, xdim - 1); x++){
        int yRange = dist - abs(centerX - x);
        for(int y = std::max(centerY - yRange, 0); y <= std::min(centerY + yRange, ydim - 1); y++){
            int zRange = yRange - abs(centerY - y);
            int z = centerZ - zRange;
            appendIfFree(x, y, z, nodeList);
            if(zRange != 0){
                z = centerZ + zRange;
                appendIfFree(x, y, z, nodeList);
            }
        }
    }
    return nodeList;
}

std::list<int>* MeshMachine::getFreeAtLInfDistance(int center, int dist) const
{
    const int centerX = xOf(center);
    const int centerY = yOf(center);
    const int centerZ = zOf(center);
    int x, y, z;
    std::list<int>* nodeList = new std::list<int>();
    if(dist < 1){
        return nodeList;
    }
    
    //first scan the sides
    
    //go backward in x
    x = centerX - dist;
    //scan all y except edges & corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            z = centerZ + zDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    //go backward in y
    y = centerY - dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            z = centerZ + zDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    //go backward in z
    z = centerZ - dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        //scan all y except edges & corners
        for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
            y = centerY + yDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    //go forward in z
    z = centerZ + dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        //scan all y except edges & corners
        for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
            y = centerY + yDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    //go forward in y
    y = centerY + dist;
    //scan all x except edges & corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            z = centerZ + zDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    //go forward in x
    x = centerX + dist;
    //scan all y except edges & corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        //scan all z except edges & corners
        for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
            z = centerZ + zDist;
            appendIfFree(x, y, z, nodeList);
        }
    }
    
    //now do edges
    
    //backward in x
    x = centerX - dist;
    //backward in y
    y = centerY - dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        z = centerZ + zDist;
        appendIfFree(x, y, z, nodeList);
    }
    //backward in z
    z = centerZ - dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in z
    z = centerZ + dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in y
    y = centerY + dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        z = centerZ + zDist;
        appendIfFree(x, y, z, nodeList);
    }
    
    //backward in y
    y = centerY - dist;
    //backward in z
    z = centerZ - dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in z
    z = centerZ + dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in y
    y = centerY + dist;
    //backward in z
    z = centerZ - dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in z
    z = centerZ + dist;
    //scan all x except corners
    for(int xDist = -(dist - 1); xDist <= (dist - 1); xDist++){
        x = centerX + xDist;
        appendIfFree(x, y, z, nodeList);
    }
    
    //forward in x
    x = centerX + dist;
    //backward in y
    y = centerY - dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        z = centerZ + zDist;
        appendIfFree(x, y, z, nodeList);
    }
    //backward in z
    z = centerZ - dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in z
    z = centerZ + dist;
    //scan all y except corners
    for(int yDist = -(dist - 1); yDist <= (dist - 1); yDist++){
        y = centerY + yDist;
        appendIfFree(x, y, z, nodeList);
    }
    //forward in y
    y = centerY + dist;
    //scan all z except corners
    for(int zDist = -(dist - 1); zDist <= (dist - 1); zDist++){
        z = centerZ + zDist;
        appendIfFree(x, y, z, nodeList);
    }
    
    //now do corners
    
    //backward in x
    x = centerX - dist;
    y = centerY - dist; //backward in y
    z = centerZ - dist; //backward in z
    appendIfFree(x, y, z, nodeList);
    z = centerZ + dist; //forward in z
    appendIfFree(x, y, z, nodeList);
    y = centerY + dist; //forward in y
    z = centerZ - dist; //backward in z
    appendIfFree(x, y, z, nodeList);
    z = centerZ + dist; //forward in z
    appendIfFree(x, y, z, nodeList);
    
    //forward in x
    x = centerX + dist;
    y = centerY - dist; //backward in y
    z = centerZ - dist; //backward in z
    appendIfFree(x, y, z, nodeList);
    z = centerZ + dist; //forward in z
    appendIfFree(x, y, z, nodeList);
    y = centerY + dist; //forward in y
    z = centerZ - dist; //backward in z
    appendIfFree(x, y, z, nodeList);
    z = centerZ + dist; //forward in z
    appendIfFree(x, y, z, nodeList);
    
    return nodeList;
}

void MeshMachine::appendIfFree(int x, int y, int z, std::list<int>* nodeList) const
{
    if(x >= 0 && x < xdim && y >= 0 && y < ydim && z >= 0 && z < zdim){
        int tempNode = indexOf(x,y,z);
        if(isFree(tempNode)){
            nodeList->push_back(tempNode);
        }
    }
}

unsigned int MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs) const
{
    int num = locs -> size();
    //returns total pairwise L_1 distance between 1st num members of array
    long retVal = 0;

    for (int i = 0; i < num; i++) {
        for (int j = i + 1; j < num; j++) {
            retVal += ((*locs)[i])->L1DistanceTo(*((*locs)[j]));
        }
    }

    return retVal;
}

AllocInfo* MeshMachine::getBaselineAllocation(Job* job)
{
    std::vector<int> dims(3);
    dims[0] = xdim;
    dims[1] = ydim;
    dims[2] = zdim;

    std::vector<int> machDims(dims);
    int nodesNeeded = (int) ceil((float) job->getProcsNeeded() / coresPerNode);
    if(nodesNeeded > numNodes){
        schedout.fatal(CALL_INFO, 1, "Baseline allocation requested for %d nodes for a %ld-node machine.", nodesNeeded, numNodes);
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
        MeshLocation curMeshLoc(curLocation[0], curLocation[1], curLocation[2]);
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

unsigned int MeshMachine::getLinkIndex(int x, int y, int z, int dimension) const
{
    int linkNo;
    //link order: first all links in x dimension (same ordering with nodes), then y, then z
    switch(dimension){
        case 0:
            if(x == (xdim - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            linkNo = x + y * (xdim - 1) + z * ydim * (xdim - 1);
            break;
        case 1:
            if(y == (ydim - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            //add all x links
            linkNo = zdim * ydim * (xdim - 1);
            linkNo += x + y * xdim + z * xdim * (ydim - 1);
            break;
        case 2:
            if(z == (zdim - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for a corner node.\n");
            }
            //add all x and y links
            linkNo = zdim * (2 * xdim * ydim - ydim - xdim);
            linkNo += x + y * xdim + z * xdim * ydim;
            break;
        default:
            schedout.fatal(CALL_INFO, 1, "Link index requested for non-existing dimension.\n");
            linkNo = 0; //avoids compilation warning
    }
    return linkNo;
}

std::vector<unsigned int> MeshMachine::getRoute(int node0, int node1, double commWeight) const
{
    std::vector<unsigned int> links(0);
    int x0 = xOf(node0);
    int x1 = xOf(node1);
    int y0 = yOf(node0);
    int y1 = yOf(node1);
    int z0 = zOf(node0);
    int z1 = zOf(node1);
    //add X route
    for(int x = std::min(x0, x1); x < std::max(x0, x1); x++){
        links.push_back(getLinkIndex(x, y0, z0, 0));
    }
    //add Y route
    for(int y = std::min(y0, y1); y < std::max(y0, y1); y++){
        links.push_back(getLinkIndex(x1, y, z0, 1));
    }
    //add Z route
    for(int z = std::min(z0, z1); z < std::max(z0, z1); z++){
        links.push_back(getLinkIndex(x1, y1, z, 2));
    }
    return links;
}

MeshLocation::MeshLocation(int X, int Y, int Z) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    x = X;
    y = Y;
    z = Z;
}

MeshLocation::MeshLocation(int inpos, const MeshMachine & m) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    z = inpos / (m.getXDim() * m.getYDim());
    inpos -= z * m.getXDim() * m.getYDim();
    y = inpos / m.getXDim();
    inpos -= y * m.getXDim();
    x = inpos;
}


MeshLocation::MeshLocation(const MeshLocation & in)
{
    schedout.init("", 8, 0, Output::STDOUT);
    //copy constructor
    x = in.x;
    y = in.y;
    z = in.z;
}

int MeshLocation::L1DistanceTo(const MeshLocation & other) const
{
    return abs(x - other.x) + abs(y - other.y) + abs(z - other.z);
}

int MeshLocation::LInfDistanceTo(const MeshLocation & other) const
{
    return MAX(abs(x - other.x), MAX(abs(y - other.y), abs(z - other.z)));
}

bool MeshLocation::operator()(MeshLocation* loc1, MeshLocation* loc2)
{
    if (loc1 -> x == loc2 -> x){
        if (loc1 -> y == loc2 -> y) {
            return loc1 -> z < loc2 -> z;
        }
        return loc1 -> y < loc2 -> y;
    }
    return loc1 -> x < loc2 -> x;
}

bool MeshLocation::equals(const MeshLocation & other) const 
{
    return x == other.x && y == other.y && z == other.z;
}

int MeshLocation::toInt(const MeshMachine & m){
    return x + m.getXDim() * y + m.getXDim() * m.getYDim() * z; 
}

std::string MeshLocation::toString(){
    std::stringstream ret;
    ret << "(" << x <<  ", " << y  << ", " << z << ")";
    return ret.str();
}
