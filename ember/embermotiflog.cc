
#include <sst_config.h>

#include <unordered_map>
#include "embermotiflog.h"

using namespace SST::Ember;

static std::unordered_map<uint32_t, EmberMotifLogRecord*>* logHandles = nullptr;

EmberMotifLog::EmberMotifLog(const std::string logPath, const uint32_t jobID) {
	printf("EmberMotifLog START =======\n");

	if(nullptr == logHandles) {
		logHandles = new std::unordered_map<uint32_t, EmberMotifLogRecord*>();
	}

	auto logHandleFind = logHandles->find(jobID);

	if(logHandleFind == logHandles->end()) {
		printf("DID NOT FIND ENTRY .. CREATING ONE ..\n");
		FILE* loggerFile = fopen(logPath.c_str(), "wt");
		EmberMotifLogRecord* newRecord = new EmberMotifLogRecord(loggerFile);
		logRecord->increment();

		logHandles->insert( std::pair<uint32_t, EmberMotifLogRecord*>(jobID, newRecord) );
	} else {
		printf("FOUND ENTRY \n");
		EmberMotifLogRecord* logRecord = logHandleFind->second;
		logRecord->increment();
	}

	printf("EmberMotifLog DONE =====\n");
}

EmberMotifLog::~EmberMotifLog() {
	logRecord->decrement();

	if(0 == logRecord->getCount()) {
		fclose(logRecord->getFile());
		logRecord->invalidateFile();
	}
}

void EmberMotifLog::logMotifStart(std::string name, int motifNum) {
	if(nullptr != logRecord->getFile()) {
       		fprintf(logRecord->getFile(), "%d %s %s\n", motifNum, name.c_str(),
       			Simulation::getSimulation()->getElapsedSimTime().toStringBestSI().c_str());
	}
}
