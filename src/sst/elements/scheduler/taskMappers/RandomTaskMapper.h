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

#ifndef SST_SCHEDULER_RANDOMTASKMAPPER_H__
#define SST_SCHEDULER_RANDOMTASKMAPPER_H__

#include "TaskMapper.h"

#include "sst/core/rng/marsaglia.h"

#include <string>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;
        class TaskMapInfo;

        class RandomTaskMapper : public TaskMapper {
        
	        public:
	        
		        RandomTaskMapper(const Machine & mach);
		        
		        ~RandomTaskMapper();
		        
		        std::string getSetupInfo(bool comment) const;

		        TaskMapInfo* mapTasks(AllocInfo* allocInfo);
		    
		    private:
		        SST::RNG::MarsagliaRNG rng; //random number generator
        };
    }
}
#endif /* SST_SCHEDULER_RANDOMTASKMAPPER_H__ */

