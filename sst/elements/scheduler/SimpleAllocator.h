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
 * Allocator for SimpleMachine- no representation of locality so just
 * keep track of the number of free processors
 */

#ifndef __SIMPLEALLOCATOR_H__
#define  __SIMPLEALLOCATOR_H__

#include <vector>
#include "Allocator.h"

using namespace std;

namespace SST {
namespace Scheduler {

class SimpleMachine;
class Job;

class SimpleAllocator : public Allocator {
 public:

  SimpleAllocator(SimpleMachine* m);

  ~SimpleAllocator() {}

  //static Allocator* Make(vector<string>* params);
  //static string getParamHelp();
  string getSetupInfo(bool comment);

  AllocInfo* allocate(Job* j);  //allocates j if possible
  //returns information on the allocation or NULL if it wasn't possible
  //(doesn't make allocation; merely returns info on possible allocation)
};

}
}
#endif
