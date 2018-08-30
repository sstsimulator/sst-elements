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
 * Class that represents machine as a bag of processors (no locations)
 */

#ifndef SST_SCHEDULER_SIMPLEMACHINE_H__
#define SST_SCHEDULER_SIMPLEMACHINE_H__

#include <cstdlib>
#include <list>
#include <string>

#include "Machine.h"

namespace SST {
    namespace Scheduler {
        class AllocInfo;

        class SimpleMachine : public Machine {

            public:
                SimpleMachine(int numNodes, bool insimulationmachine, int numCoresPerNode, double** D_matrix);
                virtual ~SimpleMachine() {}

                std::string getSetupInfo(bool comment);

                void reset();  //return to beginning-of-simulation state
                
                AllocInfo* getBaselineAllocation(Job* job) const { return NULL; }
                
                int getNodeDistance(int node0, int node1) const;
                
                int nodesAtDistance(int dist) const;
                
                //returns the free nodes at given L1 distance
                std::list<int>* getFreeAtDistance(int center, int distance) const;

                //SimpleMachine assumes a single network link in the machine
                std::list<int>* getRoute(int node0, int node1, double commWeight) const;

            private:
                bool simulationmachine;
        };

    }
}
#endif
