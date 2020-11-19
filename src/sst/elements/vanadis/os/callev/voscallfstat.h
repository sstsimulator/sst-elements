
#ifndef _H_VANADIS_SYSCALL_FSTAT
#define _H_VANADIS_SYSCALL_FSTAT

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallFstatEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallFstatEvent() : VanadisSyscallEvent() {}
	VanadisSyscallFstatEvent( uint32_t core, uint32_t thr,
		int fd, uint64_t s_addr ) :
		VanadisSyscallEvent(core, thr),
		file_id(fd), fstat_struct_addr(s_addr) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_FSTAT;
	}

	int getFileHandle() const { return file_id; }
	uint64_t getStructAddress() const { return fstat_struct_addr; }

private:
	int file_id;
	uint64_t fstat_struct_addr;

};

}
}

#endif
