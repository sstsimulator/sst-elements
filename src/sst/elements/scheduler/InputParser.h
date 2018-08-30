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

#ifndef SST_SCHEDULER_INPUTPARSER_H__
#define SST_SCHEDULER_INPUTPARSER_H__

#include <string>
#include <vector>

#include <sst/core/sst_types.h>

namespace SST {
    
    class Params;
    
    namespace Scheduler {

        class Job;
        class Machine;
        
        // the maximum length of a job ID.  used primarily for job list parsing.
#define JobIDlength 16
            
        class JobParser {
            public:
                JobParser(Machine* machine, 
                          SST::Params& params, 
                          bool* useYumYumSimulationKill, 
                          bool* YumYumSimulationKillFlag,
                          bool* doDetailedNetworkSim); //NetworkSim: added bool parameter 
                ~JobParser() { };
                        
                std::vector<Job*> parseJobs(SimTime_t currSimTime);
                //NetworkSim: parse the files for jobs completed/running on ember
                std::map<int, unsigned long> parseJobsEmberCompleted();
                std::map<int, std::pair<unsigned long, int> > parseJobsEmberRunning();
                SimTime_t ignoreUntilTime;
                //end->NetworkSim
                bool checkJobFile();
            private:
                Machine* machine;
                std::vector<Job*> jobs;

                std::string fileName;
                std::string fileNamePath;
                std::string jobTrace;

                std::string completedJobTrace; // NetworkSim: File that lists all jobs that has been completed in ember
                std::string runningJobTrace; // NetworkSim: File that lists all jobs that are still running on ember
                
                time_t LastJobFileModTime;            // Contains the last time that the job file was modified
                char lastJobRead[ JobIDlength ];      // The ID of the last job read from the Job list file
                
                bool newJobLine(std::string line);
                bool validateJob( Job * j, std::vector<Job*> * jobs, long runningTime );
                
                //yumyum
                bool useYumYumTraceFormat;
                bool newYumYumJobLine(std::string line, SimTime_t currSimTime);
                bool* useYumYumSimulationKill;
                bool* YumYumSimulationKillFlag;
                bool* doDetailedNetworkSim; //NetworkSim: added doDetailedNetworkSim parameter
        };
        
        class CommParser {
            public:
                CommParser() { }
                ~CommParser() { }
                void parseComm(Job *job);
                std::vector<std::map<int,int> >* readCommFile(std::string fileName, int procsNeeded);
            private:
                //vector: task #s
                //map<key,value> = map<communicatingTask, weight>
                double** readCoordFile(std::string fileName, int procsNeeded);
        };
        
        class DParser {
            public:
                DParser(int numNodes,
                        SST::Params& params);
                double** readDMatrix();
            private:
                int numNodes;
                std::string fileName;
                std::string filePath;
        };
            
        //helper class
        template <class T>
        class MatrixMarketReader2D {
            public:
                MatrixMarketReader2D() { };
                ~MatrixMarketReader2D() { };
		        //@ignoreValues: Ignores matrix cell values for coordinate input
		        std::vector<T*>* readMatrix(const char* fileName, bool ignoreValues = false);
                int xdim, ydim;
        };
    }
}
#endif /* _INPUTPARSER_H */
