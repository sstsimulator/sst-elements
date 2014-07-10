// Copyright 2009-2013 Sandia Corporation. Under the terms // of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Classes representing information about an allocation for Mesh
 * Machines
 */

#include "AllocInfo.h"
#include "Job.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"

using namespace SST::Scheduler;

MeshAllocInfo::MeshAllocInfo(Job* j, const Machine & mach) : AllocInfo(j, mach) 
{
    nodes = new std::vector<MeshLocation*>();
    for (int x = 0; x < j -> getProcsNeeded(); x++) {
        nodeIndices[x] = -1;
    }
    for (int x = 0; x < j -> getProcsNeeded(); x++) {
        nodes -> push_back(NULL);
    }
}

MeshAllocInfo::~MeshAllocInfo()
{
    /*
     *note that the MeshLocations in MeshAllocInfo are assumed to be unique!
     *in other words, they were created solely for the purpose of storing a
     *location of a node in this MAI.  All current allocators except MBS
     *use machine->getNodes() to get these nodes; this function 
     *  creates new MeshLocations so it works
     */
    for (int x = 0; x < (int)nodes -> size(); x++) {
        delete nodes -> at (x);
    }
    nodes -> clear();
}

std::string MeshAllocInfo::getNodeList(Machine* m)
{
    std::string ret="";
    MeshMachine* mesh = (MeshMachine*) m;
    for (std::vector<MeshLocation*>::iterator ml = nodes -> begin(); ml != nodes->end(); ++ml) {
        ret += (*ml) -> x + mesh -> getXDim() * (*ml) -> y + mesh -> getXDim() * mesh -> getYDim()*(*ml) -> z + ",";
    }
    return ret;	
}

AllocInfo* MeshAllocInfo::getBaselineAllocation(const MeshMachine & mach, Job* job)
{
    int xdim = mach.getXDim();
    int ydim = mach.getYDim();
    int zdim = mach.getZDim();
    int numNodes = ceil( (double) job->getProcsNeeded() / mach.getNumCoresPerNode());
    
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
    AllocInfo* allocInfo = new AllocInfo(job, mach);
    for(int i = 0; i < numNodes; i++){
        allocInfo->nodeIndices[i] = nodes.at(i).toInt(mach);
    }
    return allocInfo;
}


