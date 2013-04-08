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
 * Allocator that uses tbe best-fit linear allocation strategy
 * (according to the order specified when the allocator is created)->
 * Uses the smallest interval of free processors that is big enough->  If
 * none are big enough, chooses the one that minimizes the span
 * (maximum distance along linear order between assigned processors)->
 */

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <time.h>
#include <math.h>
#include <limits>

#include "sst/core/serialization/element.h"

#include "BestFitAllocator.h"
#include "LinearAllocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "AllocInfo.h"
#include "Job.h"
#include "misc.h"

#define DEBUG false

using namespace SST::Scheduler;

/*
   BestFitAllocator(MachineMesh* m, string filename) {
//takes machine to be allocated and file giving linear order
//(file format described at head of LinearAllocator->java)
super(m, filename);
}
 */

BestFitAllocator::BestFitAllocator(vector<string>* params, Machine* mach): LinearAllocator(params, mach) {
    if(DEBUG)
      printf("Constructing BestFitAllocator\n");
    if(dynamic_cast<MachineMesh*>(mach) == NULL)
      error("Linear allocators require a mesh");
  }

string BestFitAllocator::getSetupInfo(bool comment){
  string com;
  if(comment) com="# ";
  else com="";
  return com+"Linear Allocator (Best Fit)";
}

AllocInfo* BestFitAllocator::allocate(Job* job) {
  //allocates job if possible
  //returns information on the allocation or NULL if it wasn't possible
  //(doesn't make allocation; merely returns info on possible allocation)
  if(DEBUG)
    printf("Allocating %s procs: \n", job->toString().c_str());

  if(!canAllocate(job))   //check if we have enough free processors
    return NULL;

  vector<vector<MeshLocation*>*>* intervals = getIntervals();

  int num = job->getProcsNeeded();  //number of processors for job

  int bestInterval = -1;  //index of best interval found so far
  //(-1 = none)
  int bestSize = INT_MAX;  //its size

  //look for smallest sufficiently-large interval
  for(int i=0; i<(int)intervals->size(); i++) {
    int size = (int)intervals->at(i)->size();
    if((size >= num) && (size < bestSize)) {
      if(bestInterval !=  -1)
      {
        for(int j = 0; j < (int)intervals->at(bestInterval)->size(); j++)
          delete intervals->at(bestInterval)->at(j);
        intervals->at(bestInterval)->clear();
        delete intervals->at(bestInterval);
      }
      bestInterval = i;
      bestSize = size;
    }
    else{
      for(int j = 0; j < size; j++)
        delete intervals->at(i)->at(j);
      intervals->at(i)->clear();
      delete intervals->at(i);
    }
  }

  if(bestInterval == -1) {
    //no single interval is big enough; minimize the span
    return minSpanAllocate(job);
  } else {
    MeshAllocInfo* retVal = new MeshAllocInfo(job);
    int j;
    for(j=0; j<(int)intervals->at(bestInterval)->size(); j++)
    {
      if(j < num)
      {
        retVal->processors->at(j) = intervals->at(bestInterval)->at(j);
        retVal->nodeIndices[j] = intervals->at(bestInterval)->at(j)->toInt((MachineMesh*)machine);
      }
      else
        delete intervals->at(bestInterval)->at(j);
    }
    delete intervals->at(bestInterval);
    intervals->clear();
    delete intervals;
    return retVal;
  }
}
