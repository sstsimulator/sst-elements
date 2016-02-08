
#ifndef _H_SST_EMBER_MOTIF_LOG
#define _H_SST_EMBER_MOTIF_LOG

#include <stdio.h>
#include <sst/core/simulation.h>

namespace SST {
namespace Ember {

class EmberMotifLog {
	public:
		EmberMotifLog(const std::string logPath);
		~EmberMotifLog();
		void logMotifStart(std::string name, int motifNum);

};

}
}

#endif
