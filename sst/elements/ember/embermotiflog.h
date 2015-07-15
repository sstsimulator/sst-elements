
#ifndef _H_SST_EMBER_MOTIF_LOG
#define _H_SST_EMBER_MOTIF_LOG

#include <stdio.h>
#include <sst/core/simulation.h>

namespace SST {
namespace Ember {

class EmberMotifLog {

	public:
		EmberMotifLog(const std::string logPath) {
			logger = fopen(logPath.c_str(), "wt");
		}

		~EmberMotifLog() {
			fclose(logger);
		}

		void logMotifStart(std::string name) {
			if(NULL != logger) {
				fprintf(logger, "%s %s\n", name.c_str(),
					Simulation::getSimulation()->getElapsedSimTime().toStringBestSI().c_str());
			}
		}

	protected:
		FILE* logger;

};

}
}

#endif
