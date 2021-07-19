// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#ifndef _SST_EMBER_DISABLE_PARALLEL
#include <mutex>
#endif

#include <unordered_map>
#include "embermotiflog.h"

using namespace SST::Ember;

static std::unordered_map<std::string, EmberMotifLogRecord*>* logHandles = NULL;

#ifndef _SST_EMBER_DISABLE_PARALLEL
static std::mutex mapLock;
#endif

EmberMotifLog::EmberMotifLog(const std::string logPathPrefix, const uint32_t jobID) :
    jobID(jobID),
    rank(-1),
    start_time("0 ns"),
    currentMotifNum(0)
{

#ifndef _SST_EMBER_DISABLE_PARALLEL
	// Lock the map for the duration of the constructor to ensure we do not
	// trample over each other
	std::lock_guard<std::mutex> lock(mapLock);
#endif

	if(NULL == logHandles) {
		logHandles = new std::unordered_map<std::string, EmberMotifLogRecord*>();
	}

	auto logHandleFind = logHandles->find(logPathPrefix);


	if(logHandleFind == logHandles->end()) {
        std::ostringstream logFile;
        logFile << logPathPrefix;

        if ( Simulation::getSimulation()->getNumRanks().rank > 1 ) {
            logFile << "-" << Simulation::getSimulation()->getRank().rank;
        }
        logFile << ".log";

		logRecord = new EmberMotifLogRecord(logFile.str().c_str());
		logRecord->increment();

		logHandles->insert( std::pair<std::string, EmberMotifLogRecord*>(logPathPrefix, logRecord) );
	} else {
		logRecord = logHandleFind->second;
		logRecord->increment();
	}
}

EmberMotifLog::~EmberMotifLog() {
#ifndef _SST_EMBER_DISABLE_PARALLEL
	// Lock the map for the duration of the destructor to ensure we do not
        // trample over each other
        std::lock_guard<std::mutex> lock(mapLock);
#endif

	logRecord->decrement();

	if(0 == logRecord->getCount()) {
		fclose(logRecord->getFile());
		logRecord->invalidateFile();
	}
}

void EmberMotifLog::logMotifStart(int motifNum) {
    start_time = Simulation::getSimulation()->getElapsedSimTime().toStringBestSI();
    currentMotifNum = motifNum;
}

void EmberMotifLog::logMotifEnd(const std::string& name, const int motifNum) {
    if ( motifNum != currentMotifNum ) {
        // Don't have matching motif numbers, just return without logging
        return;
    }

	if(NULL != logRecord->getFile()) {
		FILE* logFile = logRecord->getFile();

		const char* nameChar = name.c_str();
        std::string endTime = Simulation::getSimulation()->getElapsedSimTime().toStringBestSI();
        const char* startTimeChar = start_time.c_str();

        // File format:  job rank motifnum motif_name start_time end_time
		fprintf(logFile, "%d %d %d %s %s %s\n", jobID, rank, motifNum, nameChar, startTimeChar, endTime.c_str());
		fflush(logFile);

	}
}
