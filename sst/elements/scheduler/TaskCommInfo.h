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

#ifndef SST_SCHEDULER_TASKCOMMINFO_H__
#define SST_SCHEDULER_TASKCOMMINFO_H__

#include <cstddef>
#include <vector>

namespace SST {
    namespace Scheduler {

        class Job;

        class TaskCommInfo {
        
	        public:
		        TaskCommInfo(Job* job); //default: all-to-all communication
	            TaskCommInfo(Job* job, std::vector<std::vector<std::vector<int>*> >* inCommInfo); // communication matrix input
	            TaskCommInfo(Job* job, int xdim, int ydim, int zdim); // mesh dimension input
	            TaskCommInfo(Job* job, std::vector<std::vector<std::vector<int>*> >* inCommInfo, double** inCoords); //coordinate input

                TaskCommInfo(const TaskCommInfo& tci);
                
                ~TaskCommInfo();
                
                enum commType{
                    ALLTOALL = 0,
                    CUSTOM = 1,
                    MESH = 2,
                    COORDINATE = 3,
                };

                //first vector: 0:communication info, 1: corresponding weights
                //second & third vectors: adjacency list of tasks
                std::vector<std::vector<std::vector<int>*> >* getCommInfo() const;
                int** getCommMatrix() const;
                int getCommWeight(int task1, int task2) const;
                int getSize() const { return size; }
                commType getCommType() const { return taskCommType; }

		        double** coordMatrix;
		        int xdim, ydim, zdim;

	        private:
                commType taskCommType;
                //first vector: 0:communication info, 1: corresponding weights
                //second & third vectors: adjacency list of tasks
                std::vector<std::vector<std::vector<int>*> >* commInfo;

		        unsigned int size;
		        
		        void init(Job* job);
		        int** buildMeshMatrix() const; //builds mesh structured communication matrix
		        int** buildAllToAllMatrix(int size) const;
		        int** buildCustomMatrix() const;
		        void getTaskDims(int taskNo, int outDims[3]) const; //returns task position for mesh structure
        };
    }
}
#endif

