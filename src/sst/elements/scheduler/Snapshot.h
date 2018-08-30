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
                void append(SimTime_t snapshotTime, unsigned long nextArrivalTime, std::map<int, ITMI> runningJobs);

                std::map<int, ITMI> runningJobs;
                
                SimTime_t getSnapshotTime() const { return snapshotTime; }
                unsigned long getNextArrivalTime() const { return nextArrivalTime; }
                bool getSimFinished() const { return simFinished; }
            private:   
                SimTime_t snapshotTime;   //current time of the snapshot
                unsigned long nextArrivalTime; //arrival time of the next jobfor ember
                bool simFinished;
        };
    }
}
#endif /* SST_SCHEDULER_SNAPSHOT_H__ */

