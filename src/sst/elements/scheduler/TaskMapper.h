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

#ifndef SST_SCHEDULER_TASKMAPPER_H__
#define SST_SCHEDULER_TASKMAPPER_H__

#include <string>

#include "Machine.h"

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class TaskMapInfo;

        class TaskMapper {

            public:
		        TaskMapper(const Machine & machine) : mach(machine) { }

		        virtual ~TaskMapper() {};

		        virtual std::string getSetupInfo(bool comment) const = 0;

		        //returns task mapping info of a single job; does not map the tasks
		        virtual TaskMapInfo* mapTasks(AllocInfo* allocInfo) = 0;

	        protected:
		        const Machine & mach;
        };
    }
}
#endif /* SST_SCHEDULER_TASKMAPPER_H__ */

