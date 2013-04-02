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

/*
 * Class that represents machine as a bag of processors (no locations)
 */

#ifndef __SIMPLEMACHINE_H__
#define __SIMPLEMACHINE_H__

#include <string>
using namespace std;

#include "Machine.h"

namespace SST {
namespace Scheduler {

class SimpleMachine : public Machine {

 public:
  SimpleMachine(int procs, schedComponent* sc);  //takes number of processors

  virtual ~SimpleMachine() {}

  //static Machine* Make(vector<string>* params); //Factory creation method
  //static string getParamHelp();
  string getSetupInfo(bool comment);

  void reset();  //return to beginning-of-simulation state

  void allocate(AllocInfo* allocInfo);

  void deallocate(AllocInfo* allocInfo);  //deallocate processors

 private:
  static const int debug = 0;  //whether to include debugging printouts
  vector<int> freeNodes;       //indices of currently-free nodes
};

}
}
#endif
