
#ifndef _H_VANADIS_SYSCALL_OPENAT
#define _H_VANADIS_SYSCALL_OPENAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallOpenAtEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallOpenAtEvent() : VanadisSyscallEvent() {}
	VanadisSyscallOpenAtEvent( uint32_t core, uint32_t thr, uint64_t dirfd, uint64_t path_ptr, uint64_t flags ) :
		VanadisSyscallEvent(core, thr),
		openat_dirfd( dirfd ), openat_path_ptr( path_ptr ), openat_flags( flags ) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_OPENAT;
	}

	int64_t getDirectoryFileDescriptor() const { return openat_dirfd; }
	uint64_t getPathPointer() const { return openat_path_ptr; }
	int64_t getFlags() const { return openat_flags; }

private:
	int64_t   openat_dirfd;
	uint64_t  openat_path_ptr;
	int64_t   openat_flags;

};

}
}

#endif
