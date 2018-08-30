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
 * Class in charge of printing logs
 *
 * Performance issue: the current implementation opens and closes the
 * log file each time a line is added.  This ensures that the log is
 * up to date, but better would be flush files periodically (or at the
 * end of the simulation).
 */

#ifndef SST_SCHEDULER_STATISTICS_H__
#define SST_SCHEDULER_STATISTICS_H__

#include <string>

namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Allocator;
        class FST;
        class Machine;
        class Scheduler;
        class TaskMapInfo;
        class TaskMapper;
        class Snapshot; //NetworkSim

        class Statistics {
            public:
                Statistics(Machine* machine, Scheduler* sched, Allocator* alloc, TaskMapper* taskMap,
                           std::string baseName, char* logList, bool simulation, FST* incalcFST);

                virtual ~Statistics();

                static void printLogList(std::ostream& out);  //print list of possible logs

                void jobArrives(unsigned long time);  //called when a job has arrived

                void jobStarts(TaskMapInfo* tmi, unsigned long time);
                //called every time a job starts

                void jobFinishes(TaskMapInfo* tmi, unsigned long time);
                //called every time a job completes

                void simPauses(Snapshot *snapshot, unsigned long time);
                //NetworkSim: called when the Simulation pauses or finishes

                void done();  //called after all events have occurred

            private:
                void writeTime(AllocInfo* allocInfo, unsigned long time);
                //write time statistics to file


                void writeAlloc(TaskMapInfo* tmi);
                //write allocation information to file

                void writeSnapshot(Snapshot *snapshot);
                //NetworkSim: write snapshot information to file

                void writeVisual(std::string mesg);
                //write to log for visualization


                void writeUtil(unsigned long time);
                //method to write utilization statistics to file
                //force it to write last entry by setting time = -1

                void writeWaiting(unsigned long time);
                //possibly add line to log recording number of waiting jobs
                //  (only prints 1 line per time: #waiting jobs after all events at that time)
                //argument is current time or -1 at end of trace

                void initializeLog(std::string extension);
                void appendToLog(std::string mesg, std::string extension);

                std::string baseName;  //part of name used as base of output files (i.e. baseName.time)
                Machine* machine;
                unsigned long currentTime;
                int procsUsed;    //#processors used at currentTime

                bool* record;  //array giving whether each kind of log should be kept

                int lastUtil;  //last observed utilization value; used to filter out values w/ shared time
                unsigned long lastUtilTime;  //when it first reached this val (-1 = no observations)

                //variables to do the same thing for the number of waiting jobs:
                unsigned long lastWaitTime;  //time for which we next will record #waiting jobs
                long lastWaitJobs;  //# waiting jobs last printed
                int waitingJobs;    //current guess of what to record for that time
                int tempWaiting;    //actual number of waiting jobs right now

                std::string fileHeader;  //commented out header for all log files

                std::string outputDirectory;  //directory for all output files
                bool simulation; //if we're running a fake copy of the schedule (for calculating FST for example), we don't want to actually write to the file
                FST* calcFST;
        };

    }
}
#endif
