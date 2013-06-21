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
#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "misc.h"

#define DEBUG false

using namespace SST::Scheduler;

/*
   FirstFitAllocator::FirstFitAllocator(MachineMesh* m, string filename) : LinearAllocator(m) {
//takes machine to be allocated and file giving linear order
//(file format described at head of LinearAllocator)
}
*/
FirstFitAllocator::FirstFitAllocator(std::vector<std::string>* params, Machine* mach): LinearAllocator(params, mach) 
{
    if (DEBUG) {
        printf("Constructing FirstFitAllocator\n");
    }

    if (dynamic_cast<MachineMesh*>(mach) == NULL) {
        error("Linear allocators require a MachineMesh* machine");
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

AllocInfo* FirstFitAllocator::allocate(Job* job) 
{
    if (DEBUG) {
        printf("Allocating %s procs: ", job -> toString().c_str());
    }
    //allocates job if possible
    //returns information on the allocation or NULL if it wasn't possible
    //(doesn't make allocation; merely returns info on possible allocation)

    if (!canAllocate(job)) {  //check if we have enough free processors
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
                if (DEBUG) {
                    printf("%d ", intervals -> at(i) -> at(j) -> toInt((MachineMesh*)machine));
                }
                retVal -> processors -> at(j) = intervals -> at(i) -> at(j);
                retVal -> nodeIndices[j] = intervals -> at(i) -> at(j) -> toInt((MachineMesh*)machine);
            }
            j++;
            while (j < (int)intervals -> at(i) -> size()) {
                delete intervals -> at(i) -> at(j++);
            }
            intervals -> at(i) -> clear();
            delete intervals -> at(i);
            if (DEBUG) {
                printf("\n");
            }
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
    if (DEBUG) {
        printf("No interval large enough, minimizing span\n");
    }
    return minSpanAllocate(job);
}
