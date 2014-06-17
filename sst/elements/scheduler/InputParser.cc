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
#include <typeinfo>

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

JobParser::JobParser(Machine* machine,
          SST::Params& params,
          bool* useYumYumSimulationKill,
          bool* YumYumSimulationKillFlag )
{
    this -> machine = machine;
    this -> useYumYumSimulationKill = useYumYumSimulationKill;
    this -> YumYumSimulationKillFlag = YumYumSimulationKillFlag;
    useYumYumTraceFormat = params.find("useYumYumTraceFormat") != params.end();
    jobTrace = params["traceName"].c_str();

    lastJobRead[ 0 ] = '\0';
    
    char* inputDir = getenv("SIMINPUT");
    if (inputDir != NULL) {
        string fullName = inputDir + jobTrace;
        fileName = fullName;
    } else {
        fileName = jobTrace;
    }

    fileNamePath = boost::filesystem::path( fileName.c_str());
    
}

std::vector<Job> JobParser::parseJobs(SimTime_t currSimTime)
{
    ifstream input;
    input.open( fileName.c_str() );
    if(!input.is_open()){
        input.open(jobTrace.c_str());  //try without directory
    }                     
    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open job trace file: %s", fileName.c_str());
    }
    
    jobs.clear();

    LastJobFileModTime = boost::filesystem::last_write_time( fileNamePath );

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
bool JobParser::checkJobFile()
{
    if( boost::filesystem::exists( fileNamePath ) ){
        time_t lastWritten = boost::filesystem::last_write_time( fileNamePath );
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
bool JobParser::newYumYumJobLine(std::string line, SimTime_t currSimTime)
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


bool JobParser::newJobLine(std::string line)
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

bool JobParser::validateJob( Job* j, vector<Job>* jobs, long runningTime )
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

DParser::DParser(int numNodes, SST::Params& params)
{
    this -> numNodes = numNodes;
    fileName = params["dMatrixFile"].c_str();
    
    char* inputDir = getenv("SIMINPUT");
    if (inputDir != NULL) {
        filePath = inputDir + fileName;
    } else {
        filePath = fileName;
    }
}

double** DParser::readDMatrix()
{
    //first check file
    ifstream input;
    string inFile = fileName.c_str();
    input.open(inFile.c_str());
    if(!input.is_open()){
        input.open(filePath.c_str());  //try without directory
        inFile = filePath;
    }                     
    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open file: %s", fileName.c_str());
    }
    input.close();

    //read matrix
    MatrixMarketReader2D<double> reader = MatrixMarketReader2D<double>();
    double** dMatrix = reader.readMatrix(inFile.c_str());
    
    //check if x&y sizes and machine size match
    if(reader.xdim != reader.ydim){
        schedout.fatal(CALL_INFO, 1, "Given matrix in file %s is not a square matrix\n", inFile.c_str());
    } else if(reader.xdim != numNodes){
        schedout.fatal(CALL_INFO, 1, "The size of the matrix in file %s does not match with the job size\n", inFile.c_str());
    }
    
    return dMatrix;
}

template <class T>
T** MatrixMarketReader2D<T>::readMatrix(const char* fileName)
{
    //TODO: make this function faster by reading the file all at once
    
    T** outMatrix;
    
    //open file
    ifstream inputFile;
    inputFile.open( fileName, std::fstream::in );
    if(!inputFile.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open file: %s", fileName);
    }

    //read header
    string fType, object, format, field, symmetry;
    if( !(inputFile >> fType >> object >> format >> field >> symmetry) ){
        schedout.fatal(CALL_INFO, 1, "Cannot read matrix market file: %s\n", fileName);
    }
    
    //check compatibility
    if(fType.compare("%%MatrixMarket") != 0){
        schedout.fatal(CALL_INFO, 1, "File %s is not a Matrix Market file\n", fileName);
    }
    if(object.compare("matrix") != 0){
        schedout.fatal(CALL_INFO, 1, "Object type in file %s should be a matrix\n", fileName);
    }
    if(format.compare("coordinate") != 0){
        schedout.fatal(CALL_INFO, 1, "Matrix Market in file %s should be given in coordinate format\n", fileName);
    }
    if(typeid(int) == typeid(T)){
        if(field.compare("integer") != 0){
            schedout.fatal(CALL_INFO, 1, "In file %s : integer input type is expected\n", fileName);
        }
    }
    if(typeid(double) == typeid(T)){
        if(field.compare("real") != 0 && field.compare("double") != 0){
            schedout.fatal(CALL_INFO, 1, "In file %s : double input type is expected\n", fileName);
        }
    }
    
    //ignore comments
    while (inputFile.peek() == '%' || inputFile.peek() == '\n'){
    	inputFile.ignore(2048, '\n');
    }
    
    //read the size line
    int numLines;
    if( !(inputFile >> xdim >> ydim >> numLines) ){
        schedout.fatal(CALL_INFO, 1, "Could not read matrix market input %s\n", fileName);
    }
    
    //initialize matrix
    outMatrix = new T*[xdim];
    for(int i=0; i < xdim; i++){
        outMatrix[i] = new T[ydim];
        for(int j=0; j < ydim; j++){
            outMatrix[i][j] = 0;
        }
    }
    
    //read data
    int x=0, y=0;
    T data;
    for(int i = 0; i < numLines; i++){
        if( !(inputFile >> x >> y >> data) ){
            schedout.fatal(CALL_INFO, 1, "Could not read matrix market input %s\n", fileName);
        }
        outMatrix[x-1][y-1] = data;
    }    
    
    inputFile.close();
    
    //apply symmetry if needed
    if(symmetry.compare("general") != 0){
        for(int i = 0; i < xdim; i++){
            for(int j = i; j < ydim; j++){
                outMatrix[i][j] = outMatrix[j][i];
            }
        }
    }
    
    return outMatrix;   
}
