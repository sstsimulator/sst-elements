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
	            TaskCommInfo(Job* job, int ** inCommMatrix); // communication matrix input
	            TaskCommInfo(Job* job, int xdim, int ydim, int zdim); // mesh dimension input
	            TaskCommInfo(Job* job, int ** inCommMatrix, std::vector<double*>* inCoords); //coordinate input

                TaskCommInfo(const TaskCommInfo& tci);
                
                ~TaskCommInfo();
                
                int** getCommMatrix() const;

		        enum commType{
		            ALLTOALL = 0,
		            CUSTOM = 1,
		            MESH = 2,
		            COORDINATE = 3,
		        };

		        commType taskCommType;
		        std::vector<double*> *coords;
		        int xdim, ydim, zdim;

	        private:
		        int** commMatrix;
		        int size;
		        
		        void init(Job* job);
		        int** buildMeshComm(int xdim, int ydim, int zdim) const; //builds mesh structured communication matrix
		        int** buildAllToAllComm(int size) const;
        };
    }
}
#endif
