
#ifndef _H_VANADIS_SYSCALL_SET_THREAD_AREA
#define _H_VANADIS_SYSCALL_SET_THREAD_AREA

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSetThreadAreaEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallSetThreadAreaEvent() : VanadisSyscallEvent() {}
	VanadisSyscallSetThreadAreaEvent( uint32_t core, uint32_t thr, uint64_t ta ) :
		VanadisSyscallEvent(core, thr), ta_ptr(ta) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_SET_THREAD_AREA;
	}

	uint64_t getThreadAreaInfoPtr() const {
		return ta_ptr;
	}

private:
	uint64_t ta_ptr;

};

}
}

#endif
