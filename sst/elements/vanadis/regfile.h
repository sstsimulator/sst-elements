
#ifndef _H_SST_VANADIS_REG_FILE
#define _H_SST_VANADIS_REG_FILE

#include <sst_config.h>
#include <stdint.h>

#include "regset.h"

namespace SST {
namespace Vanadis {

class VanadisRegisterFile {

	protected:
		VanadisRegisterSet** register_sets;
		const uint32_t threadCount;

	public:
		VanadisRegisterFile(uint32_t thrCount, uint32_t regCount, uint32_t regWidthBytes);
		VanadisRegisterSet* getRegisterForThread(uint32_t thrID);

};

}
}

#endif
