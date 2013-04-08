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

/*
 * Classes representing information about an allocation for Mesh
 * Machines
 */

#ifndef __MBSALLOCINFO_H__
#define __MBSALLOCINFO_H__

#include <vector>
#include <sstream>
#include <string>
using namespace std;

#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "MBSAllocClasses.h" //for Block
#include "misc.h"
#include "sst/core/serialization/element.h"

namespace SST {
namespace Scheduler {

class MBSMeshAllocInfo : public MeshAllocInfo {

  public:
    set<Block*, Block>* blocks;

    MBSMeshAllocInfo(Job* j) : MeshAllocInfo(j){
	//Keep track of the blocks allocated
      Block* BComp = new Block(NULL,NULL);
      blocks = new set<Block*, Block>(*BComp);
      delete BComp;
    }


    string toString(){
	string retVal = job->toString()+"\n  ";
	for(set<Block*, Block>::iterator block = blocks->begin(); block != blocks->end(); block++){
	    retVal = retVal + (*block)->toString();
	}
	return retVal;
    }
};

}
}
#endif
