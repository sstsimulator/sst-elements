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
 * Class that represents machine as a bag of processors (no locations)
 */

#ifndef SST_SCHEDULER_SIMPLEMACHINE_H__
#define SST_SCHEDULER_SIMPLEMACHINE_H__

#include <string>
#include <vector>

#include "Machine.h"

namespace SST {
    namespace Scheduler {
        class AllocInfo;

        class SimpleMachine : public Machine {

            public:                
                SimpleMachine(int numNodes, bool insimulationmachine, int numCoresPerNode, double** D_matrix);
                virtual ~SimpleMachine() {}

                //static Machine* Make(vector<string>* params); //Factory creation method
                //static string getParamHelp();
                std::string getSetupInfo(bool comment);

                void reset();  //return to beginning-of-simulation state

                void allocate(AllocInfo* allocInfo);

                void deallocate(AllocInfo* allocInfo);  //deallocate processors
                
                long getNodeDistance(int node1, int node2) const;

                //ConstraintAllocator needs these
                std::vector<int>* getFreeNodes();

            private:
                static const int debug = 0;  //whether to include debugging printouts
                std::vector<int> freeNodes;       //indices of currently-free nodes
                bool simulationmachine;
        };

    }
}
#endif
