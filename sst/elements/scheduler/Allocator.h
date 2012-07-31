// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                               

#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include <vector>
#include <string>
#include "MeshAllocInfo.h"
using namespace std;

class Machine;
class Job;
class AllocInfo;

class Allocator {
 public:
  virtual ~Allocator() {}

  virtual string getSetupInfo(bool comment) = 0;

  virtual bool canAllocate(Job* j);
  virtual bool canAllocate(Job* j, vector<MeshLocation*>* available);

  virtual AllocInfo* allocate(Job* job) = 0;
    //allocates job if possible
    //returns information on the allocation or NULL if it wasn't possible
    //(doesn't make allocation; merely returns info on possible allocation)
  
  virtual void deallocate(AllocInfo* aInfo) { }
  //in case Allocator wants to know when a job is deallocated
  //added for MBS, which wants to update its data structures

  virtual void done() { }
  //called at end of simulation so allocator can report statistics
  
 protected:
  Machine* machine;

};

#endif
