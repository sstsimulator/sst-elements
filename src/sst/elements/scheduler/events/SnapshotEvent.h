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

/*
 * Classes representing system events
 */

#ifndef SST_SCHEDULER_SNAPSHOTEVENT_H__
#define SST_SCHEDULER_SNAPSHOTEVENT_H__

#include "sst/core/sst_types.h"

namespace SST {
    namespace Scheduler {

        class SnapshotEvent : public SST::Event {
            public:

                SnapshotEvent(SimTime_t time, int jobNum) : SST::Event() {
                    this -> time = time;
                    this -> jobNum = jobNum;
                    this -> nextJobArrivalTime = 0;
                }

                SimTime_t time;   //current time of the snapshot
                int jobNum;
                //ITMI itmi;
                unsigned long nextJobArrivalTime;
                std::map<int, ITMI> runningJobs;

            private:
                SnapshotEvent();

                NotSerializable(SnapshotEvent)
         };

    }
}
#endif

