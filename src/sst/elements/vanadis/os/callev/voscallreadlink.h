
#ifndef _H_VANADIS_SYSCALL_READLINK
#define _H_VANADIS_SYSCALL_READLINK

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallReadLinkEvent : public VanadisSyscallEvent {
public:
	VanadisSyscallReadLinkEvent() : VanadisSyscallEvent() {}
	VanadisSyscallReadLinkEvent( uint32_t core, uint32_t thr,
		uint64_t path_ptr, uint64_t buff_ptr, int64_t size ) :
		VanadisSyscallEvent(core, thr), readlink_path_ptr( path_ptr ), readlink_buff_ptr( buff_ptr ),
		readlink_buff_size( size ) {}

	VanadisSyscallOp getOperation() {
		return SYSCALL_OP_READLINK;
	}

	uint64_t getPathPointer() const   { return readlink_path_ptr;  }
	uint64_t getBufferPointer() const { return readlink_buff_ptr;  }
	int64_t  getBufferSize() const    { return readlink_buff_size; }

private:
	uint64_t  readlink_path_ptr;
	uint64_t  readlink_buff_ptr;
	int64_t   readlink_buff_size;

};

}
}

#endif
