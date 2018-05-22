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

#ifndef SST_SCHEDULER_ALLOCMAPPER_H_
#define SST_SCHEDULER_ALLOCMAPPER_H_

#include "Allocator.h"
#include "TaskMapper.h"

#include <map>
#include <vector>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class Machine;
        class TaskMapInfo;

        // This class is an interface for the classes that
        // allocate a given job and map its tasks simultaneously

        class AllocMapper : public Allocator, public TaskMapper {
            public:
                AllocMapper(const Machine & mach, bool inAlloacateAndMap) : Allocator(mach), TaskMapper(mach){ allocateAndMap = inAlloacateAndMap; }
                ~AllocMapper()
                {
                    if(!mappings.empty()){
                        for(std::map<long int, std::vector<int>*>::const_iterator it = mappings.begin(); it != mappings.end(); it++){
                            delete it->second;
                        }
                    }
                }

                virtual std::string getSetupInfo(bool comment) const = 0;

                //returns information on the allocation or NULL if it wasn't possible
                //(doesn't make allocation; merely returns info on possible allocation)
                //implementations should store the task mapping information by calling addMapping()
                AllocInfo* allocate(Job* job);

                //returns task mapping info of a single job; does not map the tasks
                //deletes the job mapping info after calling the function
                TaskMapInfo* mapTasks(AllocInfo* allocInfo);

            private:
                bool allocateAndMap;
                static std::map<long int, std::vector<int>*> mappings; //keeps the task mapping after allocation

            protected:
                std::vector<bool>* isFree;      //keeps a temporary copy of node list
                //fills the two given vectors
                //@usedNodes - allocated node IDs
                //@taskToNode - tasks to machine node mapping
                virtual void allocMap(const AllocInfo & ai,
                                      std::vector<long int> & usedNodes,
                                      std::vector<int> & taskToNode) = 0;
        };
    }
}

#endif /* SST_SCHEDULER_ALLOCMAPPER_H_ */

