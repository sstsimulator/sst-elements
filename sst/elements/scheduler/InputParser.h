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

        class InputParser {
            public:
                
                std::vector<Job> parseJobs(SimTime_t currSimTime);
                bool checkJobFile();
                
                InputParser(Machine* machine,
                            SST::Params& params, 
                            bool* useYumYumSimulationKill, 
                            bool* YumYumSimulationKillFlag );
                            
                ~InputParser() { };

            private:
                Machine* machine;
                std::vector<Job> jobs;

                std::string jobListFileName;
                boost::filesystem::path jobListFileNamePath;
                std::string trace;
                
                time_t LastJobFileModTime;            // Contains the last time that the job file was modified
                char lastJobRead[ JobIDlength ];      // The ID of the last job read from the Job list file
                
                bool newJobLine(std::string line);
                bool validateJob( Job * j, std::vector<Job> * jobs, long runningTime );
                
                //yumyum
                bool useYumYumTraceFormat;
                bool newYumYumJobLine(std::string line, SimTime_t currSimTime);
                bool* useYumYumSimulationKill;
                bool* YumYumSimulationKillFlag;
        };
    }
}
#endif /* _INPUTPARSER_H */
