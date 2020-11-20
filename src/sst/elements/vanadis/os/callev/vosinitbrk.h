
#ifndef _H_VANADIS_SYSCALL_INIT_BRK
#define _H_VANADIS_SYSCALL_INIT_BRK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallInitBRKEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallInitBRKEvent() : VanadisSyscallEvent() {}
	VanadisSyscallInitBRKEvent( uint32_t core, uint32_t thr, uint64_t newBrkAddr ) :
		VanadisSyscallEvent(core, thr), newBrk(newBrkAddr) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_INIT_BRK;
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
