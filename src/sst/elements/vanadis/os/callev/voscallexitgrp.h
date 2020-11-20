
#ifndef _H_VANADIS_SYSCALL_EXIT_GROUP
#define _H_VANADIS_SYSCALL_EXIT_GROUP

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallExitGroupEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallExitGroupEvent() : VanadisSyscallEvent() {}
	VanadisSyscallExitGroupEvent( uint32_t core, uint32_t thr, uint64_t rc ) :
		VanadisSyscallEvent(core, thr), return_code(rc) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_EXIT_GROUP;
	}

	uint64_t getExitCode() const {
		return return_code;
	}

private:
	uint64_t return_code;

};

}
}

#endif
