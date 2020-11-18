
#ifndef _H_VANADIS_SYSCALL_READ
#define _H_VANADIS_SYSCALL_READ

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallReadEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallReadEvent() : VanadisSyscallEvent() {}
	VanadisSyscallReadEvent( uint32_t core, uint32_t thr, int64_t fd, uint64_t buff_ptr, int64_t count ):
		VanadisSyscallEvent(core, thr),
		read_fd(fd), read_buff_addr(buff_ptr), read_count(count) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_READ;
	}

	int64_t  getFileDescriptor() const { return read_fd; }
	uint64_t getBufferAddress() const { return read_buff_addr; }
	int64_t  getCount() const { return read_count; }

private:
	int64_t   read_fd;
	uint64_t  read_buff_addr;
	int64_t   read_count;

};

}
}

#endif
