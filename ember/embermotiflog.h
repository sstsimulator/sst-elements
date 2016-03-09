
#ifndef _H_SST_EMBER_MOTIF_LOG
#define _H_SST_EMBER_MOTIF_LOG

#include <stdio.h>
#include <sst/core/simulation.h>

namespace SST {
namespace Ember {

class EmberMotifLogRecord {
	public:
		EmberMotifLogRecord(const char* filePath) {
			loggerFile = fopen(filePath, "wt");
		}

		~EmberMotifLogRecord() {
			if(NULL != loggerFile) {
				fclose(loggerFile);
			}
		}

		void increment() {
			motifCount++;
		}

		void decrement() {
			motifCount--;
		}

		uint32_t getCount() const {
			return motifCount;
		}

		FILE* getFile() {
			return loggerFile;
		}

		void invalidateFile() {
			loggerFile = NULL;
		}

	protected:
		FILE* loggerFile;
		uint32_t motifCount;
};

class EmberMotifLog {
	public:
		EmberMotifLog(const std::string logPath, const uint32_t jobID);
		~EmberMotifLog();
		void logMotifStart(const std::string name, const int motifNum);
	protected:
		EmberMotifLogRecord* logRecord;

};

}
}

#endif
