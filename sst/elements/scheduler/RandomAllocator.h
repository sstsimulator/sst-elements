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
 * Allocator for SimpleMachine- no representation of locality so just
 * keep track of the number of free processors
 */

#ifndef __RANDOMALLOCATOR_H__
#define __RANDOMALLOCATOR_H__

#include <string>
#include <vector>
#include "Allocator.h"
#include "MachineMesh.h"


class RandomAllocator : public Allocator {

  public:
    RandomAllocator(MachineMesh* mesh);

    RandomAllocator Make(vector<string*>* params);

    string getParamHelp();

    string getSetupInfo(bool comment);

    AllocInfo* allocate(Job* job);
};

#endif

