
#ifndef _H_VANADIS_SYSCALL_BRK
#define _H_VANADIS_SYSCALL_BRK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallBRKEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallBRKEvent() : VanadisSyscallEvent() {}
	VanadisSyscallBRKEvent( uint32_t core, uint32_t thr, uint64_t newBrkAddr ) :
		VanadisSyscallEvent(core, thr), newBrk(newBrkAddr) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_BRK;
	}

	uint64_t getUpdatedBRK() const {
		return newBrk;
	}

private:
	uint64_t newBrk;

};

}
}

#endif
