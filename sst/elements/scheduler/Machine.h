// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Abstract base class for machines
 */

#ifndef SST_SCHEDULER_MACHINE_H__
#define SST_SCHEDULER_MACHINE_H__

namespace SST {
    namespace Scheduler {
        class AllocInfo;

        class Machine{
            public:

                Machine(int numNodes, int numCoresPerNode, double** D_matrix)
                {
                    this->D_matrix = D_matrix;
                    this->numNodes = numNodes;
                    this->coresPerNode = numCoresPerNode;
                }

                virtual ~Machine()
                {
                    if(D_matrix != NULL){
                        for (int i = 0; i < numNodes; i++)
	                        delete[] D_matrix[i];
                        delete[] D_matrix;
                    }
                }

                virtual std::string getSetupInfo(bool comment) = 0;

                int getNumFreeNodes() const { return numAvail; }
                int getNumNodes() const { return numNodes; }
                int getNumCoresPerNode() const { return coresPerNode; }
                
                virtual void reset() = 0;

                virtual void allocate(AllocInfo* allocInfo) = 0;

                virtual void deallocate(AllocInfo* allocInfo) = 0;
                
                double** D_matrix;

            protected:
                int numNodes;          //total number of nodes
                int numAvail;          //number of available nodes
                int coresPerNode;
        };

    }
}
#endif
