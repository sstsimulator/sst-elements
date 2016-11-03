// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

#include <unordered_map>
#include "embermotiflog.h"

using namespace SST::Ember;

static std::unordered_map<uint32_t, EmberMotifLogRecord*>* logHandles = NULL;

EmberMotifLog::EmberMotifLog(const std::string logPath, const uint32_t jobID) {
	if(NULL == logHandles) {
		logHandles = new std::unordered_map<uint32_t, EmberMotifLogRecord*>();
	}

	auto logHandleFind = logHandles->find(jobID);

	if(logHandleFind == logHandles->end()) {
		logRecord = new EmberMotifLogRecord(logPath.c_str());
		logRecord->increment();

		logHandles->insert( std::pair<uint32_t, EmberMotifLogRecord*>(jobID, logRecord) );
	} else {
		logRecord = logHandleFind->second;
		logRecord->increment();
	}
}

EmberMotifLog::~EmberMotifLog() {
	logRecord->decrement();

	if(0 == logRecord->getCount()) {
		fclose(logRecord->getFile());
		logRecord->invalidateFile();
	}
}

void EmberMotifLog::logMotifStart(const std::string name, const int motifNum) {
	if(NULL != logRecord->getFile()) {
		FILE* logFile = logRecord->getFile();

		const char* nameChar = name.c_str();
		const char* timeChar = Simulation::getSimulation()->getElapsedSimTime().toStringBestSI().c_str();

		fprintf(logFile, "%d %s %s\n", motifNum, nameChar, timeChar);
	}
}
