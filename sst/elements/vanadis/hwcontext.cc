
#include "hwcontext.h"

VanadisHWContext::VandisHWContext(uint32_t thrID, VanadisRegisterSet* regs, VanadisDispatchEngine* dispatch) :
	threadID(thrID), regset(regs), dispatcher(dispatch) {

	isHalt = false;
	pc = 0;
}

bool VanadisHWContext::isHaltState() {
	return isHalt;
}

uint32_t VanadisHWContext::getThreadID() {
	return threadID;
}

