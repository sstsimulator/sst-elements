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

#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "sst/core/serialization/element.h"

#include "Machine.h"
#include "SimpleMachine.h"
#include "AllocInfo.h"
#include "misc.h"

using namespace SST::Scheduler;

SimpleMachine::SimpleMachine(int procs, schedComponent* sc) {  //takes number of processors
  numProcs = procs;
  this->sc = sc;
  reset();
}
    
void SimpleMachine::reset() {  //return to beginning-of-simulation state
  numAvail = numProcs;
  freeNodes.clear();
  for(int i=0; i<numProcs; i++)
    freeNodes.push_back(i);
}

/*
Machine* SimpleMachine::Make(vector<string>* params) { //Factory creation method
  argsAtMost(1, params);
  argsAtLeast(1, params);

  return new SimpleMachine(atoi((*params)[1].c_str()));
}

string SimpleMachine::getParamHelp() {
  return "[<num procs>]";
}
*/

string SimpleMachine::getSetupInfo(bool comment){
  string com;
  if(comment)
    com="# ";
  else
    com="";
  char mesg[100];
  sprintf(mesg, "SimpleMachine with %d processors", numProcs);
  return com + mesg;
}

void SimpleMachine::allocate(AllocInfo* allocInfo) {  //allocate processors
  int num = allocInfo -> job -> getProcsNeeded();  //number of processors
  
  if(debug)
    cerr << "allocate(" << allocInfo -> job -> toString() << "); "
	 << (numAvail - num) << " processors free" << endl;
  
  if(num > numAvail) {
    char mesg[100];
    sprintf(mesg, "Attempt to allocate %d processors when only %d are available", num, numAvail);
    internal_error(string(mesg));
  }
  
  numAvail -= num;
  for(int i=0; i<num; i++) {
    allocInfo->nodeIndices[i] = freeNodes.back();
    freeNodes.pop_back();
  }
  sc -> startJob(allocInfo);
}

void SimpleMachine::deallocate(AllocInfo* allocInfo) {  //deallocate processors
  
  int num = allocInfo ->job -> getProcsNeeded();  //number of processors
  
  if(debug)
    cerr << "deallocate(" << allocInfo -> job << "); " 
	 << (numAvail + num) << " processors free" << endl;
  
  if(num > (numProcs - numAvail)) {
    char mesg[100];
    sprintf(mesg, "Attempt to deallocate %d processors when only %d are busy",
	    num, (numProcs-numAvail));
    internal_error(string(mesg));
  }

  numAvail += num;
  for(int i=0; i<num; i++) {
    freeNodes.push_back(allocInfo->nodeIndices[i]);
  }
}
