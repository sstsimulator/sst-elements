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

#ifndef SST_SCHEDULER_SCHEDCOMPONENT_H__
#define SST_SCHEDULER_SCHEDCOMPONENT_H__

#include <fstream>
#include <string>
#include <vector>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/sstrng.h>
#include <sst/core/elementinfo.h>

#include "output.h"
#include "misc.h" //NetworkSim: For ITMI definition

namespace SST {
    class Event;
    class Link;
    class Params;
    
    namespace Scheduler {

        class Job;
        class FaultEvent;
        class CompletionEvent;
        class ArrivalEvent;
        class Machine;
        class Scheduler;
        class Allocator;
        class TaskMapper;
        class Statistics;
        class TaskMapInfo;
        class FST;
        class JobParser;

        class Snapshot; //NetworkSim: Object that holds a snapshot of the scheduler state

        // the maximum length of a job ID.  used primarily for job list parsing.

        //struct ITMI {int i; TaskMapInfo *tmi;}; //NetworkSim: Moved this to misc.h so that Snapshot object can also use

        class schedComponent : public SST::Component {
            public:

                SST_ELI_REGISTER_COMPONENT(
                    schedComponent,
                    "scheduler",
                    "schedComponent",
                    SST_ELI_ELEMENT_VERSION(1,0,0),
                    "Schedules and allocates nodes for jobs from a simulation file",
                    COMPONENT_CATEGORY_UNCATEGORIZED
                )

                SST_ELI_DOCUMENT_PARAMS(
                    { "traceName",
                      "Name of the simulation trace file",
                      NULL
                    },
                    { "scheduler",
                      "Determines when jobs are run",
                      "First in first out priority queue"
                    },
                    { "machine",
                      "The geometry of the machine; how the nodes are laid out",
                      "Simple machine"
                    },
                    { "allocator",
                      "Assigns available nodes to each job",
                      "Simple allocator"
                    },
                    { "taskMapper",
                        "Assigns job tasks to allocated nodes",
                        "Simple task mapper"
                    },
                    { "FST",
                      "Metric to analyze scheduler in terms of social justice",
                      "None"
                    },
                    { "timeperdistance",
                      "How much the communication affects the running time",
                      "None"
                    },
                    { "faultSeed",
                        "PRNG seed for node faults",
                        "Current time"
                    },
                    { "seed",
                        "general PRNG seed for node faults",
                        "Current time"
                    },
                    { "errorLogSeed",
                        "PRNG seed for error log",
                        "Current time"
                    },
                    { "jobKillSeed",
                        "PRNG seed for job kill",
                        "Current time"
                    },
                    { "useYumYumSimulationKill",
                        "Simulation ends as the last job is finished being simulated",
                        "False"
                    },
                    { "YumYumPollWait",
                        "Frequency with which component polls the trace file ",
                        "250"
                    },
                    { "useYumYumTraceFormat",
                        "Should yumyum trace format be used",
                        "False"
                    },
                    { "printYumYumJobLog",
                        "Should yumyum job log be used",
                        "False"
                    },
                    { "printJobLog",
                        "Should job log be printed",
                        "False"
                    },
                    { "errorLatencySeed",
                        "PRNG seed for error latency for node failure",
                        "Current time"
                    },
                    { "errorCorrectionSeed",
                        "PRNG seed for probability an error is corrected",
                        "Current time"
                    },
                    { "jobLogFileName",
                        "Name for file to output job log",
                        "REQUIRED if job log used"
                    },
                    { "runningTimeSeed",
                        "RNG seed for running time deviation",
                        "Current time"
                    },
                    { "dMatrixFile",
                        "D_matrix input file",
                        "none"
                    },
                    { "coresPerNode",
                        "Number of cores per Node",
                        "1"
                    },
                    { "detailedNetworkSim",
                        "Detailed simulation of the network communication delay",
                        "OFF"
                    },
                    { "completedJobsTrace",
                        "A file that lists all jobs that has been completed in ember, needed for detailed network sim",
                        "none"
                    },
                    { "runningJobsTrace",
                        "A file that lists all jobs that are still running on ember, needed for detailed network sim",
                        "none"
                    }
                )

                SST_ELI_DOCUMENT_PORTS(
                    {"Scheduler",
                      "Used to communicate with the scheduler",
                      {"ArrivalEvent","CompletionEvent","FaultEvent","FinalTimeEvent", "JobKillEvent", "JobStartEvent"}
                    },
                    {"nodeLink%(number of node)d",
                     "Each node has an associated port to send events",
                     {"FaultEvent","JobKillEvent","JobStartEvent"}
                    }
                )

                schedComponent(SST::ComponentId_t id, SST::Params& params);
                ~schedComponent(); 
                void setup();
                void finish();
                
                //ConstraintAllocator needs this
                std::string getNodeID(int nodeIndex) const
                {
                    return nodeIDs.at(nodeIndex);
                }
            private:
                unsigned long lastfinaltime;
                schedComponent();  // for serialization only
                schedComponent(const schedComponent&); // do not implement
                void operator=(const schedComponent&); // do not implement

                void registerThis();
                void unregisterThis();
                bool isRegistered();

                bool registrationStatus;

                std::string trace;
                std::string jobListFilename;

                void handleCompletionEvent(Event *ev, int n);
                void handleJobArrivalEvent(Event *ev);

                void unregisterYourself();

                void startNextJob();
                void startJob(Job* job);

                void logJobStart(ITMI itmi);
                void logJobFinish(ITMI itmi);
                void logJobFault(ITMI itmi, FaultEvent * faultEvent );

                typedef std::vector<int> targetList_t;

                std::vector<Job*> jobs;
                std::list<CompletionEvent*> finishingcomp;
                std::list<ArrivalEvent*> finishingarr;
                Machine* machine;
                Scheduler* scheduler;
                Scheduler* FSTscheduler;
                Allocator* theAllocator;
                TaskMapper* theTaskMapper;
                Statistics* stats;
                int FSTtype;
                FST* calcFST;
                std::vector<SST::Link*> nodes;
                std::vector<std::string> nodeIDs;
                SST::Link* selfLink;
                std::map<int, ITMI> runningJobs;
                std::vector<double>* timePerDistance; //used if we want to add time to the jobs proportional to the L1 distance
                
                std::string jobLogFileName;
                std::ofstream jobLog;
                bool printJobLog;
                bool printYumYumJobLog;       // should the Job Log use the YumYum format?
                bool useYumYumTraceFormat;    // should we expect the incoming job list to use the YumYum format?
                                              // useYumYumTraceFormat is regularly used to decide if YumYum functionality should be used or not.

                //NetworkSim: enables detailed network simulation
                bool doDetailedNetworkSim; //variable that protects the original functionality without detailed network sim 
                Snapshot *snapshot;
                std::map<int, unsigned long> emberFinishedJobs; // The jobs that we have run on ember and finished
                std::map<int, std::pair<unsigned long, int> > emberRunningJobs; // The jobs that are still running on ember <jobNum, <soFarRunningTime, currentMotifCount>>
                SimTime_t ignoreUntilTime; // Avoid taking snapshots until this time
                int jobNumLastArrived;
                int numJobs;
                //end->NetworkSim

                JobParser* jobParser;

                bool useYumYumSimulationKill;         // should the simulation end on a special job (true), or just when the job list is exhausted (false)?
                bool YumYumSimulationKillFlag;        // this will signal the schedComponent to unregister itself iff useYumYumSimulationKill == true
                int YumYumPollWait;                   // this is the length of time in ms to wait between checks for new jobs
                
                SST::RNG::SSTRandom* rng;  //random number generator

        };

    }
}
#endif /* _SCHEDCOMPONENT_H */
