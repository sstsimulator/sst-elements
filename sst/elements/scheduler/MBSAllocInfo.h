// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Classes representing information about an allocation for Mesh
 * Machines
 */

#ifndef SST_SCHEDULER_MBSALLOCINFO_H__
#define SST_SCHEDULER_MBSALLOCINFO_H__

#include <sstream>
#include <string>
#include <set>

#include "sst/core/serialization.h"

#include "Job.h"
#include "MeshAllocInfo.h"
#include "MBSAllocClasses.h"

namespace SST {
    namespace Scheduler {

        class MBSMeshAllocInfo : public MeshAllocInfo {

            public:
                std::set<Block*, Block>* blocks;

                MBSMeshAllocInfo(Job* j) : MeshAllocInfo(j){
                    //Keep track of the blocks allocated
                    Block* BComp = new Block(NULL,NULL);
                    blocks = new std::set<Block*, Block>(*BComp);
                    delete BComp;
                }


                std::string toString(){
                    std::string retVal = job -> toString() + "\n  ";
                    for (std::set<Block*, Block>::iterator block = blocks -> begin(); block != blocks -> end(); block++){
                        retVal = retVal + (*block) -> toString();
                    }
                    return retVal;
                }
        };

    }
}
#endif
