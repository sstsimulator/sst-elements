
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

		void logMotifStart(std::string name, int motifNum) {
			if(NULL != logger) {
				fprintf(logger, "%d %s %s %s %" PRIu64 "\n", motifNum, name.c_str(),
					Simulation::getSimulation()->getElapsedSimTime().toStringBestSI().c_str(),
					Simulation::getSimulation()->getElapsedSimTime().toString().c_str(),
					Simulation::getSimulation()->getCurrentSimCycle());
			}
		}

	protected:
		FILE* logger;

};

}
}

#endif
