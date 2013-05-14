
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
 Attempt to allocate compute nodes subject to constraints
 Presumably constraints designed to minimize uncertainty
 i.e. to maximize the amount of information that job failures
 will give about system parameters
 */

#ifndef __CONSTRAINTALLOCATOR_H__
#define __CONSTRAINTALLOCATOR_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Allocator.h"

namespace SST {
namespace Scheduler {

class SimpleMachine;
class Job;

class ConstraintAllocator : public Allocator {

  public:
    ConstraintAllocator(SimpleMachine* m, std::string DepsFile, std::string ConstFile);

    //ConstraintAllocator Make(vector<string*>* params);

    string getParamHelp();

    string getSetupInfo(bool comment);

    AllocInfo* allocate(Job* job);

  private:
    //constraints
    //for now, only type of constraint is to separate one node from rest of cluster
    //check file for updates to parameter estimates and set constraints accordingly
    void GetConstraints();

    //map from internal node u to  set of dependent compute nodes D[u]
    map< std::string, set<std::string> > D;
    vector<std::string> Cluster; // current cluster of suspects to be separated
    std::string ConstraintsFileName;
};

#endif

}
}
