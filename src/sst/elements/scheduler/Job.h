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

#ifndef SST_SCHEDULER_JOB_H__
#define SST_SCHEDULER_JOB_H__


#include <string>
#include <sstream>

#include "Statistics.h"  //needed for friend declaration
#include "TaskCommInfo.h"


namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;

        class Job {
            public:
                struct CommInfo;
                struct PhaseInfo; //NetworkSim: declare the PhaseInfo struct

                Job(unsigned long arrivalTime, int procsNeeded, unsigned long actualRunningTime,
                    unsigned long estRunningTime, CommInfo commInfo = CommInfo(), PhaseInfo phaseInfo = PhaseInfo()); //NetworkSim: added phase info
                Job(long arrivalTime, int procsNeeded, long actualRunningTime,
                    long estRunningTime, std::string ID, CommInfo commInfo = CommInfo(), PhaseInfo phaseInfo = PhaseInfo()); //NetworkSim: added phase info
                Job(const Job &job);
                
                ~Job();

                unsigned long getStartTime() const { return startTime; }
                unsigned long getArrivalTime() const { return arrivalTime; }
                unsigned long getActualTime() const { return actualRunningTime; }
                int getProcsNeeded() const { return procsNeeded; }
                long getJobNum() const { return jobNum; }
                bool hasStarted() const { return started; }
                //assumes that allocation is not considered:
                unsigned long getEstimatedRunningTime() const { return estRunningTime; }
                
                std::string * getID() {
                    if (0 == ID.length()) {
                        std::ostringstream jobNumStr;
                        jobNumStr << jobNum;
                        ID = std::string( jobNumStr.str() );
                    }
                    return & ID;
                }
                
                void reset();
                
                std::string toString();

                void start(unsigned long time);
                void setFST(unsigned long FST);
                unsigned long getFST();
                void startsAtTime(unsigned long time);
                TaskCommInfo* taskCommInfo;
                
                //following structure is used to prevent large memory usage:
                //the communication files are read only when the job starts
                const struct CommInfo{
                    CommInfo() : commType(TaskCommInfo::ALLTOALL), centerTask(-1) {}
                    TaskCommInfo::commType commType;
                    std::string commFile;
                    std::string coordFile;
                    int meshx, meshy, meshz;
                    int centerTask;
                } commInfo;

                //NetworkSim: added Phase info struct for jobs
                struct PhaseInfo{
                    PhaseInfo() : startingMotif(0), soFarRunningTime(0) {}
                    std::string phaseFile;
                    int startingMotif; // the Motif to start from in the next simulation
                    unsigned long soFarRunningTime; // the cumulative time that the job has run on ember so far (for jobs that are still running)
                }phaseInfo;
                //end->NetworkSim

                unsigned long arrivalTime;        //when the job arrived
                int procsNeeded;         //how many processors it uses
                unsigned long actualRunningTime;  //how long it runs
                unsigned long estRunningTime;     //user estimated running time
            private:
                unsigned long startTime;	     //when the job started (-1 if not running)
                //unsigned long jobFST;           //FST value for job
                bool hasRun;
                bool started;

                long jobNum;             //ID number unique to this job

                std::string ID;       // alternate ID not used by SST

                //helper for constructors:
                void initialize(unsigned long arrivalTime, int procsNeeded,
                                unsigned long actualRunningTime, unsigned long estRunningTime);

                friend class Statistics;
                friend class schedComponent;
       };
    }
}
#endif

