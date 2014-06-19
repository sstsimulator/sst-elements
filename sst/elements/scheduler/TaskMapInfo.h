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

#ifndef SST_SCHEDULER_TASKMAPINFO_H__
#define SST_SCHEDULER_TASKMAPINFO_H__

#include <boost/bimap/bimap.hpp>

typedef boost::bimaps::bimap< int, int > taskMapType;

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Job;
        class Machine;
        class TaskCommInfo;

        class TaskMapInfo {

	        private:
		        taskMapType taskMap; //leftKey: task index, rightKey: node index

		        TaskCommInfo* taskCommInfo;

	        public:
		        Job* job;
		        AllocInfo* allocInfo;

		        TaskMapInfo(AllocInfo* ai);

		        ~TaskMapInfo() { };

		        //does not do the mapping; only assigns the given task
		        //assumes no task migration, i.e., a task is mapped only once at the start of execution
		        void insert(int taskInd, int nodeInd);

		        taskMapType getTaskMap();

		        unsigned long getTotalHopDist(Machine* machine);
        };
    }
}
#endif /* SST_SCHEDULER_TASKMAPINFO_H__ */
