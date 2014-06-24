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

/**
 * Allocator that uses tbe first-fit linear allocation strategy
 * (according to the order specified when the allocator is created)->
 * Uses the first intervals of free processors that is big enough->  If
 * none are big enough, chooses the one that minimizes the span
 * (maximum distance along linear order between assigned processors)->
 */

//#include <stdio.h>
//#include <stdlib.h>
//#include <sstream>
//#include <time.h>
//#include <math.h>

#include "sst_config.h"
#include "FirstFitAllocator.h"

#include <vector>
#include <string>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "LinearAllocator.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"

using namespace SST::Scheduler;

FirstFitAllocator::FirstFitAllocator(std::vector<std::string>* params, Machine* mach): LinearAllocator(params, mach) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    //if (DEBUG) printf("Constructing FirstFitAllocator\n");
    schedout.debug(CALL_INFO, 1, 0, "Constructing FirstFitAllocator\n");

    if (dynamic_cast<MeshMachine*>(mach) == NULL) {
        schedout.fatal(CALL_INFO, 1, "Linear allocators require a MeshMachine* machine");
        //error("Linear allocators require a MeshMachine* machine");
    }
}

std::string FirstFitAllocator::getSetupInfo(bool comment)
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
    //if (DEBUG) {
    //    printf("Allocating %s procs: ", job -> toString().c_str());
    //}
    schedout.fatal(CALL_INFO, 1, "Allocating %s procs: ", job -> toString().c_str());

    if (!canAllocate(*job)) {  //check if we have enough free processors
        return NULL;
    }

    std::vector<std::vector<MeshLocation*>*>* intervals = getIntervals();

    int num = job -> getProcsNeeded();  //number of processors for job

    //find an interval to use if one exists
    for (int i = 0; i < (int)intervals -> size(); i++) {
        if ((int)intervals -> at(i) -> size() >= num) {
            MeshAllocInfo* retVal = new MeshAllocInfo(job);
            int j;
            for (j = 0; j<num; j++) {
                //if (DEBUG) {
                //    printf("%d ", intervals -> at(i) -> at(j) -> toInt((MeshMachine*)machine));
                //}
                schedout.debug(CALL_INFO, 7, 0, "%d ", intervals -> at(i) -> at(j) -> toInt((MeshMachine*)machine));
                retVal -> processors -> at(j) = intervals -> at(i) -> at(j);
                retVal -> nodeIndices[j] = intervals -> at(i) -> at(j) -> toInt((MeshMachine*)machine);
            }
            j++;
            while (j < (int)intervals -> at(i) -> size()) {
                delete intervals -> at(i) -> at(j++);
            }
            intervals -> at(i) -> clear();
            delete intervals -> at(i);
            //if (DEBUG) {
            //    printf("\n");
            //}
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
