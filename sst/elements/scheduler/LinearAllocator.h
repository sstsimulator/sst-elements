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
 * Abstract class to act as superclass for allocators based on curves.
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
#ifndef __LINEARALLOCATOR_H__
#define __LINEARALLOCATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "Allocator.h"
#include "Job.h"
#include "misc.h"


namespace SST {
namespace Scheduler {

class LinearAllocator : public Allocator {


  protected:
    class MeshLocationOrdering : public binary_function<MeshLocation*, MeshLocation*, bool>{
      //represent linear ordering

      private:
        int xpos,ypos,zpos; //helps because we don't know which is largest
        int xdim;     //size of mesh in each dimension
        int ydim;
        int zdim;

        int* rank;   //way to store ordering
        //(x,y,z) has position rank[x+y*xdim+z*xdim*ydim] in ordering

      public:
        MeshLocationOrdering(Machine* m, bool SORT);

        int rankOf(MeshLocation* L);

        //MeshLocation* locationOf(int Rank);

        bool operator()(MeshLocation* L1, MeshLocation* L2){
          return rankOf(L1) < rankOf(L2);
        }
    };

    vector<vector<MeshLocation*>*>* getIntervals();
    AllocInfo* minSpanAllocate(Job* job);
    MeshLocationOrdering* ordering;

  public:
    LinearAllocator(vector<string>* params, Machine* m);
};

}
}
#endif
