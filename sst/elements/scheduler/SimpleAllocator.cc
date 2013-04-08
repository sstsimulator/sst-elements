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

#include <stdlib.h>

#include "sst/core/serialization/element.h"

#include "SimpleAllocator.h"
#include "Machine.h"
#include "SimpleMachine.h"
#include "AllocInfo.h"
#include "Job.h"
#include "misc.h"

using namespace SST::Scheduler;

SimpleAllocator::SimpleAllocator(SimpleMachine* m) {
  machine = m;
}

string SimpleAllocator::getSetupInfo(bool comment) {
  string com;
  if(comment)
    com="# ";
  else
    com="";
  return com+"Simple Allocator";
}

/*
Allocator* SimpleAllocator::Make(vector<string>* params) {
  argsAtMost(0, params);
  Machine* m = getMachine();
  SimpleMachine* sm = dynamic_cast<SimpleMachine*>(m);
  if(sm != NULL)
    return new SimpleAllocator(sm);
  
  error("You cannot use SimpleAllocator with anything but SimpleMachine");
  return NULL;
}

string SimpleAllocator::getParamHelp() {
  return "";
}
*/

AllocInfo* SimpleAllocator::allocate(Job* j) {  //allocates j if possible
  //returns information on the allocation or NULL if it wasn't possible
  //(doesn't make allocation; merely returns info on possible allocation)

  if(canAllocate(j)) {
    return new AllocInfo(j);
  }
  return NULL;
}
