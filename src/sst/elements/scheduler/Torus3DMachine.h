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
 * Abstract base class for machines based on a mesh structure
 */

#ifndef SST_SCHEDULER_TORUS3DMACHINE_H__
#define SST_SCHEDULER_TORUS3DMACHINE_H__

#include <list>
#include <string>
#include <vector>

#include "StencilMachine.h"

namespace SST {
    namespace Scheduler {

        class Torus3DMachine : public StencilMachine {

            private:

                //helper for getFreeAt... functions
                void appendIfFree(std::vector<int> dims, std::list<int>* nodeList) const;

            public:
                Torus3DMachine(std::vector<int> dims, int numCoresPerNode, double** D_matrix = NULL);
                ~Torus3DMachine() { };

                std::string getSetupInfo(bool comment);

                //returns the network distance of the given nodes
                int getNodeDistance(int node0, int node1) const;

                //returns the free nodes at given Distance
                std::list<int>* getFreeAtDistance(int center, int distance) const;
                //LInf distance list is sorted based on L1 distance
                std::list<int>* getFreeAtLInfDistance(int center, int distance) const;

                int nodesAtDistance(int dist) const;

                //returns the index of the given network link
                //@nodeDims the dimensions of the source node
                //@dimension link dimension from the source node(x=0,y=1,...)
                int getLinkIndex(std::vector<int> nodeDims, int dimension) const;

                //MeshMachine default routing is dimension ordered: first x, then y, then z, all in increasing direction
                //@return list of link indices
                std::list<int>* getRoute(int node0, int node1, double commWeight) const;

                //custom mod implementation to avoid negatives
                int mod(const int a, const int b) const
                {
                    int ret = a % b;
                    if(ret < 0)
                        ret += b;
                    return ret;
                }
        };
    }
}
#endif
