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
    int numNodes = ceil( (double) job->getProcsNeeded() / coresPerNode);
    int xSize, ySize, zSize;
    //dimensions if we fit job in a cube
    xSize = (int)ceil( (float)cbrt((float)numNodes) ); 
    ySize = xSize;
    zSize = xSize;
    //restrict dimensions
    if(xSize > xdim) {
        xSize = xdim;
        ySize = (int)ceil( (float)std::sqrt( ((float)numNodes) / xdim ) );
        zSize = ySize;
        if( ySize > ydim ) {
            ySize = ydim;
            zSize = (int)ceil( ((float)numNodes) / (xdim * ydim) );
        } else if ( zSize > zdim ) {
            zSize = zdim;
            ySize = (int)ceil( ((float)numNodes) / (xdim * zdim) );
        }
    } else if(ySize > ydim) {
        ySize = ydim;
        xSize = (int)ceil( (float)std::sqrt( ((float)numNodes) / ydim ) );
        zSize = xSize;
        if( xSize > xdim ) {
            xSize = xdim;
            zSize = (int)ceil( ((float)numNodes) / (xdim * ydim) );
        } else if ( zSize > zdim ) {
            zSize = zdim;
            xSize = (int)ceil( ((float)numNodes) / (ydim * zdim) );
        }
    } else if(zSize > zdim) {
        zSize = zdim;
        ySize = (int)ceil( (float)std::sqrt( ((float)numNodes) / zdim ) );
        xSize = ySize;
        if( ySize > ydim ){
            ySize = ydim;
            xSize = (int)ceil( ((float)numNodes) / (zdim * ydim) );
        } else if ( xSize > xdim ) {
            xSize = xdim;
            ySize = (int)ceil( ((float)numNodes) / (xdim * zdim) );
        }
    }
    //order dimensions from shortest to longest
    int state = 0; //keeps order mapping
    if(xSize <= ySize && ySize <= zSize) {
        state = 0;
    } else if(ySize <= xSize && xSize <= zSize) {
        state = 1;
        std::swap(xSize, ySize);
    } else if(zSize <= ySize && ySize <= xSize) {
        state = 2;
        std::swap(xSize, zSize);
    } else if(xSize <= zSize && zSize <= ySize) {
        state = 3;
        std::swap(zSize, ySize);
    } else if(ySize <= zSize && zSize <= xSize) {
        state = 4;
        std::swap(xSize, ySize);
        std::swap(ySize, zSize);
    } else if(zSize <= xSize && xSize <= ySize) {
        state = 5;
        std::swap(xSize, ySize);
        std::swap(xSize, zSize);
    }
   
    //Fill given space with snake-shaped order, use shortest dim first
    int nodeCount = 0;
    bool done = false;
    bool xFwd = true;
    bool yFwd = true;
    int i, j;
    std::vector<MeshLocation> nodes;
    for(int k = 0; k < zSize && !done; k++){
        for(int yVal = 0; yVal < ySize && !done; yVal++){
            if(yFwd){
                j = yVal;
            } else {
                j = ySize - yVal - 1;
            }
            for(int xVal = 0; xVal < xSize && !done; xVal++){
                if(xFwd){
                    i = xVal;
                } else {
                    i = xSize - xVal - 1;
                }
                //use state not to mix dimension of the actual machine
                switch(state) {
                case 0: nodes.push_back(MeshLocation(i,j,k)); break;
                case 1: nodes.push_back(MeshLocation(j,i,k)); break;
                case 2: nodes.push_back(MeshLocation(k,j,i)); break;
                case 3: nodes.push_back(MeshLocation(i,k,j)); break;
                case 4: nodes.push_back(MeshLocation(k,i,j)); break;
                case 5: nodes.push_back(MeshLocation(j,k,i)); break;
                default: schedout.fatal(CALL_INFO, 0, "Unexpected error.\n");
                }
                nodeCount++;
                if(nodeCount == numNodes){
                    done = true;
                }
            }
            xFwd = !xFwd;
        }
        yFwd = !yFwd;
    }
    //create allocInfo
    AllocInfo* allocInfo = new AllocInfo(job, *this);
    for(int i = 0; i < numNodes; i++){
        allocInfo->nodeIndices[i] = nodes.at(i).toInt(*this);
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
                schedout.fatal(CALL_INFO, 1, "Link index requested for an corner node.\n");
            }
            linkNo = x + y * (xdim - 1) + z * ydim * (xdim - 1);
            break;
        case 1:
            if(y == (ydim - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for an corner node.\n");
            }
            //add all x links
            linkNo = zdim * ydim * (xdim - 1);
            linkNo += x + y * xdim + z * xdim * (ydim - 1);
            break;
        case 2:
            if(z == (zdim - 1)){
                schedout.fatal(CALL_INFO, 1, "Link index requested for an corner node.\n");
            }
            //add all x and y links
            linkNo = zdim * (2 * xdim * ydim - ydim - xdim);
            linkNo += x + y * xdim + z * xdim * ydim;
            break;
        default:
        linkNo = -1;
            schedout.fatal(CALL_INFO, 1, "Link index requested for non-existing dimension.\n");
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
