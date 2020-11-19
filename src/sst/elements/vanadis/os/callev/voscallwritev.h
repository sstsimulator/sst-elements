
#ifndef _H_VANADIS_SYSCALL_WRITEV
#define _H_VANADIS_SYSCALL_WRITEV

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallWritevEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallWritevEvent() : VanadisSyscallEvent() {}
	VanadisSyscallWritevEvent( uint32_t core, uint32_t thr, int64_t fd, uint64_t iovec_addr, int64_t iovec_count ) :
		VanadisSyscallEvent(core, thr),
		writev_fd(fd), writev_iovec_addr(iovec_addr), writev_iov_count(iovec_count) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_WRITEV;
	}

	int64_t  getFileDescriptor() const { return writev_fd; }
	uint64_t getIOVecAddress() const { return writev_iovec_addr; }
	int64_t  getIOVecCount() const { return writev_iov_count; }

private:
	int64_t   writev_fd;
	uint64_t  writev_iovec_addr;
	int64_t   writev_iov_count;

};

}
}

#endif
