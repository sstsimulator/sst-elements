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


#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "misc.h"
#include "output.h"

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define ABS(X) ((X) >= 0 ? (X) : (-(X)))

using namespace SST::Scheduler;

/**
 * The default ordering for MeshLocations is by the component: x, y, then z.
 * Comparator used to order free blocks in MBSAllocator.
 */

MeshLocation::MeshLocation(int X, int Y, int Z) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    x = X;
    y = Y;
    z = Z;
}

MeshLocation::MeshLocation(int inpos, MachineMesh* m) 
{
    //return x + m -> getXDim() * y + m -> getXDim() * m -> getYDim() * z; 

    schedout.init("", 8, 0, Output::STDOUT);
    z = inpos / (m -> getXDim() * m -> getYDim());
    inpos -= z * m -> getXDim() * m -> getYDim();
    y = inpos / m -> getXDim();
    inpos -= y * m -> getXDim();
    x = inpos;
}


MeshLocation::MeshLocation(MeshLocation* in)
{
    schedout.init("", 8, 0, Output::STDOUT);
    //copy constructor
    x = in -> x;
    y = in -> y;
    z = in -> z;
}

int MeshLocation::L1DistanceTo(MeshLocation* other) 
{
    return ABS(x - other -> x) + ABS(y - other -> y) + ABS(z - other -> z);
}

int MeshLocation::LInfDistanceTo(MeshLocation* other) 
{
    return MAX(ABS(x - other -> x), MAX(ABS(y - other -> y), ABS(z - other -> z)));
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

bool MeshLocation::equals(MeshLocation* other) {
    return x == other -> x && y == other -> y && z == other -> z;
}

void MeshLocation::print() {
    //printf("(%d,%d,%d)\n",x,y,z);
    schedout.output("(%d,%d,%d)\n", x, y, z);
}


int MeshLocation::toInt(MachineMesh* m){
    return x + m -> getXDim() * y + m -> getXDim() * m -> getYDim() * z; 
}

std::string MeshLocation::toString(){
    std::stringstream ret;
    ret << "(" << x <<  ", " << y  << ", " << z << ")";
    return ret.str();
}

int MeshLocation::hashCode() {
    return x + 31 * y + 961 * z;
}

MeshAllocInfo::MeshAllocInfo(Job* j) : AllocInfo(j) 
{
    processors = new std::vector<MeshLocation*>();
    for (int x = 0; x < j -> getProcsNeeded(); x++) {
        nodeIndices[x] = -1;
    }
    for (int x = 0; x < j -> getProcsNeeded(); x++) {
        processors -> push_back(NULL);
    }
}

MeshAllocInfo::~MeshAllocInfo()
{
    /*
     *note that the MeshLocations in MeshAllocInfo are assumed to be unique!
     *in other words, they were created solely for the purpose of storing a
     *location of a processor in this MAI.  All current allocators except MBS
     *use machine->getprocessors() to get these processors; this function 
     *  creates new MeshLocations so it works
     */
    for (int x = 0; x < (int)processors -> size(); x++) {
        delete processors -> at (x);
    }
    processors -> clear();
}

std::string MeshAllocInfo::getProcList(Machine* m)
{
    std::string ret="";
    MachineMesh* mesh = (MachineMesh*) m;
    //if (NULL == m) {
    //    error("MeshAllocInfo requires Mesh machine");
    //}
    for (std::vector<MeshLocation*>::iterator ml = processors -> begin(); ml != processors->end(); ++ml) {
        ret += (*ml) -> x + mesh -> getXDim() * (*ml) -> y + mesh -> getXDim() * mesh -> getYDim()*(*ml) -> z + ",";
    }
    return ret;	
}


