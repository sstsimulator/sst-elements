
#include "regfile.h"

VanadisRegisterFile::VanadisRegisterFile(uint32_t thrCount, uint32_t regCount, uint32_t regWidthBytes) :
	threadCount(thrCount) {

	assert(thrCount > 1)
	assert(regCount > 2)
	assert(regWidthBytes > 8)

	register_sets = (VanadisRegisterSet*) malloc(sizeof(VanadisRegisterSet*) * thrCount);

	for(uint32_t i = 0; i < thrCount; ++i) {
		register_sets = new VanadisRegisterSet(i, regCount, regWidthBytes);
	}
}

VanadisRegisterSet* getRegisterForThread(uint32_t thrID) {
	assert(thrID < threadCount);

	return register_sets[thrID];
}
