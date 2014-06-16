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

        class MeshLocation;
        class schedComponent;

        class MachineMesh : public Machine {

            private:
                int xdim;              //size of mesh in each dimension
                int ydim;
                int zdim;  
                schedComponent* sc;

                std::vector<std::vector<std::vector<bool> > > isFree;  //whether each processor is free
                
            public:
                
                MachineMesh(int Xdim, int Ydim, int Zdim, schedComponent* sc, double** D_matrix);

                static std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                int getXDim();

                int getYDim();

                int getZDim();

                int getMachSize();

                void reset();

                std::vector<MeshLocation*>* freeProcessors();

                std::vector<MeshLocation*>* usedProcessors();

                void allocate(AllocInfo* allocInfo);

                void deallocate(AllocInfo* allocInfo);

                long pairwiseL1Distance(std::vector<MeshLocation*>* locs);

                long pairwiseL1Distance(std::vector<MeshLocation*>* locs, int num);

				double getCoolingPower(std::vector<MeshLocation*>* locs);

                //std::string tostd::string();
        };

    }
}
#endif
