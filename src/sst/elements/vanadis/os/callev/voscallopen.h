
#ifndef _H_VANADIS_SYSCALL_OPEN
#define _H_VANADIS_SYSCALL_OPEN

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallOpenEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallOpenEvent() : VanadisSyscallEvent() {}
	VanadisSyscallOpenEvent( uint32_t core, uint32_t thr, uint64_t path_ptr, uint64_t flags, uint64_t mode ) :
		VanadisSyscallEvent(core, thr),
		open_path_ptr( path_ptr ), open_flags( flags ),
		open_mode( mode ) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_OPEN;
	}

	uint64_t getPathPointer() const { return open_path_ptr; }
	uint64_t getFlags() const { return open_flags; }
	uint64_t getMode() const { return open_mode; }

private:
	uint64_t  open_path_ptr;
	uint64_t  open_flags;
	uint64_t  open_mode;

};

}
}

#endif
