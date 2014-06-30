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

#ifndef SST_SCHEDULER_RCBTASKMAPPER_H__
#define SST_SCHEDULER_RCBTASKMAPPER_H__

#include "TaskMapper.h"

#include <string>
#include <vector>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;
        class MeshMachine;
        class MeshLocation;
        class TaskMapInfo;

        class RCBTaskMapper : public TaskMapper {

            public:

                RCBTaskMapper(Machine* mach);

                ~RCBTaskMapper() { };

                std::string getSetupInfo(bool comment) const;

                TaskMapInfo* mapTasks(AllocInfo* allocInfo);

            private:

                MeshMachine* mMachine;
                std::vector<MeshLocation> nodeLocs;

                //job dimensions
                int jxdim;
                int jydim;
                int jzdim;

                //holds vector of locations along with the dimensions
                template <typename T>
                class Grouper {
                   public:
                       int dims[3];
                       std::vector<T>* elements;
                       Grouper(std::vector<T>* elements, const RCBTaskMapper & rcb);
                       ~Grouper();
                       int getLongestDim(int* result) const; //returns the length of the dimension
                       void divideBy(int dim, Grouper<T>** first, Grouper<T>** second);
                   private:
                       const RCBTaskMapper & rcb;
                       void sort_buildMaxHeap(std::vector<T> & v, int dim);
                       void sort_maxHeapify(std::vector<T> & v, int i, int dim);
                       int sort_compByDim(T first, T second, int dim); //returns >= 0 if first >= second
                };

                //recursive helper function
                void mapTaskHelper(Grouper<MeshLocation>* inLocs, Grouper<int>* jobs, TaskMapInfo* tmi);

                template <typename T>
                void getDims(int* x, int* y, int* z, T t) const; //template specialization workaround
                void getDims(int* x, int* y, int* z, int taskID) const;
                void getDims(int* x, int* y, int* z, MeshLocation loc) const;
                //debug functions:
                template <typename T>
                int getTaskNum(T t) const; //template specialization workaround
                int getTaskNum(int taskID) const;
                int getLocNum(MeshLocation loc) const;
        };
    }
}
#endif /* SST_SCHEDULER_RCBTASKMAPPER_H__ */
