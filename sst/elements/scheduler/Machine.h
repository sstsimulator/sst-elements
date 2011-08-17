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
 * Abstract base class for machines
 */

#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <string>
#include "schedComponent.h"
using namespace std;

class AllocInfo;

class Machine {
 public:

  virtual ~Machine() {}

  virtual string getSetupInfo(bool comment) = 0;

  int getNumFreeProcessors() {
    return numAvail;
  }

  int getNumProcs() {
    return numProcs;
  }

  virtual void reset() = 0;

  virtual void allocate(AllocInfo* allocInfo) = 0;

  virtual void deallocate(AllocInfo* allocInfo) = 0;

 protected:
    int numProcs;          //total number of processors
    int numAvail;          //number of available processors

    schedComponent* sc;    //interface to rest of simulator
};

Machine* getMachine();     //defined in Main.cc

#endif
