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

#ifndef SST_SCHEDULER_SIMPLETASKMAPPER_H__
#define SST_SCHEDULER_SIMPLETASKMAPPER_H__

#include "TaskMapper.h"

#include <string>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;
        class TaskMapInfo;

        class SimpleTaskMapper : public TaskMapper {
        
	        public:
	        
		        SimpleTaskMapper(const Machine & mach) : TaskMapper(mach) { };
		        
		        std::string getSetupInfo(bool comment) const;

		        TaskMapInfo* mapTasks(AllocInfo* allocInfo);
        };
    }
}
#endif /* SST_SCHEDULER_SIMPLETASKMAPPER_H__ */

