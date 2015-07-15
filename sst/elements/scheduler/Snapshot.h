// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_SCHEDULER_SNAPSHOT_H__
#define SST_SCHEDULER_SNAPSHOT_H__

#include <map>
#include <vector>
#include "sst/core/sst_types.h"
#include "misc.h"
//#include "events/SnapshotEvent.h"

namespace SST {
    namespace Scheduler {

        class Snapshot {

            public:
                Snapshot();

                ~Snapshot();

                //void append(SnapshotEvent *ev);
                void append(SimTime_t snapshot_time, int jobNum, ITMI itmi);
                std::map<int, ITMI> runningJobs;
            
            private:   
                SimTime_t snapshot_time;   //current time of the snapshot
        };
    }
}
#endif /* SST_SCHEDULER_SNAPSHOT_H__ */

