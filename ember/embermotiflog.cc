
#include <sst_config.h>
#include "embermotiflog.h"

static FILE* motifLogger = nullptr;

using namespace SST::Ember;

EmberMotifLog::EmberMotifLog(const std::string logPath) {
	if(nullptr == motifLogger) {
		motifLogger = fopen(logPath.c_str(), "wt");
	}
}

EmberMotifLog::~EmberMotifLog() {
	if(nullptr != motifLogger) {
		fclose(motifLogger);
		motifLogger = nullptr;
	}
}

void EmberMotifLog::logMotifStart(std::string name, int motifNum) {
	if(nullptr != motifLogger) {
        	fprintf(motifLogger, "%d %s %s\n", motifNum, name.c_str(),
        		Simulation::getSimulation()->getElapsedSimTime().toStringBestSI().c_str());
        }
}
