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

#ifndef SST_SCHEDULER_TASKCOMMINFO_H__
#define SST_SCHEDULER_TASKCOMMINFO_H__

#include <cstddef>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-warnings"
#include <map>
#pragma clang diagnostic pop
#include <vector>

namespace SST {
    namespace Scheduler {

        class Job;

        class TaskCommInfo {
        
	        public:
		        TaskCommInfo(Job* job); //default: all-to-all communication
	            TaskCommInfo(Job* job, std::vector<std::map<int,int> >* inCommInfo, int centerTask = -1); // communication matrix input
	            TaskCommInfo(Job* job, int xdim, int ydim, int zdim, int centerTask = -1); // mesh dimension input
	            TaskCommInfo(Job* job, std::vector<std::map<int,int> >* inCommInfo, double** inCoords, int centerTask = -1); //coordinate input

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
                std::vector<std::map<int,int> >* getCommInfo() const;
                int** getCommMatrix() const;
                int getCommWeight(int task1, int task2) const;
                int getSize() const { return size; }
                commType getCommType() const { return taskCommType; }

		        double** coordMatrix;
		        const int xdim, ydim, zdim;
		        const int centerTask;

	        private:
                commType taskCommType;
                //vector: task #s
                //map<key,value> = map<communicatingTask, weight>
                std::vector<std::map<int,int> >* commInfo;

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

