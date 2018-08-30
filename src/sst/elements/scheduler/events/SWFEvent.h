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

#ifndef _SWFEVENT_H
#define _SWFEVENT_H

#include <sst/core/event.h>

namespace SST {
    namespace Scheduler {

        // Based on the Standard Workload Format
        // http://www.cs.huji.ac.il/labs/parallel/workload/swf.html
        class SWFEvent : public SST::Event {
            public:
                SWFEvent() : SST::Event() 
            {
                jobStatus = SUBMITTED;
            }

                typedef enum {
                    SUBMITTED, 
                    RUNNING,
                    COMPLETED
                } jobStatus_t;

                // Current Status of the job
                jobStatus_t jobStatus;

                // a counter field, starting from 1. 
                int JobNumber;

                // in seconds. The earliest time the log refers to is zero, and is
                // the submittal time the of the first job. The lines in the log are
                // sorted by ascending submittal times. It makes sense for jobs to
                // also be numbered in this order.
                int SubmitTime;

                // in seconds. The difference between the job's submit time and the
                // time at which it actually began to run. Naturally, this is only
                // relevant to real logs, not to models.
                int WaitTime;

                // in seconds. The wall clock time the job was running (end time
                // minus start time).
                int RunTime;

                // an integer. In most cases this is also the number of processors
                // the job uses; if the job does not use all of them, we typically
                // don't know about it.
                int NumberOfAllocatedProcessors;

                // both user and system, in seconds. This is the average over all
                // processors of the CPU time used, and may therefore be smaller
                // than the wall clock runtime. If a log contains the total CPU time
                // used by all the processors, it is divided by the number of
                // allocated processors to derive the average.
                int AverageCPUTimeUsed;

                // in kilobytes. This is again the average per processor.
                int UsedMemory;

                int RequestedNumberOfProcessors;

                //  This can be either runtime (measured in wallclock seconds), or
                //  average CPU time per processor (also in seconds) -- the exact
                //  meaning is determined by a header comment. In many logs this
                //  field is used for the user runtime estimate (or upper bound)
                //  used in backfilling. If a log contains a request for total CPU
                //  time, it is divided by the number of requested processors.
                int RequestedTime;

                // (again kilobytes per processor).
                int RequestedMemory;

                // 1 if the job was completed, 0 if it failed, and 5 if cancelled.
                // If information about chekcpointing or swapping is included, other
                // values are also possible. See usage note below. This field is
                // meaningless for models, so would be -1.
                int Status;

                int UserID;
                int GroupID;

                //Number -- a natural number, between one and the number of
                //different applications appearing in the workload. in some logs,
                //this might represent a script file used to run jobs rather than
                //the executable directly; this should be noted in a header comment.
                int Executable; 

                //  -- a natural number, between one and the number of different
                //  -- queues in the system. The nature of the system's queues
                //  -- should be explained in a header comment. This field is where
                //  -- batch and interactive jobs should be differentiated: we
                //  -- suggest the convention of denoting interactive jobs by 0.
                int QueueNumber;

                // a natural number, between one and the number of different
                // partitions in the systems. The nature of the system's partitions
                // should be explained in a header comment. For example, it is
                // possible to use partition numbers to identify which machine in a
                // cluster was used.
                int PartitionNumber;

                // this is the number of a previous job in the workload, such that
                //the current job can only start after the termination of this
                //preceding job. Together with the next field, this allows the
                //workload to include feedback as described below.
                int PrecedingJobNumber;

                //this is the number of seconds that should elapse between the
                //termination of the preceding job and the submi
                int ThinkTimeFromPrecedingJob;

            private:
                NotSerializable(SWFEvent)
        }; 

    }
}
#endif /* _SWFEVENT_H */
