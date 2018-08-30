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

/**
 * Allocator that uses tbe best-fit linear allocation strategy
 * (according to the order specified when the allocator is created)->
 * Uses the smallest interval of free nodes that is big enough->  If
 * none are big enough, chooses the one that minimizes the span
 * (maximum distance along linear order between assigned nodes)->
 */

#include "sst_config.h"
#include "BestFitAllocator.h"

#include <vector>
#include <string>
#include <limits>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "LinearAllocator.h"
#include "Machine.h"
#include "output.h"
#include "StencilMachine.h"

using namespace SST::Scheduler;
using namespace std;

BestFitAllocator::BestFitAllocator(vector<string>* params, Machine* mach): LinearAllocator(params, mach) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    schedout.debug(CALL_INFO, 0, 0, "Constructing BestFitAllocator\n");
}

string BestFitAllocator::getSetupInfo(bool comment) const
{
    string com;
    if (comment) {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Linear Allocator (Best Fit)";
}

//This function allocates a job if possible, or
//returns information on the allocation or NULL if it wasn't possible.
//(It doesn't make the allocation; merely returns info on a possible
//allocation)
AllocInfo* BestFitAllocator::allocate(Job* job) 
{
    //check if we have enough free nodes
    if (!canAllocate(*job)) return NULL;

    vector<vector<MeshLocation*>*>* intervals = getIntervals();

    int num = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

    int bestInterval = -1;  //index of best interval found so far
    //(-1 = none)
    int bestSize = std::numeric_limits<int>::max(); //its size

    //look for smallest sufficiently-large interval
    for (int i = 0; i < (int)intervals -> size(); i++) {
        int size = (int)intervals -> at(i)-> size();
        if ((size >= num) && (size < bestSize)) {
            if (-1 != bestInterval) {
                for (int j = 0; j < (int)intervals -> at(bestInterval) -> size(); j++) {
                    delete intervals -> at(bestInterval) -> at(j);
                }
                intervals -> at(bestInterval) -> clear();
                delete intervals -> at(bestInterval);
            }
            bestInterval = i;
            bestSize = size;
        } else {
            for (int j = 0; j < size; j++) {
                delete intervals -> at(i) -> at(j);
            }
            intervals -> at(i) -> clear();
            delete intervals -> at(i);
        }
    }

    if (bestInterval == -1) {
        //no single interval is big enough; minimize the span
        return minSpanAllocate(job);
    } else {
        AllocInfo* retVal = new AllocInfo(job, machine);
        int j;
        for (j = 0; j < (int)intervals -> at(bestInterval) -> size(); j++) {
            if (j < num) {
                retVal -> nodeIndices[j] = intervals -> at(bestInterval) -> at(j) -> toInt(*mMachine);
            } else {
                delete intervals -> at(bestInterval) -> at(j);
            }
        }
        delete intervals -> at(bestInterval);
        intervals -> clear();
        delete intervals;
        return retVal;
    }
}
