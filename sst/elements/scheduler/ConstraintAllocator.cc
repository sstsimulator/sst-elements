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
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "ConstraintAllocator.h"
#include "Machine.h"
#include "SimpleMachine.h"
#include "AllocInfo.h"
#include "Job.h"
#include "misc.h"

using namespace std;
using namespace SST::Scheduler;

ConstraintAllocator::ConstraintAllocator(SimpleMachine* m, string DepsFile, string ConstFile) {
    machine = m;
    ConstraintsFileName = ConstFile;
  // read Dependencies
  // if file does not exist or is empty, D will be an empty mapping
  // and in effect we default to simple allocator
  ifstream DepsStream(DepsFile.c_str(),  ifstream::in );
  string u,v;
  string curline;
  stringstream lineStream;
  while(getline(DepsStream, curline)){ //for each line in file
      lineStream << curline;
      lineStream >> u; // line is u followed by elements of D[u]
      cout << "------------------Dependencies of " << u << endl;
      while (lineStream >> v){
          D[u].insert(v);
          cout << v << " ";
      }
      cout << endl;
      lineStream.clear(); //so we can write to it again
  }

  srand(0);
 }

//external process (python) will read analysis output and create a file
//whose content is one line "u 0.0 v 0.0"
//the zeroes are dummy values for currently unused 'weights' on suspicious nodes
//if file does not exist or is empty, Du and Dv will be empty sets
//an in effect we default to simple allocator
//of course this is inefficent, should check if file has changed
//instead of re-reading every time
void ConstraintAllocator::GetConstraints(){
    string u,v,weight;
    ifstream ConstraintsStream(ConstraintsFileName.c_str(), ifstream::in );
  //currently we are not using weights
    ConstraintsStream >> u;
    ConstraintsStream >> weight;
    ConstraintsStream >> v;
    ConstraintsStream >> weight;
    //just do one lookup and use repeatedly in allocate method
    Du = D[u];
    Dv = D[v];
}

string ConstraintAllocator::getParamHelp(){
  return "";
}

string ConstraintAllocator::getSetupInfo(bool comment){
  string com;
  if(comment) com="# ";
  else com="";
  return com+"Constraint Allocator";
}

AllocInfo* ConstraintAllocator::allocate(Job* job) {
  //allocates job if possible
  //returns information on the allocation or null if it wasn't possible

  if(!canAllocate(job))
    return NULL;

  AllocInfo* retVal = new AllocInfo(job);
  unsigned int numProcs = job->getProcsNeeded();
  vector<int>* available = ((SimpleMachine*)machine)->freeProcessors(); 

  GetConstraints();
  // four classes of available nodes
  vector<int> uOnly; // available nodes in Du \ Dv
  vector<int> vOnly; // available nodes in Dv \ Du
  vector<int> Both; //  available nodes in Du \intersect Dv
  vector<int> Neither; // availabe nodes not in Du \union Dv
  vector<int> xOnly; //either u or v depending on what is available
  // the allocation we will return
  vector<int> Alloc;

  int curIdx;
  std::string curNode;
  // true iff a u/v-separating allocation exists
  bool uvAllocExists = false;
  
  //classify available nodes
  unsigned int i;
  for(i=0; i<available->size(); i++){
      curIdx  = (*available)[i];
      curNode = ((SimpleMachine*)machine)->getNodeID(curIdx);
      if (Du.count(curNode)){
        if (Dv.count(curNode))
            Both.push_back(curIdx);
        else
            uOnly.push_back(curIdx);
      }
      else{
        if (Dv.count(curNode))
            vOnly.push_back(curIdx);
        else
            Neither.push_back(curIdx);
      }
  }

  //determine allocation//use as little of Neither as possible
  //not sure this is the best; should avoid using up all of Du?
  if ((uOnly.size()>0) && (uOnly.size()+Neither.size() >= numProcs)){
    xOnly = uOnly;
    uvAllocExists = true;
    }
  
    if ((vOnly.size()>0) && (vOnly.size()+Neither.size() >= numProcs)){
    xOnly = vOnly;
    uvAllocExists = true;
    }
  
  if (uvAllocExists) { // allocate to depend on x only
    i=0;
    while((i<xOnly.size()) && (Alloc.size() < numProcs))
        Alloc.push_back(xOnly[i++]);
    i=0;
    while((i<Neither.size()) && (Alloc.size() < numProcs))
        Alloc.push_back(Neither[i++]);
  }
  else {
      //allocation forced to depend on both; use 'Both' first
      //still not sure this is best prioritization of available nodes
      i=0;
      while((i<Both.size()) && (Alloc.size() < numProcs))
          Alloc.push_back(Both[i++]);
      i=0;
      while((i<uOnly.size()) && (Alloc.size() < numProcs))
          Alloc.push_back(uOnly[i++]);
      i=0;
      while((i<vOnly.size()) && (Alloc.size() < numProcs))
          Alloc.push_back(vOnly[i++]);
      i=0;
      while((i<Neither.size()) && (Alloc.size() < numProcs))
          Alloc.push_back(Neither[i++]);
  }
  
  for(i=0; i<numProcs; i++) {
      retVal->nodeIndices[i] = Alloc[i];
  }
  available->clear();
  delete available;
  return retVal;
}
//presumably no need to clear the candidate allocs -- destructor will be
//called when function exits

