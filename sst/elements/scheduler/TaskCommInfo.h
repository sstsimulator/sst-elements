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

namespace SST {
    namespace Scheduler {

        class Job;

        class TaskCommInfo {
	        public:
                //defaults to all-to-all communication
                TaskCommInfo(Job* job, int ** inCommMatrix = NULL, int xdim = 0, int ydim = 0, int zdim = 0);

                ~TaskCommInfo();

                int** commMatrix;
                Job* job;
                int xdim, ydim, zdim;
        };
    }
}
#endif
