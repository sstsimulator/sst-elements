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

#ifndef SST_SCHEDULER_MESHALLOCINFO_H__
#define SST_SCHEDULER_MESHALLOCINFO_H__

#include <vector>
#include <string>

//#include "sst/core/serialization.h"

#include "AllocInfo.h"

namespace SST {
    namespace Scheduler {

        class MachineMesh;


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
                MeshLocation(int inpos, MachineMesh* m);

                MeshLocation(MeshLocation* in);

                int L1DistanceTo(MeshLocation* other);

                int LInfDistanceTo(MeshLocation* other);

                bool operator()(MeshLocation* loc1, MeshLocation* loc2);

                bool equals(MeshLocation* other);

                void print();

                int toInt(MachineMesh* m);

                std::string toString();

                int hashCode();
        };

        class MeshAllocInfo : public AllocInfo {
            public:
                std::vector<MeshLocation*>* processors;
                MeshAllocInfo(Job* j);
                ~MeshAllocInfo();

                std::string getProcList(Machine* m);
        };

    }
}

#endif
