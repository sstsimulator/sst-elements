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

/*
 * Abstract base class for machines based on a stencil structure
 */

#ifndef SST_SCHEDULER_STENCILMACHINE_H__
#define SST_SCHEDULER_STENCILMACHINE_H__

#include <list>
#include <string>
#include <vector>

#include "Machine.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class MeshLocation;

        class StencilMachine : public Machine {

            private:

                static int constHelper(std::vector<int> dims); //calculates total number of nodes

                //helper for getFreeAt... functions
                virtual void appendIfFree(std::vector<int> dims, std::list<int>* nodeList) const = 0;

            public:

                const std::vector<int> dims;   //size of mesh in each dimension

                StencilMachine(std::vector<int> dims,
                               int numLinks,
                               int numCoresPerNode,
                               double** D_matrix);

                virtual ~StencilMachine() { }

                static std::string getParamHelp();

                //int getDimSize(int dim) const { return dims[dim]; }
                int numDims() const { return dims.size(); }

                //returns the coordinate of the given node index along the given dimension
                int coordOf(int node, int dim) const;

                //returns index of given dimensions
                int indexOf(std::vector<int> dims) const;

                //returns baseline allocation used for running time estimation
                //baseline allocation: dimension-ordered allocation in minimum-volume
                //rectangular prism that fits into the machine
                AllocInfo* getBaselineAllocation(Job* job) const;

                virtual std::string getSetupInfo(bool comment) = 0;

                //returns the network distance of the given nodes
                virtual int getNodeDistance(int node0, int node1) const = 0;

                //max number of nodes at a given distance - NearestAllocMapper uses this
                int nodesAtDistance(int dist) const = 0;

                //returns the free nodes at given distance
                virtual std::list<int>* getFreeAtDistance(int center, int distance) const = 0;
                //returns the free nodes at given LInf distance, sorted by L1 distance
                virtual std::list<int>* getFreeAtLInfDistance(int center, int distance) const = 0;

                //returns the index of the given network link
                //@dim link dimension from the source node
                virtual int getLinkIndex(std::vector<int> dims, int dim) const = 0;

                //default routing is dimension ordered: first x, then y, ...
                //@return list of link indices
                virtual std::list<int>* getRoute(int node0, int node1, double commWeight) const = 0;
        };

        /**
         * The default ordering for MeshLocations is by the dimensions: x, y, ...
         * Comparator used to order free blocks in MBSAllocator.
         */

        class MeshLocation : public std::binary_function<SST::Scheduler::MeshLocation*, SST::Scheduler::MeshLocation*,bool>{

            public:
                std::vector<int> dims;

                MeshLocation(std::vector<int> dims);
                MeshLocation(int inpos, const StencilMachine & m);
                MeshLocation(const MeshLocation & in);

                int L1DistanceTo(const MeshLocation & other) const;

                int LInfDistanceTo(const MeshLocation & other) const;

                bool operator()(MeshLocation* loc1, MeshLocation* loc2);

                bool equals(const MeshLocation & other) const;

                int toInt(const StencilMachine & m);

                std::string toString();
        };
    }
}
#endif
