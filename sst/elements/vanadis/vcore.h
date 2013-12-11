
#ifndef _H_SST_VANADIS_CORE
#define _H_SST_VANADIS_CORE

#include <sst/core/output.h>

using namespace SST;

namespace SST {
namespace Vanadis {

class VanadisCore {

	protected:
		VanadisHWContext* hwContexts;
		VanadisRegisterFile* regFile;

		const uint32_t hwContextCount;
		uint64_t ownerProcessor;
		Output* output;

	public:
		VanadisCore(const uint32_t hwCxtCount, const uint32_t verbosity,
			const uint64_t ownerProc, const uint32_t regCount, const uint32_t regWidthBytes);
		~VanadisCore();
		void tick();

};

}
}

#endif
