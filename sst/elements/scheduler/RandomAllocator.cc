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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "RandomAllocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "AllocInfo.h"
#include "MeshAllocInfo.h"
#include "Job.h"
#include "misc.h"

using namespace std;
using namespace SST::Scheduler;


RandomAllocator::RandomAllocator(Machine* mesh) {
  machine = dynamic_cast<MachineMesh*>(mesh);
  if(machine == NULL)
    error("Random Allocator requires Mesh");
  srand(0);
}
/*
RandomAllocator RandomAllocator::Make(vector<string*>* params){
  Factory.argsAtLeast(0,params);
  Factory.argsAtMost(0,params);

  MachineMesh* m = (MachineMesh) Main->getMachine();
  if(m == NULL)
    error("Random allocator requires a Mesh machine");
  return new RandomAllocator(m);
  return null;

}
*/

string RandomAllocator::getParamHelp(){
  return "";
}

string RandomAllocator::getSetupInfo(bool comment){
  string com;
  if(comment) com="# ";
  else com="";
  return com+"Random Allocator";
}

AllocInfo* RandomAllocator::allocate(Job* job) {
  //allocates job if possible
  //returns information on the allocation or null if it wasn't possible
  //(doesn't make allocation; merely returns info on possible allocation)

  if(!canAllocate(job))
    return NULL;

  MeshAllocInfo* retVal = new MeshAllocInfo(job);

  //figure out which processors to use
  int numProcs = job->getProcsNeeded();
  vector<MeshLocation*>* available = ((MachineMesh*)machine)->freeProcessors();
  for(int i=0; i<numProcs; i++) {
    int num = rand() % available->size();
    (*retVal->processors)[i] = (*available)[num];
    retVal->nodeIndices[i] = (*available)[num]->toInt((MachineMesh*)machine);
    available->erase(available->begin() + num);
  }
  for(int i = 0; i < (int)available->size(); i++)
    delete available->at(i);
  available->clear();
  delete available;
  return retVal;
}
