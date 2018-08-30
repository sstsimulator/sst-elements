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

#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <typeinfo>


#include <sst/core/params.h>
#include <sst/core/stringize.h>

#include "util.h"
#include "InputParser.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "TaskCommInfo.h"

#include <iostream> //debug

using namespace std;
using namespace SST;
using namespace SST::Scheduler;

JobParser::JobParser(Machine* machine,
          SST::Params& params,
          bool* useYumYumSimulationKill,
          bool* YumYumSimulationKillFlag,
          bool* doDetailedNetworkSim ) //NetworkSim: added bool parameter
{
    this->machine = machine;
    this->useYumYumSimulationKill = useYumYumSimulationKill;
    this->YumYumSimulationKillFlag = YumYumSimulationKillFlag;
    this->doDetailedNetworkSim = doDetailedNetworkSim; //NetworkSim: added doDetailedNetworkSim parameter
    useYumYumTraceFormat = !params.find<std::string>("useYumYumTraceFormat").empty();
    jobTrace = params.find<std::string>("traceName").c_str();

    //NetworkSim: traces for completed/running jobs on ember
    char* dir = getenv("SIMOUTPUT");
    std::string outputDirectory;
    if (NULL == dir) {
        outputDirectory = "./";
    } else {
        outputDirectory = dir;
    }

    //NetworkSim: traces for completed/running jobs on ember
    if (*(this->doDetailedNetworkSim)){
        //initialize outputDirectory        
        if (NULL == dir) {
            completedJobTrace = params.find<std::string>("completedJobsTrace").c_str();
            runningJobTrace = params.find<std::string>("runningJobsTrace").c_str();
        } else {
            completedJobTrace = outputDirectory + params.find<std::string>("completedJobsTrace").c_str();
            runningJobTrace = outputDirectory + params.find<std::string>("runningJobsTrace").c_str();
        }
    }
    //end->NetworkSim

    lastJobRead[ 0 ] = '\0';
    
    char* inputDir = getenv("SIMINPUT");
    if (inputDir != NULL) {
        string fullName = inputDir + jobTrace;
        fileName = fullName;
    } else {
        fileName = jobTrace;
    }

    fileNamePath = fileName;
}

std::vector<Job*> JobParser::parseJobs(SimTime_t currSimTime)
{
    ifstream input;
    input.open( fileName.c_str() );
    if(!input.is_open()){
        input.open(jobTrace.c_str());  //try without directory
    }                     
    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open job trace file: %s\n", fileName.c_str());
    }
    
    jobs.clear();

    LastJobFileModTime = Utils::file_time_last_written( fileNamePath );

    //read line by line
    string line;
    while (!input.eof()) {
        //DEBUG
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

//NetworkSim: parser for completedJobTrace
std::map<int, unsigned long> JobParser::parseJobsEmberCompleted()
{
    fstream input;
    input.open( completedJobTrace.c_str());
    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open job trace file: %s\n", completedJobTrace.c_str());
    }

    std::map<int, unsigned long> completedEmberJobs;

    // if the file is not empty, read line by line
    input.seekg (0, ios::end);
    int length = input.tellg();

    if(length != 0){
        
        input.seekg (0, ios::beg);
        string line;
        while (!input.eof()) {
            getline(input, line);
            if (line.find_first_not_of(" \t\n") == string::npos)
                continue;

            int jobNum = -1;
            unsigned long runningTime = 0;
            string nextStr = "";

            std::stringstream is(line);
            is >> jobNum >> runningTime >> nextStr;
        
            if(!nextStr.empty()){
                schedout.fatal(CALL_INFO, 1, "Input line format is incorrect:\n\"%s\"\n", line.c_str());
            }
            if(jobNum == -1 || runningTime == 0){
                schedout.fatal(CALL_INFO, 1, "Invalid values are assigned to jobNum or runningTime:\njobNum:%d runningTime:%lu\n", jobNum, runningTime);
            }
            completedEmberJobs[jobNum] = runningTime;
        }
    } else {
        std::cout << "completedJobsTrace file is empty" << std::endl;
    }

    input.close();
    
    return completedEmberJobs;
}
//end->NetworkSim

//NetworkSim: parser for runningJobTrace, also gets the current time from ember
std::map<int, std::pair<unsigned long, int> > JobParser::parseJobsEmberRunning()
{
    fstream input;
    input.open( runningJobTrace.c_str());

    if(!input.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open job trace file: %s\n", runningJobTrace.c_str());
    }

    std::map<int, std::pair<unsigned long, int> > runningEmberJobs;

    // if the file is not empty, read line by line
    input.seekg (0, ios::end);
    int length = input.tellg();

    if(length != 0){

        input.seekg (0, ios::beg);
        string line;
        while (!input.eof()) {
            getline(input, line);
            if (line.find_first_not_of(" \t\n") == string::npos)
                continue;

            bool isTimeLine = false; // if true, the current line specifies the snapshot time from ember, else it specifies a running job from ember
            int jobNum = -1;
            unsigned long soFarRunningTime = 0;
            int currentMotifCount = -1;
            string nextStr = "";
            std::stringstream is(line);

            is >> nextStr;
            while(!nextStr.empty()){
                if(nextStr.compare("time") == 0){
                    isTimeLine = true;
                    is >> this->ignoreUntilTime;     
                } else if (nextStr.compare("job") == 0){
                    is >> jobNum >> soFarRunningTime >> currentMotifCount;
                } else {
                    schedout.fatal(CALL_INFO, 1, "Input line format is incorrect:\n\"%s\"\n", line.c_str());
                }
                if(!(is >> nextStr)){
                    break;
                }
            }

            if(!isTimeLine){
                if(jobNum == -1 || soFarRunningTime == 0 || currentMotifCount == -1){
                    schedout.fatal(CALL_INFO, 1, "Invalid values are assigned to jobNum or soFarRunningTime or currentMotifCount:\njobNum:%d soFarRunningTime:%lu currentMotifCount:%d\n", jobNum, soFarRunningTime, currentMotifCount);
                }
                runningEmberJobs[jobNum] = make_pair(soFarRunningTime, currentMotifCount);
            }
        }
    } else {
        std::cout << "runningJobsTrace file is empty" << std::endl;
        this->ignoreUntilTime = 0;
    }

    input.close();
    
    return runningEmberJobs;
}
//end->NetworkSim
/*
 * Returns true if the job list file has been modified since the last time it was checked, false otherwise
 */
bool JobParser::checkJobFile()
{
    if( Utils::file_exists( fileNamePath ) ){
        time_t lastWritten = Utils::file_time_last_written( fileNamePath );
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
    trim(line);

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

    Tokenizer< escaped_list_separator > Tokenizer(line);
    vector<string> tokens;
    tokens.assign(Tokenizer.begin(), Tokenizer.end());

    trim(tokens.at(0));
    trim(tokens.at(1));
    trim(tokens.at(2));

    strncpy(ID, tokens.at(0).c_str(), JobIDlength);
    if (JobIDlength > 0) {
        ID[JobIDlength - 1] = '\0';
    }

    uint64_t currentJobID;
    uint64_t lastJobID;

    sscanf( ID, "%" PRIu64, &currentJobID );
    sscanf( lastJobRead, "%" PRIu64, &lastJobID );

    if (lastJobRead[ 0 ] != '\0' && currentJobID <= lastJobID ){
        return false;       // We have read this job before, don't do it again.
    }

    sscanf( tokens.at( 1 ).c_str(), "%" PRIu64, &duration );
    sscanf( tokens.at( 2 ).c_str(), "%" PRIu64, &procs );

    strcpy( lastJobRead, ID );

    if (tokens.size() != 3) {
        schedout.fatal(CALL_INFO, 1, "Poorly formatted input line: %s\n", line.c_str());
    } else {
        if (useYumYumTraceFormat) {
            jobs.push_back(new Job(currSimTime + 1, procs, duration, duration + 1, std::string(ID)));
        } else {
            jobs.push_back(new Job(currSimTime, procs, duration, duration, std::string(ID)));
        }
    }

    // validate the job to make sure that the specified machine can actually run it. 
    Job* j = jobs.back();

    return validateJob(j, &jobs, abs((long)duration));
}


bool JobParser::newJobLine(std::string line)
{
    if (line.find_first_not_of(" \t\n") == string::npos)
        return false;

    unsigned long arrivalTime = -1;
    int procsNeeded = -1;
    unsigned long runningTime = 0;
    long int estRunningTime = 0;
    string nextStr = "";
    int x = 0, y = 0, z = 0;

    std::stringstream is(line);
    is >> arrivalTime >> procsNeeded >> runningTime >> estRunningTime >> nextStr;

    if(estRunningTime <= 0){
        estRunningTime = 2 * runningTime;
    }
    
    //get communication info
    Job::CommInfo commInfo;
    //NetworkSim: get phase info
    Job::PhaseInfo phaseInfo;
    //end->NetworkSim
    if(nextStr.empty()){
        commInfo.commType = TaskCommInfo::ALLTOALL;
    } else {
        while(!nextStr.empty()) {
            if(nextStr.compare("mesh") == 0) {
                if( !(is >> x >> y >> z) ) {
                    z = 1;
                }
                if(x*y*z != procsNeeded) {
                    schedout.fatal(CALL_INFO, 1, "The communication mesh structure does not match the number of processors in line:\n\"%s\"\n", line.c_str());
                } else {
                    commInfo.commType = TaskCommInfo::MESH;
                    commInfo.meshx = x;
                    commInfo.meshy = y;
                    commInfo.meshz = z;
                }
            } else if(nextStr.compare("coord") == 0){
                commInfo.commType = TaskCommInfo::COORDINATE;
                //read task communication file name
                is >> nextStr;
                commInfo.commFile = nextStr;
                //read coordinates file name
                is >> nextStr;
                commInfo.coordFile = nextStr;
            } else if(nextStr.compare("center") == 0){
                is >> commInfo.centerTask;
            } else if(nextStr.compare("comm") == 0){
                commInfo.commType = TaskCommInfo::CUSTOM;
                is >> nextStr;
                commInfo.commFile = nextStr;
            //NetworkSim: read phase file name if exists
            } else if(nextStr.compare("phase") == 0){
                is >> nextStr;
                phaseInfo.phaseFile = nextStr;
            //end->NetworkSim
            } else {
                schedout.fatal(CALL_INFO, 1, "Input line format is incorrect:\n\"%s\"\n", line.c_str());
            }
            if(!(is >> nextStr)){
                break;
            }
        }
    }

    //NetworkSim: if using the detailed network sim, we have to define a phase file
    if(*doDetailedNetworkSim == true && phaseInfo.phaseFile.empty()) {
        schedout.fatal(CALL_INFO, 1, "Phase file must be defined when using DETAILED NETWORK SIM mode:\n\"%s\"\n", line.c_str());
    }
    //end->NetworkSim

    //add job
    jobs.push_back(new Job(arrivalTime, procsNeeded, runningTime, estRunningTime, commInfo, phaseInfo)); //NetworkSim: added phase info

    //validate
    return validateJob(jobs.back() , &jobs, runningTime);
}

bool JobParser::validateJob( Job* j, vector<Job*>* jobs, long runningTime )
{
    bool ok = true;
    if (j->getProcsNeeded() <= 0) {
        schedout.verbose(CALL_INFO, 0, 0, "Warning: Job %ld  requests %d processors; ignoring it\n",
                         j->getJobNum(), j->getProcsNeeded());
        delete jobs->back();
        jobs->pop_back();
        ok = false;
    }
    if (ok && runningTime < 0) {  //time 0 also strange, but perhaps rounded down     
        schedout.verbose(CALL_INFO, 0, 0, "Warning: Job %ld  has running time of %ld; ignoring it\n",
                         j->getJobNum(), runningTime);
        delete jobs->back();
        jobs->pop_back();
        ok = false;
    }
    if (ok && j->getProcsNeeded() > (machine->numNodes * machine->coresPerNode)) {
        schedout.fatal(CALL_INFO, 1, "Job %ld requires %d processors but only %d are in the machine\n", 
                       j->getJobNum(), j->getProcsNeeded(), (machine->numNodes * machine->coresPerNode));
        ok = false;
    }
    
    return ok;
}

void CommParser::parseComm(Job * job)
{
    switch(job->commInfo.commType){
    case TaskCommInfo::ALLTOALL:
        new TaskCommInfo(job);
        break;
    case TaskCommInfo::CUSTOM:
        new TaskCommInfo(job, 
                               readCommFile(job->commInfo.commFile, job->getProcsNeeded()),
                               job->commInfo.centerTask );
        break;
    case TaskCommInfo::MESH:
        new TaskCommInfo(job, 
                               job->commInfo.meshx,
                               job->commInfo.meshy,
                               job->commInfo.meshz,
                               job->commInfo.centerTask );
        break;
    case TaskCommInfo::COORDINATE:
        new TaskCommInfo(job, 
                               readCommFile(job->commInfo.commFile, job->getProcsNeeded()),
                               readCoordFile(job->commInfo.coordFile, job->getProcsNeeded()),
                               job->commInfo.centerTask );
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown Communication type at Job %ld", 
                                      job->getJobNum() );
    }
}

std::vector<std::map<int,int> >* CommParser::readCommFile(std::string fileName, int procsNeeded)
{
    //read matrix
	MatrixMarketReader2D<int> reader = MatrixMarketReader2D<int>();
	vector<int*>* dataVec = reader.readMatrix(fileName.c_str(), true); //current version ignores edge weights

	//check size
	if(reader.xdim != reader.ydim){
		schedout.fatal(CALL_INFO, 1, "Given matrix in file %s is not a square matrix\n", fileName.c_str());
	} else if(reader.xdim != procsNeeded){
    	schedout.fatal(CALL_INFO, 1, "The size of the matrix in file %s does not match with the job size\n", fileName.c_str());
    }

	//convert matrix to adjacency & weight list
	//init vector
	std::vector<std::map<int,int> >* retVec = new std::vector<std::map<int,int> >;
	retVec->resize(procsNeeded);

	//fill with data
	for(unsigned int i = 0; i < dataVec->size(); i++){
	    retVec->at(dataVec->at(i)[0])[dataVec->at(i)[1]] = dataVec->at(i)[2];
	    delete [] dataVec->at(i);
	}
	delete dataVec;

	return retVec;
}

double** CommParser::readCoordFile(std::string fileName, int procsNeeded)
{
    //read matrix
    MatrixMarketReader2D<double> reader = MatrixMarketReader2D<double>();
    vector<double*>* dataVec = reader.readMatrix(fileName.c_str());

    //check size
    if(reader.xdim != procsNeeded){
        schedout.fatal(CALL_INFO, 1, "The size of the matrix in file %s does not match with the job size\n", fileName.c_str());
    }
    if(reader.ydim != 2 && reader.ydim != 3){
        schedout.fatal(CALL_INFO, 1, "Coordinates matrix in file %s has a dimension other than 2 or 3.\n", fileName.c_str());
    }
    
    //init matrix
    double** matrix = new double*[procsNeeded];
    for(int i=0; i < procsNeeded; i++){
        matrix[i] = new double[3];
    }

    //fill with data
    double* data;
    for(unsigned int i = 0; i < dataVec->size(); i++){
        data = dataVec->at(i);
        for(int j = 0; j < reader.ydim; j++){
            matrix[i][j] = data[j];
        }
        //fill 3rd dimension with dummy value
        if(reader.ydim == 2){
            matrix[i][2] = 0;
        }
        delete [] data;
    }
    delete dataVec;

    return matrix;
}

DParser::DParser(int numNodes, SST::Params& params)
{
    this->numNodes = numNodes;
    fileName = params.find<std::string>("dMatrixFile").c_str();
    
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
        schedout.fatal(CALL_INFO, 1, "Unable to open file: %s\n", fileName.c_str());
    }
    input.close();

    //read matrix
    MatrixMarketReader2D<double> reader = MatrixMarketReader2D<double>();
    vector<double*>* dataVec = reader.readMatrix(inFile.c_str());
    
    //check if x&y sizes and machine size match
    if(reader.xdim != reader.ydim){
        schedout.fatal(CALL_INFO, 1, "Given matrix in file %s is not a square matrix\n", inFile.c_str());
    } else if(reader.xdim != numNodes){
        schedout.fatal(CALL_INFO, 1, "The size of the matrix in file %s does not match with the job size\n", inFile.c_str());
    }
    
    //init matrix
    double** matrix = new double*[numNodes];
    for(int i = 0; i < numNodes; i++){
        matrix[i] = new double[numNodes]; 
        for(int j = 0; j < numNodes; j++){
            matrix[i][j] = 0;
        }
    }

    //fill with data
    double* data;
    for(unsigned int i = 0; i < dataVec->size(); i++){
        data = dataVec->at(i);
        matrix[ (int)data[0] ][ (int)data[1] ] = data[2];
        delete [] data;
    }
    delete dataVec;
    
    return matrix;
}

template <class T>
vector<T*>* MatrixMarketReader2D<T>::readMatrix(const char* fileName, bool ignoreValues)
{
    //TODO: make this function faster by reading the file all at once

    //open file
    ifstream inputFile;
    inputFile.open( fileName, std::fstream::in );
    if(!inputFile.is_open()){
        schedout.fatal(CALL_INFO, 1, "Unable to open matrix market file: %s\n", fileName);
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
    if(!ignoreValues){
        if(typeid(int) == typeid(T)){
            if(field.compare("integer") != 0){
                schedout.fatal(CALL_INFO, 1, "In file %s : integer input type is expected\n", fileName);
            }
        } else if(typeid(double) == typeid(T)) {
            if(field.compare("real") != 0 && field.compare("double") != 0){
                schedout.fatal(CALL_INFO, 1, "In file %s : double input type is expected\n", fileName);
            }
        }
    }

    //ignore comments
    while (inputFile.peek() == '%' || inputFile.peek() == '\n'){
        inputFile.ignore(2048, '\n');
    }

    //read the size line
    int numLines;
    if( !(inputFile >> xdim >> ydim) ) {
        schedout.fatal(CALL_INFO, 1, "Could not read matrix market input %s\n", fileName);
    }

    //read data
    vector<T*>* outVector = new vector<T*>();
    T* tempData;
    double ignoredDummy;
    string line;

    if(format.compare("coordinate") == 0){
        inputFile >> numLines;
        inputFile.ignore(2048, '\n');
        for(int i = 0; i < numLines; i++){
            tempData = new T[3];
            if(ignoreValues){
                getline(inputFile, line);
                stringstream is(line);
                is >> tempData[0] >> tempData[1] >> ignoredDummy;
                tempData[2] = 1;
            } else {
                if( !(inputFile >> tempData[0] >> tempData[1] >> tempData[2]) ){
                    schedout.fatal(CALL_INFO, 1, "Could not read matrix market input %s\n", fileName);
                }
            }
            //convert numbering convention
            tempData[0] -= 1;
            tempData[1] -= 1;
            outVector->push_back(tempData);
        }
    } else if(format.compare("array") == 0) {
        //init outVector
        for(int i = 0; i < xdim; i++){
            outVector->push_back(new T[ydim]);
        }
        //fill outVector
        for(int j = 0; j < ydim; j++){
            for(int i = 0; i < xdim; i++){
                tempData = outVector->at(i);
                if( !(inputFile >> tempData[j]) ){
                    schedout.fatal(CALL_INFO, 1, "Number of lines in file %s does not match with the matrix.\n", fileName);
                }
            }
        }
    } else {
        schedout.fatal(CALL_INFO, 1, "Unknown matrix file format in file %s\n", fileName);
    }

    inputFile.close();

    //apply symmetry if needed
    if(symmetry.compare("general") != 0){
        T* tempDataSym;
        int size = outVector->size();
        for(int i = 0; i < size; i++){
            tempData = outVector->at(i);
            if(tempData[0] != tempData[1] && tempData[2] != 0){
                tempDataSym = new T[3];
                tempDataSym[0] = tempData[1];
                tempDataSym[1] = tempData[0];
                tempDataSym[2] = tempData[2];
                outVector->push_back(tempDataSym);
            }
        }
    }

    return outVector;
}
