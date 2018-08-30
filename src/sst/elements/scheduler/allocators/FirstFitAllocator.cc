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
 * Allocator that uses tbe first-fit linear allocation strategy
 * (according to the order specified when the allocator is created)->
 * Uses the first intervals of free processors that is big enough->  If
 * none are big enough, chooses the one that minimizes the span
 * (maximum distance along linear order between assigned processors)->
 */

#include "sst_config.h"
#include "FirstFitAllocator.h"

#include <vector>
#include <string>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "LinearAllocator.h"
#include "Machine.h"
#include "StencilMachine.h"
#include "output.h"

using namespace SST::Scheduler;

FirstFitAllocator::FirstFitAllocator(std::vector<std::string>* params, Machine* mach): LinearAllocator(params, mach) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    schedout.debug(CALL_INFO, 1, 0, "Constructing FirstFitAllocator\n");
}

std::string FirstFitAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment)  {
        com = "# ";
    } else {
        com = "";
    }
    return com + "Linear Allocator (First Fit)";
}

//allocates job if possible
//returns information on the allocation or NULL if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation)
AllocInfo* FirstFitAllocator::allocate(Job* job) 
{
    schedout.fatal(CALL_INFO, 1, "Allocating %s procs: ", job -> toString().c_str());

    if (!canAllocate(*job)) {  //check if we have enough free processors
        return NULL;
    }

    std::vector<std::vector<MeshLocation*>*>* intervals = getIntervals();

    int numNodes = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

    //find an interval to use if one exists
    for (int i = 0; i < (int)intervals -> size(); i++) {
        if ((int)intervals -> at(i) -> size() >= numNodes) {
            AllocInfo* retVal = new AllocInfo(job, machine);
            int j;
            for (j = 0; j<numNodes; j++) {
                schedout.debug(CALL_INFO, 7, 0, "%d ", intervals -> at(i) -> at(j) -> toInt(*mMachine));
                retVal -> nodeIndices[j] = intervals -> at(i) -> at(j) -> toInt(*mMachine);
            }
            j++;
            while (j < (int)intervals -> at(i) -> size()) {
                delete intervals -> at(i) -> at(j++);
            }
            intervals -> at(i) -> clear();
            delete intervals -> at(i);

            schedout.debug(CALL_INFO, 7, 0, "\n");
            i++;
            while (i < (int)intervals -> size()) {
                for (int j = 0; j < (int)intervals -> at(i) -> size(); j++) {
                    delete intervals -> at(i) -> at(j);
                }
                intervals -> at(i) -> clear();
                delete intervals -> at(i);
                i++;
            }
            intervals -> clear();
            delete intervals;
            return retVal;
        } else {
            for (int j = 0; j < (int)intervals -> at(i) -> size(); j++) {
                delete intervals -> at(i) -> at(j);
            }
            intervals -> at(i) -> clear();
            delete intervals -> at(i);
        }
    }
    intervals -> clear();
    delete intervals;

    //no single interval is big enough; minimize the span
    //if (DEBUG) {
    //    printf("No interval large enough, minimizing span\n");
    //}
    schedout.debug(CALL_INFO, 7, 0, "No interval large enough, minimizing span\n");
    return minSpanAllocate(job);
}
