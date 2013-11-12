

#ifndef _H_SST_VANADIS_DECODER
#define _H_SST_VANADIS_DECODER

#include <sst_config.h>
#include <queue>

#include "microop.h"

namespace SST {
namespace Vanadis {

class VanadisDecoder {

	protected:
		VanadisDispatchEngine* dispatcher;

	public:
		VanadisDecoder(VanadisDispatchEngine* dispatch);
		virtual void decode(uint64_t* pc, char* inst_buffer) = 0;

};

}
}

#endif
