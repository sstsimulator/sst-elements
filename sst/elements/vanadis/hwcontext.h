
#ifndef _H_SST_VANADIS_HWCONTEXT
#define _H_SST_VANADIS_HWCONTEXT


#include "regset.h"
#include "dispatch.h"

namespace SST {
namespace Vanadis {

class VanadisHWContext {

	protected:
		VanadisRegisterSet* regset;
		VanadisDispatchEngine* dispatcher;

		uint64_t pc;
		uint32_t threadID;
		bool inHalt;

	public:
		VanadisHWContext(uint32_t thrID, VanadisRegisterSet* regset, VanadisDispatchEngine* dispatch);
		void dispatch();
		bool inHaltState();
		uint32_t getThreadID();


};

}
}

#endif
