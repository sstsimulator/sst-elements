
#ifndef _H_VANADIS_SYSCALL_BRK
#define _H_VANADIS_SYSCALL_BRK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallBRKEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallBRKEvent() : VanadisSyscallEvent() {}
	VanadisSyscallBRKEvent( uint32_t core, uint32_t thr, uint64_t newBrkAddr, bool zero_mem = false ) :
		VanadisSyscallEvent(core, thr), newBrk(newBrkAddr),
		zero_memory(zero_mem) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_BRK;
	}

	uint64_t getUpdatedBRK() const {
		return newBrk;
	}

	bool requestZeroMemory() const {
		return zero_memory;
	}

private:
	uint64_t newBrk;
	bool zero_memory;

};

}
}

#endif
