// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

#include "Job.h"
#include "AllocInfo.h"
#include "MBSAllocClasses.h"

namespace SST {
    namespace Scheduler {

        class MBSMeshAllocInfo : public AllocInfo {

            public:
                std::set<Block*, Block>* blocks;
                std::vector<MeshLocation*>* nodes;

                MBSMeshAllocInfo(Job* j, const Machine & mach) : AllocInfo(j, mach){
                    //Keep track of the blocks allocated
                    Block* BComp = new Block(NULL,NULL);
                    blocks = new std::set<Block*, Block>(*BComp);
                    delete BComp;
                    nodes = new std::vector<MeshLocation*>(j -> getProcsNeeded());
                    for (int x = 0; x < j -> getProcsNeeded(); x++) {
                        nodeIndices[x] = -1;
                    }
                }
                
                ~MBSMeshAllocInfo()
                {
                    for (int x = 0; x < (int)nodes -> size(); x++) {
                        delete nodes -> at (x);
                    }
                    delete nodes;
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
