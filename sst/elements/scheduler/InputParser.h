// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_SCHEDULER_INPUTPARSER_H__
#define SST_SCHEDULER_INPUTPARSER_H__

#include <string>
#include <vector>

#include <boost/filesystem.hpp>

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
                          bool* YumYumSimulationKillFlag ); 
                ~JobParser() { };
                        
                std::vector<Job> parseJobs(SimTime_t currSimTime);
                bool checkJobFile();
            private:
                Machine* machine;
                std::vector<Job> jobs;   //TODO: make this a pointer vector to prevent unnecessary copying

                std::string fileName;
                boost::filesystem::path fileNamePath;
                std::string jobTrace;
                
                time_t LastJobFileModTime;            // Contains the last time that the job file was modified
                char lastJobRead[ JobIDlength ];      // The ID of the last job read from the Job list file
                
                bool newJobLine(std::string line);
                int** readCommFile(std::string fileName, int procsNeeded);
                bool validateJob( Job * j, std::vector<Job> * jobs, long runningTime );
                
                //yumyum
                bool useYumYumTraceFormat;
                bool newYumYumJobLine(std::string line, SimTime_t currSimTime);
                bool* useYumYumSimulationKill;
                bool* YumYumSimulationKillFlag;
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
                T** readMatrix(const char* fileName);
                int xdim, ydim;
        };
    }
}
#endif /* _INPUTPARSER_H */
