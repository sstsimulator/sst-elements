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
 * Abstract base class for machines based on a mesh structure
 */

#ifndef SST_SCHEDULER_MESHMACHINE_H__
#define SST_SCHEDULER_MESHMACHINE_H__

#include <string>
#include <vector>

#include "Machine.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class MeshLocation;

        class MeshMachine : public Machine {

            private:
                int xdim;   //size of mesh in each dimension
                int ydim;
                int zdim;
                
            public:
                
                MeshMachine(int Xdim, int Ydim, int Zdim, int numCoresPerNode, double** D_matrix);

                static std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                int getXDim() const { return xdim; }
                int getYDim() const { return ydim; }
                int getZDim() const { return zdim; }
                
                //returns the coordinates of the given node index
                int xOf(unsigned int node) const { return node % xdim; }
                int yOf(unsigned int node) const { return (node / xdim) % ydim; }
                int zOf(unsigned int node) const { return node / (xdim * ydim); }

                //returns index of given dimensions
                unsigned int indexOf(int x, int y, int z) const { return (x + xdim * y + xdim * ydim * z); }

                unsigned int getNodeDistance(int node0, int node1) const;

                unsigned int pairwiseL1Distance(std::vector<MeshLocation*>* locs) const;

                //returns the index of the given network link
                //@x,y,z coordinates of the source node 
                //@direction link dimension from the source node(x=x,y=1,z=2)
                //assumes that sending&receiving is the same
                unsigned int getLinkIndex(int x, int y, int z, int dimension) const;
                
                //baseline allocation: minimum-volume rectangular prism that fits into the machine
                AllocInfo* getBaselineAllocation(Job* job);

                //MeshMachine default routing is dimension ordered: first x, then y, then z, all in increasing direction
                //@return vector of link indices
                std::vector<unsigned int> getRoute(int node0, int node1, double commWeight) const;
        };
        
        /**
         * The default ordering for MeshLocations is by the component: x, y, then z.
         * Comparator used to order free blocks in MBSAllocator.
         */

        class MeshLocation : public std::binary_function<SST::Scheduler::MeshLocation*, SST::Scheduler::MeshLocation*,bool>{

            public:
                int x;
                int y;
                int z;

                MeshLocation(int X, int Y, int Z);
                MeshLocation(int inpos, const MeshMachine & m);

                MeshLocation(const MeshLocation & in);

                int L1DistanceTo(const MeshLocation & other) const;

                int LInfDistanceTo(const MeshLocation & other) const;

                bool operator()(MeshLocation* loc1, MeshLocation* loc2);

                bool equals(const MeshLocation & other) const;

                int toInt(const MeshMachine & m);

                std::string toString();
        };
    }
}
#endif

