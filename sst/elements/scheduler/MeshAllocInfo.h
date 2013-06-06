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
 * Classes representing information about an allocation for Mesh
 * Machines
 */

#ifndef __MESHALLOCINFO_H__
#define __MESHALLOCINFO_H__

#include <vector>
#include <sstream>
using namespace std;

#include "MachineMesh.h"
#include "misc.h"
#include "sst/core/serialization/element.h"

#define MAX(X,Y) ((X)>(Y)?(X):(Y))
#define ABS(X) ((X) >=0 ? (X) : (-(X)))

namespace SST {
    namespace Scheduler {

        /**
         * The default ordering for MeshLocations is by the component: x, y, then z.
         * Comparator used to order free blocks in MBSAllocator.
         */

        class MeshLocation : public binary_function<SST::Scheduler::MeshLocation*, SST::Scheduler::MeshLocation*,bool>{

            public:
                int x;
                int y;
                int z;

                MeshLocation(int X, int Y, int Z) 
                {
                    x = X;
                    y = Y;
                    z = Z;
                }

                MeshLocation(MeshLocation* in)
                {
                    //copy constructor
                    x = in -> x;
                    y = in -> y;
                    z = in -> z;
                }

                int L1DistanceTo(MeshLocation* other) 
                {
                    return ABS(x - other -> x) + ABS(y - other -> y) + ABS(z - other -> z);
                }

                int LInfDistanceTo(MeshLocation* other) 
                {
                    return MAX(ABS(x - other -> x), MAX(ABS(y - other -> y), ABS(z - other -> z)));
                }

                bool operator()(MeshLocation* loc1, MeshLocation* loc2)
                {
                    if (loc1 -> x == loc2 -> x){
                        if (loc1 -> y == loc2 -> y) {
                            return loc1 -> z < loc2 -> z;
                        }
                        return loc1 -> y < loc2 -> y;
                    }
                    return loc1 -> x < loc2 -> x;
                }

                bool equals(MeshLocation* other) {
                    return x == other -> x && y == other -> y && z == other -> z;
                }

                void print() {
                    printf("(%d,%d,%d)\n",x,y,z);
                }


                int toInt(MachineMesh* m){
                    return x + m -> getXDim() * y + m -> getXDim() * m -> getYDim() * z; 
                }

                string toString(){
                    stringstream ret;
                    ret << "(" << x <<  ", " << y  << ", " << z << ")";
                    return ret.str();
                }

                int hashCode() {
                    return x + 31 * y + 961 * z;
                }
        };

        class MeshAllocInfo : public AllocInfo {
            public:

                vector<MeshLocation*>* processors;
                MeshAllocInfo(Job* j) : AllocInfo(j) 
            {
                processors = new vector<MeshLocation*>();
                for (int x = 0; x < j -> getProcsNeeded(); x++) {
                    nodeIndices[x] = -1;
                }
                for (int x = 0; x < j -> getProcsNeeded(); x++) {
                    processors -> push_back(NULL);
                }
            }
                ~MeshAllocInfo()
                {
                    /*
                     *note that the MeshLocations in MeshAllocInfo are assumed to be unique!
                     *in other words, they were created solely for the purpose of storing a
                     *location of a processor in this MAI.  All current allocators except MBS
                     *use machine->getprocessors() to get these processors; this function 
                     *  creates new MeshLocations so it works
                     */
                    for (int x = 0; x < (int)processors -> size(); x++) {
                        delete processors->at (x);
                    }
                    processors -> clear();
                }

                string getProcList(Machine* m)
                {
                    string ret="";
                    MachineMesh* mesh = (MachineMesh*) m;
                    if (NULL == m) {
                        error("MeshAllocInfo requires Mesh machine");
                    }
                    for (vector<MeshLocation*>::iterator ml = processors -> begin(); ml != processors->end(); ++ml) {
                        ret += (*ml) -> x + mesh -> getXDim() * (*ml) -> y + mesh -> getXDim() * mesh -> getYDim()*(*ml) -> z+",";
                    }
                    return ret;	
                }

        };

    }
}

#endif
