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

#include <ctime>
#include <fstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>        // for reading YumYum jobs
#include <boost/filesystem.hpp>

#include <sst/core/params.h>

#include "InputParser.h" 
#include "Job.h"
#include "Machine.h"
#include "output.h"

using namespace std;
using namespace SST;
using namespace SST::Scheduler;

InputParser::InputParser(Machine* machine,
                         SST::Params& params, 
                         bool* useYumYumSimulationKill, 
                         bool* YumYumSimulationKillFlag)
{
    this -> machine = machine;
    this -> useYumYumSimulationKill = useYumYumSimulationKill;
    this -> YumYumSimulationKillFlag = YumYumSimulationKillFlag;
    useYumYumTraceFormat = params.find("useYumYumTraceFormat") != params.end();
    trace = params["traceName"].c_str();

    lastJobRead[ 0 ] = '\0';
    
    char* inputDir = getenv("SIMINPUT");
    if (inputDir != NULL) {
        string fullName = inputDir + trace;
        jobListFileName = fullName;
    } else {
        jobListFileName = trace;
    }

    jobListFileNamePath = boost::filesystem::path( jobListFileName.c_str());
    
}

std::vector<Job> InputParser::parseJobs(SimTime_t currSimTime)
{
    ifstream input;
    input.open( jobListFileName.c_str() );
    jobs.clear();

    if(!input.is_open()){
        input.open(trace.c_str());  //try without directory
    }                     
    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open job trace file: %s", jobListFileName.c_str());
    }

    LastJobFileModTime = boost::filesystem::last_write_time( jobListFileNamePath );

    //read line by line
    string line;
    while (!input.eof()) {
        getline(input, line);
        if (useYumYumTraceFormat) {
            newYumYumJobLine(line, currSimTime);
        } else {
            newJobLine(line);
        }
    }

    input.close();
    
    return jobs;
}


/*
 * Returns true if the job list file has been modified since the last time it was checked, false otherwise
 */
bool InputParser::checkJobFile()
{
    if( boost::filesystem::exists( jobListFileNamePath ) ){
        time_t lastWritten = boost::filesystem::last_write_time( jobListFileNamePath );
        if( lastWritten > LastJobFileModTime ){
            LastJobFileModTime = lastWritten;
            return true;
        }
    }
    return false;
}

/*
   Reads in a joblist in the YumYum format.

   All jobs will have an arrival time of getCurrentSimTime().

   The YumYum format is something like:
   [Job ID], [Job duration], [Procs needed]
   */
bool InputParser::newYumYumJobLine(std::string line, SimTime_t currSimTime)
{
    boost::algorithm::trim(line);

    if (line.compare("YYKILL") == 0) {
        *YumYumSimulationKillFlag = true;
        *useYumYumSimulationKill = false;
        return false;
    }

    if (line.find_first_of("1234567890") == string::npos)
        return false;

    char ID[JobIDlength];
    uint64_t duration;
    uint64_t procs;

    boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer(line);
    vector<string> tokens;
    tokens.assign(Tokenizer.begin(), Tokenizer.end());

    boost::algorithm::trim(tokens.at(0));
    boost::algorithm::trim(tokens.at(1));
    boost::algorithm::trim(tokens.at(2));

    strncpy(ID, tokens.at(0).c_str(), JobIDlength);
    if (JobIDlength > 0) {
        ID[JobIDlength - 1] = '\0';
    }

    uint64_t currentJobID;
    uint64_t lastJobID;

    sscanf( ID, "%"PRIu64, &currentJobID );
    sscanf( lastJobRead, "%"PRIu64, &lastJobID );

    if (lastJobRead[ 0 ] != '\0' && currentJobID <= lastJobID ){
        return false;       // We have read this job before, don't do it again.
    }

    sscanf( tokens.at( 1 ).c_str(), "%"PRIu64, &duration );
    sscanf( tokens.at( 2 ).c_str(), "%"PRIu64, &procs );

    strcpy( lastJobRead, ID );

    if (tokens.size() != 3) {
        schedout.fatal(CALL_INFO, 1, "Poorly formatted input line: %s", line.c_str());
    } else {
        if (useYumYumTraceFormat) {
            jobs.push_back(Job(currSimTime + 1, procs, duration, duration + 1, std::string(ID)));
        } else {
            jobs.push_back(Job(currSimTime, procs, duration, duration, std::string(ID)));
        }
    }

    // validate the job to make sure that the specified machine can actually run it. 
    Job* j = &jobs.back();

    return validateJob(j, &jobs, abs((long)duration));
}


bool InputParser::newJobLine(std::string line)
{
    if (line.find_first_not_of(" \t\n") == string::npos)
        return false;

    unsigned long arrivalTime;
    int procsNeeded;
    unsigned long runningTime;
    unsigned long estRunningTime;
    int num = sscanf(line.c_str(), "%ld %d %ld %ld", &arrivalTime,
                     &procsNeeded, &runningTime, &estRunningTime);
    if (num < 3) schedout.fatal(CALL_INFO, 1, "Poorly formatted input line: %s",  line.c_str());
    if (3 == num) {
        jobs.push_back(Job(arrivalTime, procsNeeded, runningTime, runningTime));
    } else { //read all 4                                                        
        jobs.push_back(Job(arrivalTime, procsNeeded, runningTime, estRunningTime));
    }

    //validate                                                                  
    Job* j = &jobs.back();

    return validateJob(j, &jobs, runningTime);
}

bool InputParser::validateJob( Job* j, vector<Job>* jobs, long runningTime )
{
    bool ok = true;
    if (j -> getProcsNeeded() <= 0) {
        schedout.verbose(CALL_INFO, 0, 0, "Warning: Job %ld  requests %d processors; ignoring it",
                         j -> getJobNum(), j -> getProcsNeeded());
        jobs->pop_back();
        ok = false;
    }
    if (ok && runningTime < 0) {  //time 0 also strange, but perhaps rounded down     
        schedout.verbose(CALL_INFO, 0, 0, "Warning: Job %ld  has running time of %ld; ignoring it",
                         j -> getJobNum(), runningTime);
        jobs->pop_back();
        ok = false;
    }
    if (ok && j -> getProcsNeeded() > machine -> getNumProcs()) {
        schedout.fatal(CALL_INFO, 1, "Job %ld requires %d processors but only %d are in the machine", 
                       j -> getJobNum(), j -> getProcsNeeded(), machine -> getNumFreeProcessors());
    }
    
    return ok;
}
