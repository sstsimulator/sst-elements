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
 * Abstract class to act as superclass for allocators based on curves->
 *
 * Format for file giving curve:
 * list of pairs of numbers (separated by whitespace)
 *   the first member of each pair is the processor 0-indexed
 *     rank if the coordinates are treated as a 3-digit number
 *     (z coord most significant, x coord least significant)
 *     these values should appear in order
 *   the second member of each pair gives its rank in the desired order
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <time.h>
#include <math.h>

#include "sst/core/serialization/element.h"

#include "LinearAllocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "AllocInfo.h"
#include "Job.h"
#include "misc.h"

#define DEBUG false

using namespace SST::Scheduler;

LinearAllocator::MeshLocationOrdering::MeshLocationOrdering(Machine* mach, bool SORT = false) 
{
    //right now only the snake curve is implemented; that should be a
    //parameter in the future
    MachineMesh* m = dynamic_cast<MachineMesh*>(mach);
    if (NULL == m)
        error("Linear Allocators Require Mesh Machine");


    set<int> dimordering;
    set<int>::iterator xit = dimordering.insert(m -> getXDim()).first;
    set<int>::iterator yit = dimordering.insert(m -> getYDim()).first;
    set<int>::iterator zit = dimordering.insert(m -> getZDim()).first;
    set<int>::iterator it = dimordering.begin();
    if (SORT) {
        xpos = distance(dimordering.begin(), xit);
        ypos = distance(dimordering.begin(), yit);
        zpos = distance(dimordering.begin(), zit);
        xdim = *it++;
        ydim = *it++;
        zdim = *it;
    } else {
        xpos = 0;
        ypos = 1;
        zpos = 2;
        xdim = m -> getXDim();
        ydim = m -> getYDim();
        zdim = m -> getZDim();
    }
    rank = new int[xdim*ydim*zdim];
    int xcount = 0;
    int ycount = 0;
    int zcount = 0;
    int xdir = 1;
    int ydir = 1;
    int totalcount = 0;
    while (zcount < zdim) {
        rank[xcount + ycount*xdim + zcount*ydim*xdim] = totalcount;
        totalcount++;
        if (xcount + xdir >= xdim || xcount + xdir < 0) {
            xdir *= -1;
            if (ycount + ydir >= ydim || ycount + ydir < 0) {
                ydir *= -1;
                zcount++; //z always goes up; once it reaches its max we're done
            } else {
                ycount+= ydir;
            }
        } else {
            xcount += xdir;
        }
    }
}

int LinearAllocator::MeshLocationOrdering::rankOf(MeshLocation* L) {
    //returns the rank of a given location
    //have to mix which coordinate is which in case x was not smallest
    int coordinates[3];
    coordinates[xpos] = L->x;
    coordinates[ypos] = L->y;
    coordinates[zpos] = L->z;
    return rank[coordinates[0] + coordinates[1] * xdim + coordinates[2] * xdim * ydim];
}
/*
   currently unused (and doesn't work with dimordering)
   MeshLocation* LinearAllocator::MeshLocationOrdering::locationOf(int Rank) {
//return MeshLocation having given rank
//raises exception if Rank is out of range

int pos = -1;   //ordinal value of position having rank
int x, y, z;    //coordinate values

//find Rank in array with ordering
int i=0;
while(pos == -1) {
if(rank[i] == Rank)
pos = i;
i++;
}

//translate position in ordering into coordinates
x = pos % xdim;
pos = pos / xdim;
y = pos % ydim;
z = pos / ydim;
return new MeshLocation(x, y, z);
}
*/

LinearAllocator::LinearAllocator(vector<string>* params, Machine* mach) 
{
    //takes machine to be allocated and name of the file with ordering
    //file format is as described in comment at top of this file
    MachineMesh* m = dynamic_cast<MachineMesh*>(mach);
    if (NULL == m) {
        error("Linear allocators require a MachineMesh* machine");
    }

    machine = m;

    if (params -> size() == 0) {
        ordering = new MeshLocationOrdering(m);
    } else {
        if (params -> at(0) == "sort") {
            ordering = new MeshLocationOrdering(m, true);
        } else if (params -> at(0) == "nosort") {
            ordering = new MeshLocationOrdering(m, false);
        } else {
            error("Argument to Linear Allocator must be sort or nosort:" + params->at(0));
        }
    }
}

vector<vector<MeshLocation*>*>* LinearAllocator::getIntervals() {
    //returns list of intervals of free processors
    //each interval represented by a list of its locations

    set<MeshLocation*, MeshLocationOrdering>* avail = new set<MeshLocation*,MeshLocationOrdering>(*ordering);
    //add all from machine->freeProcessors() to avail
    vector<MeshLocation*>* machfree = ((MachineMesh*)machine) -> freeProcessors();
    avail -> insert(machfree -> begin(), machfree -> end());

    vector<vector<MeshLocation*>*>* retVal =  //list of intervals so far
        new vector<vector<MeshLocation*>*>();
    vector<MeshLocation*>* curr =               //interval being built
        new vector<MeshLocation*>();
    int lastRank = -2;                           //rank of last element
    //-2 is sentinel value

    for (set<MeshLocation*,MeshLocationOrdering>::iterator ml = avail -> begin(); ml != avail -> end(); ml++) {
        int mlRank = ordering -> rankOf(*ml);
        if ((mlRank != lastRank + 1) && (lastRank != -2)) {
            //need to start new interval
            retVal -> push_back(curr);
            curr = new vector<MeshLocation*>();
        }
        curr->push_back(*ml);
        lastRank = mlRank;
    }
    if (curr -> size() != 0) {  //add last interval if nonempty
        retVal -> push_back(curr);
    } else {
        curr -> clear();
        delete curr;
    }

    if (DEBUG) {
        printf("getIntervals:");
        for (vector<vector<MeshLocation*>*>::iterator ar = retVal -> begin(); ar != retVal -> end(); ar++) {
            printf("Interval: ");
            for (int x = 0; x < (int)(*ar) -> size(); x++) {
                (*ar) -> at(x) -> print();
                printf(" ");
            }
            printf("\n");
        }
    }
    avail -> clear();
    machfree -> clear();
    delete avail;
    delete machfree;

    return retVal;
}

//Version of allocate that just minimizes the span.
AllocInfo* LinearAllocator::minSpanAllocate(Job* job) {

    vector<MeshLocation*>* avail = ((MachineMesh*)machine) -> freeProcessors();
    sort(avail -> begin(), avail -> end(), *ordering);
    int num = job -> getProcsNeeded();

    //scan through possible starting locations to find best one
    int bestStart = 0;   //index of best starting location so far
    int bestSpan = ordering -> rankOf(avail -> at(num -1)) - ordering-> rankOf(avail -> at(0));//best location's span
    for (int i = 1; i <= (int)avail -> size() - num; i++) {
        int candidate = ordering -> rankOf(avail -> at(i+num -1)) - ordering-> rankOf(avail -> at(i));
        if (candidate < bestSpan) {
            bestStart = i;
            bestSpan = candidate;
        }
    }

    //return the best allocation found
    MeshAllocInfo* retVal = new MeshAllocInfo(job);
    for (int i = 0; i< (int)avail -> size(); i++) {
        if (i >= bestStart && i < bestStart + num) {
            retVal -> processors -> at(i  - bestStart) = avail-> at(i);
            retVal -> nodeIndices[i  - bestStart] = avail-> at(i)->toInt((MachineMesh*)machine);
        } else {
            delete avail -> at(i); //have to delete any not being used
        }
    }
    avail -> clear();
    delete avail;
    return retVal;
}
