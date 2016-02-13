
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
