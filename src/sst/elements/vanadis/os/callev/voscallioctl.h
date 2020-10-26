
#ifndef _H_VANADIS_SYSCALL_IOCTL
#define _H_VANADIS_SYSCALL_IOCTL

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallIOCtlEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallIOCtlEvent() : VanadisSyscallEvent() {}
	VanadisSyscallIOCtlEvent( uint32_t core, uint32_t thr,
		int64_t file_d, bool o_read, bool o_write,
		uint64_t io_request, uint64_t io_driver,
		uint64_t data_ptr, uint64_t data_len ) :
		VanadisSyscallEvent(core, thr), fd(file_d),
		op_read(o_read), op_write(o_write),
		io_op(io_request), io_drv(io_driver),
		ptr(data_ptr), ptr_len(data_len) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_IOCTL;
	}

	int64_t getFileDescriptor() const { return fd; }
	uint64_t getIOOperation() const { return io_op; }
	uint64_t getIODriver() const { return io_drv; }

	uint64_t getDataPointer() const { return ptr;  }
	uint64_t getDataLength() const { return ptr_len; }

	bool isRead() const  { return op_read;  }
	bool isWrite() const { return op_write; }

private:
	int64_t fd;

	bool op_read;
	bool op_write;

	uint64_t io_op;
	uint64_t io_drv;

	uint64_t ptr;
	uint64_t ptr_len;
};

}
}

#endif
