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

        class Job;
        class MeshLocation;

        class MeshMachine : public Machine {

            private:
                int xdim;              //size of mesh in each dimension
                int ydim;
                int zdim;

                std::vector<std::vector<std::vector<bool> > > isFree;  //whether each processor is free
                
            public:
                
                MeshMachine(int Xdim, int Ydim, int Zdim, double** D_matrix);

                static std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                int getXDim() const { return xdim; }
                int getYDim() const { return ydim; }
                int getZDim() const { return zdim; }

                int getMachSize() const;

                void reset();

                std::vector<MeshLocation*>* freeProcessors() const;

                std::vector<MeshLocation*>* usedProcessors() const;

                void allocate(AllocInfo* allocInfo);

                void deallocate(AllocInfo* allocInfo);

                long pairwiseL1Distance(std::vector<MeshLocation*>* locs) const;

                long pairwiseL1Distance(std::vector<MeshLocation*>* locs, int num) const;

				double getCoolingPower() const;
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

                void print();

                int toInt(const MeshMachine & m);

                std::string toString();

                int hashCode();
        };
    }
}
#endif
