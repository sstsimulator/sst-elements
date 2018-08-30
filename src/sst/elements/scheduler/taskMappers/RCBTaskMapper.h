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

#ifndef SST_SCHEDULER_RCBTASKMAPPER_H__
#define SST_SCHEDULER_RCBTASKMAPPER_H__

#include "TaskMapper.h"

#include <string>
#include <vector>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class StencilMachine;
        class MeshLocation;
        class TaskCommInfo;
        class TaskMapInfo;

        class RCBTaskMapper : public TaskMapper {

            class Rotator;

            public:

                RCBTaskMapper(const StencilMachine & inMach);

                ~RCBTaskMapper() { };

                std::string getSetupInfo(bool comment) const;

                TaskMapInfo* mapTasks(AllocInfo* allocInfo);

            private:

                typedef struct Dims
                {
                    double val[3];
                } Dims;

                //holds vector of locations along with the dimensions
                template <typename T>
                class Grouper {
                    public:
                        std::vector<T>* elements;

                        Grouper(std::vector<T>* elements, Rotator *rotator);
                        ~Grouper();

                        void sortDims(int sortedDims[3]) const; //sorts max to min.
                        void divideBy(int dim, Grouper<T>** first, Grouper<T>** second);
                        double getDim(int dim) { return dims.val[dim]; }

                   private:
                        Dims dims;
                        Rotator *rotator;
                        void sort_maxHeapify(std::vector<T> & v, int i, int dim);
                        int sort_compByDim(T first, T second, int dim); //returns >= 0 if first >= second

                   friend class Rotator;
                };

                //helps for rotations of the nodes
                class Rotator {
                    public:
                        Rotator(const RCBTaskMapper & rcb, const StencilMachine & mach); //dummy rotator for Grouper initialization
                        Rotator(Grouper<MeshLocation> *meshLocs, 
                                Grouper<int> *jobLocs,
                                const RCBTaskMapper & rcb,
                                const StencilMachine & mach );
                        ~Rotator();

                        //returns the dimensions (size) of the structure
                        template <typename T>
                        Dims getStrDims(const std::vector<T> & elements) const;
                        //returns the dimensions of an element:
                        template <typename T>
                        Dims getDims(T t) const; //template specialization workaround
                        Dims getDims(int taskID) const;
                        Dims getDims(MeshLocation loc) const;
                        //debug functions:
                        template <typename T>
                        int getTaskNum(T t) const; //template specialization workaround
                        int getTaskNum(int taskID) const;
                        int getTaskNum(MeshLocation loc) const;

                    private:
                        const RCBTaskMapper & rcb;
                        const StencilMachine & mach;
                        int** locs;
                        int* indMap; // the mesh locations may not be ordered. This saves the loc indexes
                };

                const StencilMachine & mMachine;
                std::vector<MeshLocation> nodeLocs;
                TaskCommInfo *tci;
                Job* job;

                //recursive helper function
                void mapTaskHelper(Grouper<MeshLocation>* inLocs, Grouper<int>* jobs, TaskMapInfo* tmi);
        };
    }
}
#endif /* SST_SCHEDULER_RCBTASKMAPPER_H__ */

