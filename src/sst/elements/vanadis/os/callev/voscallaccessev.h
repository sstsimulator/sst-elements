
#ifndef _H_VANADIS_SYSCALL_ACCESS
#define _H_VANADIS_SYSCALL_ACCESS

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallAccessEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallAccessEvent() : VanadisSyscallEvent() {}
	VanadisSyscallAccessEvent( uint32_t core, uint32_t thr, const uint64_t path, const uint64_t mode ) :
		VanadisSyscallEvent(core, thr), path_ptr(path), access_mode(mode) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_ACCESS;
	}

	uint64_t getPathPointer() const {
		return path_ptr;
	}

	uint64_t getAccessMode() const {
		return access_mode;
	}

protected:
	uint64_t path_ptr;
	uint64_t access_mode;

};

}
}

#endif
