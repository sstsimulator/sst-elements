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
 * Class to implement allocation algorithms of the family that
 * includes Gen-Alg, MM, and MC1x1; from each candidate center,
 * consider the closest points, and return the set of closest points
 * that is best.  Members of the family are specified by giving the
 * way to find candidate centers, how to measure "closeness" of points
 * to these, and how to evaluate the sets of closest points.

 GenAlg - try centering at open places;
 select L_1 closest points;
 eval with sum of pairwise L1 distances
 MM - center on intersection of grid in each direction by open places;
 select L_1 closest points
 eval with sum of pairwise L1 distances
 MC1x1 - try centering at open places
 select L_inf closest points
 eval with L_inf distance from center
 */


#ifndef SST_SCHEDULER_NEARESTALLOCATOR_H__
#define SST_SCHEDULER_NEARESTALLOCATOR_H__

#include <vector>
#include <string>

#include "sst/core/serialization.h"

#include "Allocator.h"

namespace SST {
    namespace Scheduler {
        class Job;
        class Machine;
        class CenterGenerator;
        class PointCollector;
        class Scorer;
        class MeshMachine;

        class NearestAllocator : public Allocator {

            private:
                //way to generate list of possible centers:
                CenterGenerator* centerGenerator;

                //how to find candidate points from a center:
                PointCollector* pointCollector;

                //how we evaluate a possible allocation:
                Scorer* scorer;

                std::string configName;

                MeshMachine *mMachine;

            public:

                NearestAllocator(std::vector<std::string>* params, Machine* mach);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                AllocInfo* allocate(Job* job);

                AllocInfo* allocate(Job* job, std::vector<MeshLocation*>* available); 

                void genAlgAllocator(MeshMachine* m);

                void MMAllocator(MeshMachine* m); 

                void OldMC1x1Allocator(MeshMachine* m); 

                void MC1x1Allocator(MeshMachine* m); 

                void HybridAllocator(MeshMachine* m);
        };

    }
}
#endif
