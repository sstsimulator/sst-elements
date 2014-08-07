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
                         : Machine((long) Xdim*Ydim*Zdim, numCoresPerNode, D_matrix)
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

long MeshMachine::getNodeDistance(int node1, int node2) const
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

long MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs) const
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
   
    //Fill given space, use shortest dim first
    int nodeCount = 0;
    bool done = false;
    std::vector<MeshLocation> nodes;
    for(int k = 0; k < zSize && !done; k++){
        for(int j = 0; j < ySize && !done; j++){
            for(int i = 0; i < xSize && !done; i++){
                //use state not to mix dimension of the actual machine
                switch(state) {
                case 0: nodes.push_back(MeshLocation(i,j,k)); break;
                case 1: nodes.push_back(MeshLocation(j,i,k)); break;
                case 2: nodes.push_back(MeshLocation(k,j,i)); break;
                case 3: nodes.push_back(MeshLocation(i,k,j)); break;
                case 4: nodes.push_back(MeshLocation(k,i,j)); break;
                case 5: nodes.push_back(MeshLocation(j,k,i)); break;
                default: schedout.fatal(CALL_INFO, 0, "Unexpected error.");
                }
                nodeCount++;
                if(nodeCount == numNodes){
                    done = true;
                }
            }
        }
    }
    
    //create allocInfo
    AllocInfo* allocInfo = new AllocInfo(job, *this);
    for(int i = 0; i < numNodes; i++){
        allocInfo->nodeIndices[i] = nodes.at(i).toInt(*this);
    }
    return allocInfo;
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
