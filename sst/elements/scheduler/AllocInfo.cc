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
        
#include "sst/core/serialization/element.h"
#include <string>
#include "AllocInfo.h"
//#include "Mesh.h"
//#include "MeshLocation.h"
#include "Machine.h"
#include "Job.h"
#include "misc.h"
using namespace std;

AllocInfo::AllocInfo(Job* job) {
  this->job = job;
  nodeIndices = new int[job->getProcsNeeded()];
}

AllocInfo::~AllocInfo() {
  delete nodeIndices;
}

string AllocInfo::getProcList() {
  return "";
}

/*string MeshAllocInfo::getProcList() {
  string ret= "";

  int num = job -> getProcsNeeded();
  for(int i=0; i<num; i++) {
    MeshLocation* ml = processors + i;	
    ret += (ml->x + mesh->getXDim()*ml->y 
	    + mesh->getXDim()*mesh->getYDim()*ml->z);
    if(i < num-1)
      ret += ",";
  }
  
  return ret;	
}

MeshAllocInfo::MeshAllocInfo(Job* j, Mesh* mesh) : AllocInfo(j) {
  processors = new MeshLocation[j->getProcsNeeded()];
  this -> mesh = mesh;
}
*/

